#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAXLINE 255
#define SERV_PORT 8000

void perr_exit(const char *str) {
  // perror函数：把一个描述性错误信息输出到标砖错误 stderr。首先输出字符串str，后跟一个冒号，然后是一个空格。
  perror(str);
  // exit函数：立即终止调用进程。
  exit(1);
}

int main(int argc, char **argv) {
  // 创建套接字
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  // 指定服务器IP地址和端口号的结构体
  /*
    sockaddr 在头文件 #include <sys/socket.h> 中定义，定义如下：
      struct sockaddr
      {
        unsigned short int sa_amily; // 地址族
        char sa_data[4]; // 14字节，包含套接字中的目标地址和端口信息
      }
    sockaddr_in在头文件#include<netinet/in.h>或#include <arpa/inet.h>中定义，该结构体
    解决了sockaddr的缺陷。把目标地址和端口信息分开存储在两个变量中。
      struct sockaddr_in
      {
        short int sin_family; // 地址组
        uint15_t sin_port; // 16位 TCP/UDP 端口号
        struct in_addr sin_addr; // 32位IP地址
        char sin_zero[8];
      }
      struct in_addr
      {
        in_addr_t s_addr
      }
  */
  struct sockaddr_in serv_addr;
  int len;
  char buf[MAXLINE];
  // 创建 UDP 套接字
  // sfd = socket(AF_INET, SOCK_DRGAM, 0);

  // 将 serv_addr 各个字段清零
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  // 设置地址族
  serv_addr.sin_family = AF_INET;
  // 设置端口
  // htons()作用是将端口号由主机字节序转换为网络字节序的整数值
  serv_addr.sin_port = htons(SERV_PORT);
  // 将 ipv4 或 ipv6 地址转换为二进制形式
  // inet_pton(AF_INET, argv[1], &serv.sin_addr.s_addr);
  // inet_addr()作用是将一个IP字符串转化为一个网络字节序的整数值，用于sockaddr_in.sin_addr.s_addr
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  if (connect(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perr_exit("connect");
  }

  while (fgets(buf, sizeof(buf), stdin)) {
    // 写数据到服务器
    if (send(sfd, buf, strlen(buf), 0) < 0) {
      perr_exit("send38");
    }
    // 读取服务器
    len = recv(sfd, buf, sizeof(buf), 0);
    if (len < 0) {
      perr_exit("read error");
    }
    if (len == 0) {
      printf("the other size closed\n");
      close(sfd);
      exit(1);
    }
    if (write(STDOUT_FILENO, buf, len) < 0) {
      perr_exit("write error");
    }
  }
  close(sfd);
  return 0;
}
