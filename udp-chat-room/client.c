#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h> // 解决‘serv_addr’的存储大小未知

// 名字长度 包含 '\0' \0代表字符数串的结束标志
#define INT_NAME 64
// 报文长度 包含 '\0'
#define INT_TEXT 512

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

// 发送和接受的信息体
struct umsg {
  int type; // 协议 1 => 向服务器发送名字, 2 => 向服务器发送信息, 3 => 向服务器发送退出信息
  char name[INT_NAME]; // 用户名字
  char text[INT_TEXT]; // 文本信息 空间换时间
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
  socklen_t serv_addr_len = sizeof(serv_addr);

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
  strncpy(msg.name, argv[3], INT_NAME - 1); // 拼接用户名字

  IF_CHECK(sfd = socket(AF_INET, SOCK_DGRAM, 0));

  // 发送登录信息到udp服务器
  IF_CHECK(sendto(sfd, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, serv_addr_len));

  // 开启一个进程，子进程处理发送信息，父进程接受信息
  IF_CHECK(pid = fork());
  if (pid == 0) {
    // 子进程，用来发送信息
    /*
     * 僵尸进程是当子进程比父进程先结束，而父进程又没有回收子进程，释放子进程占用的资源，此时子进程将成为一个僵尸进程。
     * UNⅨ提供了一种机制可以保证只要父进程想知道子进程结束时的状态信息，就可以得到。这种机制就是： 
     * 在每个进程退出的时候，内核释放该进程所有的资源，包括打开的文件，占用的内存等。
     * 但是仍然为其保留一定的信息（包括进程号the process ID，退出状态the termination status of the process，运行时间the amount of CPU time taken by the process等）。
     * 直到父进程通过wait / waitpid来取时才释放. 但这样就导致了问题，如果进程不调用wait / waitpid的话，那么保留的那段信息就不会释放，其进程号就会一直被占用，
     * 但是系统所能使用的进程号是有限的，如果大量的产生僵尸进程，将因为没有可用的进程号而导致系统不能产生新的进程. 此即为僵尸进程的危害，应当避免。
     */
    /*
     * signal(参数一, 参数二) 函数通知内核，当子进程结束时不关心，直接回收。参数一：要进行处理的信号 参数二：处理的方式（系统默认|忽略|捕获）
     * SIG_IGN 忽略
     */
    signal(SIGCHLD, SIG_IGN);
    while(fgets(msg.text, INT_TEXT, stdin)) {
      if (strcasecmp(msg.text, "quit\n") == 0) {
        msg.type = 3;
        // 发送数据并检测
        IF_CHECK(sendto(sfd, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, serv_addr_len));
        break;
      }
      msg.type = 2;
      IF_CHECK(sendto(sfd, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, serv_addr_len));
    }
    // 关闭socket并杀死父进程
    close(sfd);
    kill(getppid(), SIGKILL);
    exit(0);
  }
  // 父进程 用来接受信息
  while(1) {
    bzero(&msg, sizeof(msg));
    IF_CHECK(recvfrom(sfd, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, &serv_addr_len));
    msg.name[INT_NAME - 1] = msg.text[INT_TEXT - 1] = '\0';
    switch(msg.type) {
      case 1:
        printf("%s 登录了聊天室!\n", msg.name);
        break;
      case 2:
        printf("%s 说了：%s\n", msg.name, msg.text);
        break;
      case 3:
        printf("%s 退出了聊天室!\n", msg.name);
        break;
      default:
        // 未识别的异常报文,程序直接退出
        fprintf(
          stderr,
          "msg is error! [%s:%d] => [%c:%s:%s]\n",
          inet_ntoa(serv_addr.sin_addr),
          ntohs(serv_addr.sin_port),
          msg.type,
          msg.name,
          msg.text
        );
        goto __exit;
    }
  }

__exit:
  close(sfd);
  kill(pid, SIGKILL);
  waitpid(pid, NULL, -1);

  return 0;
}