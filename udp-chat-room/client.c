

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> // 解决‘serv_addr’的存储大小未知

// 置空操作
#ifndef bzero
#define BZERO(v) \
  memset(&v, 0, sizeof(v))
#endif

// 控制台打印错误信息, fmt必须是双引号括起来的宏
#define CERR(fmt, ...) \
  fprintf(stderr, "[%s:%s:%d][error %d:%s]" fmt "\r\n", \
    __FILE__, __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)

// 控制台打印错误信息并退出, 同样fmt必须是 ""括起来的字符串常量
#define CERR_EXIT(fmt, ...) \
  CERR(fmt, ##__VA_ARGS__), exit(EXIT_FAILURE)

// API错误判断检测宏
#define IF_CHECK(code) \
  if((code) < 0) \
    CERR_EXIT(#code)

// 名字长度 包含 '\0'
#define _INT_NAME 64
// 报文长度 包含 '\0'
#define _INT_TEXT 512

// 发送和接受的信息体
struct umsg {
  int type; // 协议 1 => 向服务器发送名字, 2 => 向服务器发送信息, 3 => 向服务器发送退出信息
  char name[_INT_NAME]; // 用户名字
  char text[_INT_TEXT]; // 文本信息 空间换时间
};

/*
 * udp 聊天室的客户端
 */
int main(int argc, char **argv) {
  int sfd, pid;
  struct umsg msg = { 1 };
  /* 下面代码可以等价为：
   * struct sockaddr_in serv_addr;
   * memset(&serv_addr, 0, sizeof(serv_addr));
   * serv_addr.sin_family = AF_INET;
   */
  struct sockaddr_in serv_addr = { AF_INET };

  if (argc != 4) {
    CERR_EXIT("uage: %s [ip] [port] [name]", argv[0]);
  }

  // int inet_aton(const char *string, struct in_addr*addr) 将一个字符串IP地址转换为一个32位的网络序列IP地址
  IF_CHECK(inet_aton(argv[1], &serv_addr.sin_addr));

  // int atoi(const char *str) 把参数 str 所指向的字符串转换为一个整数（类型为 int 型）
  int port = atoi(argv[2]);
  if (port < 1024 || port > 65535) {
    CERR_EXIT("atoi port = %s is error!", argv[2]);
  }
  serv_addr.sin_port = htons(port);
  // char *strncpy(char *destinin, char *source, int maxlen) 将指定长度的字符串复制到字符数组中
  strncpy(msg.name, argv[3], _INT_NAME - 1); // 拼接用户名字

  IF_CHECK(sfd = socket(AF_INET, SOCK_DGRAM, 0));

  // 发送登录信息到udp服务器
  IF_CHECK(sendto(sfd, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)));

  // 开启一个进程，子进程处理发送信息，父进程接受信息
  IF_CHECK(pid = fork());
  if (pid == 0) {
    // 子进程
  } else {
    // 父进程
  }
  return 0;
}