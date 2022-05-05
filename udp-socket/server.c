#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define SERVER_PORT 8088
#define BUF_SIZE 1024

/*
  这里简单比较一下TCP和UDP在编程实现上的一些区别：

  TCP流程
    建立一个TCP连接需要三次握手，而断开一个TCP则需要四个分节。
    当某个应用进程调用close(主动端)后(可以是服务器端，也可以是客户端)，这一端的TCP发送一个FIN，表示数据发送完毕；
    另一端(被动端)发送一个确认，当被动端待处理的应用进程都处理完毕后，发送一个FIN到主动端，并关闭套接口，
    主动端接收到这个FIN后再发送一个确认，到此为止这个TCP连接被断开。 

  UDP套接口
  　UDP套接口是无连接的、不可靠的数据报协议；既然他不可靠为什么还要用呢？
    其一：当应用程序使用广播或多播是只能使用UDP协议；其二：由于它是无连接的，所以速度快。
    因为UDP套接口是无连接的，如果一方的数据报丢失，那另一方将无限等待，解决办法是设置一个超时。
    在编写UDP套接口程序时，有几点要注意：
    建立套接口时socket函数的第二个参数应该是SOCK_DGRAM，说明是建立一个UDP套接口；
    由于UDP是无连接的，所以服务器端并不需要listen或accept函数；
    当UDP套接口调用connect函数时，内核只记录连接放的IP地址和端口，并立即返回给调用进程。
  */

/*
 * recvfrom()/sendto()这两个函数基本等同于 一个 recv 和 send. 详细参数解释如下
 * s    : 文件描述符,等同于 socket返回的值
 * buf  : 数据起始地址
 * len  : 发送数据长度或接受数据缓冲区最大长度
 * flags: 发送标识,默认就用O.带外数据使用 MSG_OOB, 偷窥用MSG_PEEK .....
 * addr : 发送的网络地址或接收的网络地址
 * alen : sento标识地址长度做输入参数, recvfrom表示输入和输出参数.可以为NULL此时addr也要为NULL
 * 
 * 返回 0 表示执行成功,否则返回 < 0.
 */

void perror_exit(const char *str) {
  perror(str);
  exit(1);
}

int main(int argc, char **argv) {
  int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket_fd < 0) {
    perror_exit("Creating socket failed");
  }
  printf("Socket has been created\n");

  // 设置udp的地址并与socket绑定
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET; // 使用 ipv4 协议
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  int server_addr_len = sizeof(server_addr);
  if (bind(udp_socket_fd, (struct sockaddr*)&server_addr, server_addr_len) < 0) {
    perror_exit("Binding sockaddr and socket error.");
  }
  printf("Socket and sockaddr have been bound\n");

  // 读取客户端数据
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  char message[BUF_SIZE];
  while (1) {
    int len = recvfrom(udp_socket_fd, message, sizeof(message), 0, (struct sockaddr*)&client_addr, &client_addr_len);
    if (len < 0) {
      break;
    }
    printf(
      "client from: %s %d\n",
      inet_ntoa(client_addr.sin_addr),
      ntohs(client_addr.sin_port)
    );
    printf(
      "message: %s\n",
      message
    );
    if (strcmp(message, "exit") == 0) {
      break;
    }
    memset(message, 0, sizeof(message));
  }

  close(udp_socket_fd);
  return 0;
}