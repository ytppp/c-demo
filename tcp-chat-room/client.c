#include "utility.h"

int main(int argc, char **argv) {
  // 客户端要连接的服务端地址
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERVER_PORT);
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);

  // 客户端创建一个 socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror_exit("Failed to create socket");
  }
  printf("Socket has been created\n");

  // 向服务器发出连接请求  
  if (connect(sockfd, (struct sockaddr*)&serv_sockaddr, sizeof(serv_sockaddr)) < 0) {
    perror_exit("Connecting error");
  }
  printf("Connecting server success\n");

  // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
  int pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    perror_exit("pipe error");
  }
  printf("Connecting pipe success\n");

  // 在内核中创建事件表
  int epfd = epoll__create(EPOLL_SIZE); // 创建一个epoll句柄
  if (epfd < 0) {
    perror_exit("Creating epfd error");
  }
  printf("Epoll has been created, epollfd = %d\n", epfd);

  static struct epoll_event events[2];
  // 将socket添加到内核事件表中
  addfd(epfd, sockfd, true);
  // 将管道读端描述符添加到内核事件表中
  addfd(epfd, pipe_fd[0], true);
  // 表示客户端是否正常工作
  bool isClientwork = true;

  // 聊天信息缓冲区
  char message[BUF_SIZE];

  int pid = fork();

  if (pid < 0) {
    perror_exit("fork error");
  } else if (pid == 0) {
    // 子进程
    // 负责写入管道，因此先关闭读端
    close(pipe_fd[0]); 
    printf("Please input 'exit' to exit the chat room\n");

    while (isClientwork) {
      bzero(&message, BUF_SIZE);
      fgets(message, BUF_SIZE, stdin);

      // 客户输出exit，退出
      if(strncasecmp(message, EXIT, strlen(EXIT)) == 0){
        isClientwork = 0;
      } else {
        if (write(pipe_fd[1], message, strlen(message) - 1) < 0) {
          perror_exit("fork error");
        }
      }
    }
  } else {
    // 父进程
    // 负责读管道数据，因此先关闭写端
    close(pipe_fd[1]);
    while (isClientwork) {
      int epoll_events_count = epoll_wait( epfd, events, 2, -1 );
      // 处理就绪事件
      for(int i = 0; i < epoll_events_count ; i++) {
        bzero(&message, BUF_SIZE);
        // 服务端发来消息
        if (events[i].data.fd == sock) {
          // 接受服务端消息
          int ret = recv(sock, message, BUF_SIZE, 0);
          // ret= 0 服务端关闭
          if (ret == 0) {
            printf("Server closed connection: %d\n", sock);
            close(sock);
            isClientwork = 0;
          } else {
            printf("%s\n", message);
          }
        } else {
          // 子进程写入事件发生，父进程处理并发送服务端
          // 父进程从管道中读取数据
          int ret = read(events[i].data.fd, message, BUF_SIZE);
          if (ret == 0) {
            isClientwork = 0;
          } else {
            // 将信息发送给服务端
            send(sock, message, BUF_SIZE, 0);
          }
        }
      }
    }
  }

  if (pid) {
    //关闭父进程和sock
    close(pipe_fd[0]);
    close(sockfd);
  }else{
    //关闭子进程
    close(pipe_fd[1]);
  }
  return 0;
}
