#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clients_list save all the clients's socket
struct clients_list {
  int data;
  struct clients_list* next;
}

#define SERVER_IP "127.0.0.1"

#define SERVER_PORT 8888

// epoll size
#define EPOLL_SIZE 5000

// message buffer size
#define BUF_SIZE 0xFFFF

#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"

#define SERVER_MESSAGE "ClientID %d say >> %s"

#define EXIT "EXIT"

#define CAUTION "There is only one in the chat room!"

void perror_exit(const char *str) {
  perror(str);
  exit(1);
}

// 将文件描述符设置为非阻塞方式
int setnonblocking(int sockfd) {
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);
  return 0;
}

void addfd(int epollfd, int fd, bool enable_et) {
  struct epoll_event ev;
  ev.data.fd = fd;
  /*
  epoll主要是事件回调运行的，我们使用socket的时候主要使用两个事件:
  
  EPOLLOUT事件只有在连接时触发一次，表示可写，其他时候想要触发，那你要先准备好下面条件：
    1.某次write，写满了发送缓冲区，返回错误码为EAGAIN。
    2.对端读取了一些数据，又重新可写了，此时会触发EPOLLOUT。
  简单地说：EPOLLOUT事件只有在不可写到可写的转变时刻，才会触发一次，所以叫边缘触发。
  如果你真的想强制触发一次，也是有办法的，直接调用epoll_ctl重新设置一下event就可以了，event跟原来的设置一模一样都行（但必须包含EPOLLOUT），关键是重新设置，就会马上触发一次EPOLLOUT事件。
  从应用上看这个事件是本地端状态改变了。

  EPOLLIN事件：
  EPOLLIN事件则只有当对端有数据写入时才会触发，所以触发一次后需要不断读取所有数据直到读完EAGAIN为止。否则剩下的数据只有在下次对端有写入时才能一起取出来了。
  现在明白为什么说epoll必须要求异步socket了吧？如果同步socket，而且要求读完所有数据，那么最终就会在堵死在阻塞里。
  */
  ev.events = EPOLLIN;
  if (enable_et) {
    ev.events = EPOLLIN | EPOLLET;
  }
  /* epoll_ctl
  epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
  函数功能：epoll事件注册函数
  参数epfd为epoll的句柄，即epoll_create返回值
  参数op表示动作，用3个宏来表示:
   EPOLL_CTL_ADD (注册新的fd到epfd)
   EPOLL_CTL_MOD (修改已经注册的fd的监听事件)
   EPOLL_CTL_DEL (从epfd删除一个fd)
  参数fd为需要监听的标志符
  参数event告诉内核需要监听的事件，event的结构如下:
   struct epoll_event {
     __uint32_t events; // Epoll events 宏的集合，本项目主要使用EPOLLIN（表示对应的文件描述符可以读）
     epoll_data_t data; // User dada variable
   }
  */
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
  setnonblocking(fd);
  printf("fd addef to epoll!\n");
}

int sendBroadcastmessage (int clientfd) {
  // buf[BUF_SIZE] receive new chat message
  // message[BUF_SIZE] save format message
  char buf[BUF_SIZE], message[BUF_SIZE];
  bzero(buf, BUF_SIZE);
  bzero(message, BUF_SIZE);

  // receive message
  printf("read from client(clientID = %d)\n", clientfd);
  int len = recv(clientfd, buf, BUF_SIZE, 0);

  if (len == 0) {
    // len = 0 means the client closed connection
    close(clientfd);
    clients_list.remove(clientfd);
    printf("The clientId = %d closed.\n Now there are %d clients in the chat room\n",
      clientfd, clients_list.size()));
  } else {
    // broadcast message
    if (clients_list.size() == 1) {
      // this means there is only one in the chat room.
      send(clientfd, CAUTION, strlen(CAUTION), 0);
      return len;
    }
    sprintf(message, SERRVER_MESSAGE, clientfd, buf);

    list<int>::iterator it;
    for(it = clients_list.begin(); it != clients_list.end(); it++) {
      if (*it != clientfd) {
        if (send(*it, message, BUF_SIZE, 0) < 0) {
          perror_exit("error");
        }
      }
    }
  }
  return len;
}

#endif