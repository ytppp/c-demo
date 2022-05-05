#include "utility.h"

int main(int argc, char **argv) {
  struct sockaddr_in servaddr; // 用于存放ip和端口的结构
  servaddr.sin_family = AF_INET;
  // htons ?
  servaddr.sin_port = htons(SERVER_PORT);
  // htonl ?
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY表示任意都可连接（因为客户端不是来自同一个网络地址）

  // 服务端创建一个 socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror_exit("Creating socket failed");
  }
  printf("Socket has been created\n");

  // servaddr与socket绑定
  if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    perror_exit("Binding servaddr and socket error.");
  }
  printf("Socket and servaddr have been bound\n");

  // 开始监听
  int ret = listen(sockfd, 5);
  if (ret < 0) {
    perror_exit("Listening error");
  }
  printf("Starting to listen at port %s:%s.\n", SERVER_IP, SERVER_PORT);

  // 在内核中创建事件表
  int epfd = epoll__create(EPOLL_SIZE); // 创建一个epoll句柄
  if (epfd < 0) {
    perror_exit("Creating epfd error");
  }
  printf("Epoll has been created, epollfd = %d\n", epfd);

  static struct epoll_event events[EPOLL_SIZE];
  // 将socket添加到内核事件表中
  addfd(epfd, sockfd, true);
  while (1) {
    // 等待事件的产生，函数返回需要处理的事件数目
    int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
    if (epoll_events_count < 0) {
      perror_exit("epoll failure");
    }
    printf("epoll_events_count = %d\n", epoll_events_count);
    // 处理这epoll_events_count个就绪事件
    for (int i = 0; i < epoll_events_count; i++) {
      int sockfd = events[i].data.fd;
      if (sockfd == sockfd) {
        // 新用户连接
        struct sockaddr_in client_sockaddr;
        socklen_t client_addr_len = sizeof(struct sockaddr_in);
        int cfd = accept(sockfd, (struct sockaddr*)&client_sockaddr, &client_addr_len);
        printf(
          "client connection from: %s : %d(IP : port), clientfd = %d \n",
          inet_ntoa(client_sockaddr.sin_addr),
          ntohs(client_sockaddr.sin_port),
          cfd
        );
        addfd(epfd, cfd, true);
        // 服务端用list保存用户连接
        clients_list.push_back(cfd);
        printf("Add new cfd = %d to epoll\n", cfd);
        printf("Now there are %d clients in the chat room\n", (int)clients_list.size());

        // 服务端发送欢迎信息  
        printf("welcome message\n");                
        char message[BUF_SIZE];
        bzero(message, BUF_SIZE);
        sprintf(message, SERVER_WELCOME, clientfd);
        int ret = send(clientfd, message, BUF_SIZE, 0);
        if (ret < 0) {
          perror("send error"); exit(-1);
        }
      } else {
        // 处理用户发来的消息，并广播，使其他用户收到信息
        int ret = sendBroadcastmessage(sockfd);
        if(ret < 0) { perror("error");exit(-1); }
      }
    }
  }
  close(sockfd);
  return 0;
}
