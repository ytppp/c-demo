#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// 名字长度 包含 '\0' \0代表字符数串的结束标志
#define INT_NAME 64
// 报文长度 包含 '\0'
#define INT_TEXT 64

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
  int type; // 协议 1 => 新用户登录 2 => 广播信息, 3 => 退出
  char name[INT_NAME]; // 用户名字
  char text[INT_TEXT]; // 文本信息 空间换时间
};

// 维护一个客户端链表信息,记录登录信息
typedef struct ucnode {
  struct sockaddr_in addr;
  struct ucnode* next;
} *ucnode_t;

/*
 * 使用inline关键字可以使程序的执行效率更高，同时是代码更紧凑。
 * 但是并不是所有的函数都可以将其声明为inline函数，因此使用inline函数还应注意一下几个问题：
 *  函数的展开是由编译器来决定的，这一点对程序员是透明的;
 *  只有在代码很短的情况下，函数才会被展开。递归函数是不会被展开的。
*/
// 新建一个结点对象
static inline ucnode_t new_ucnode(struct sockaddr_in *pa) {
  // void *calloc(size_t nitems, size_t size) 分配所需的内存空间，并返回一个指向它的指针。
  //  nitems -- 要被分配的元素个数。
  //  size -- 元素的大小。
  // void *malloc(size_t size) 分配所需的内存空间，并返回一个指向它的指针。
  //  size -- 内存块的大小，以字节为单位。
  // malloc 和 calloc 之间的不同点是，malloc 不会设置内存为零，而 calloc 会设置分配的内存为零。
  ucnode_t node = calloc(sizeof(struct ucnode), 1);
  if (NULL == node) {
    CERR_EXIT("calloc sizeof struct ucnode is error. ");
  }
  node->addr = *pa;
  return node;
}

// 插入数据，这里head默认头结点是当前服务器结点
static inline void insert_ucnode(ucnode_t head, struct sockaddr_in *pa) {
  ucnode_t node = new_ucnode(pa);
  node->next = head->next;
  head->next = node;
}

// 这里是用户登录处理
static void login_ucnode(ucnode_t head, int sfd, struct sockaddr_in *pa, struct umsg *msg) {
  insert_ucnode(head, pa);
  head = head->next;
  // 从此之后才为以前的链表
  while (head->next){
    head = head->next;
    IF_CHECK(sendto(sfd, msg, sizeof(*msg), 0, (struct sockaddr*)&head->addr, sizeof(struct sockaddr_in)));
  }
}

// 信息广播
static void broadcast_ucnode(ucnode_t head, int sfd, struct sockaddr_in *pa, struct umsg *msg) {
  int flag = 0; // 1表示已经找到了
  while (head->next) {
    head = head->next;
    // int memcmp(const void *str1, const void *str2, size_t n)) 把存储区 str1 和存储区 str2 的前 n 个字节进行比较。
    if (flag || !(flag = memcmp(pa, &head->addr, sizeof(struct sockaddr_in)) == 0)){
      IF_CHECK(sendto(sfd, msg, sizeof(*msg), 0, (struct sockaddr*)&head->addr, sizeof(struct sockaddr_in)));
    }
  }
}

// 有人退出群聊
static void quit_ucnode(ucnode_t head, int sfd, struct sockaddr_in* pa, struct umsg* msg) {
  int flag = 0;// 1表示已经找到
  while (head->next) {
    if (flag || !(flag = memcmp(pa, &head->next->addr, sizeof(struct sockaddr_in)) == 0)){
      IF_CHECK(sendto(sfd, msg, sizeof(*msg), 0, (struct sockaddr*)&head->next->addr, sizeof(struct sockaddr_in)));
      head = head->next;
    } else { //删除这个退出的用户
      ucnode_t tmp = head->next;
      head->next = tmp->next;
      free(tmp);
    }
  }
}

// 销毁维护的对象池
static void destroy_ucnode(ucnode_t* phead) {
  ucnode_t head;
  if (!phead || !(head = *phead)) {
    return;
  }
  while (head) {
    ucnode_t tmp = head->next;
    free(head);
    head = tmp;
  }
  *phead = NULL;
}

int main(int argc, char **argv) {
  int sfd;
  struct sockaddr_in serv_addr = { AF_INET };
  socklen_t serv_addr_len = sizeof(serv_addr);
  struct umsg msg;
  ucnode_t head;

  if (argc != 3) {
    CERR_EXIT("uage: %s [ip] [port]", argv[0]);
  }
  // int inet_aton(const char *string, struct in_addr*addr) 将一个字符串IP地址转换为一个32位的网络序列IP地址
  IF_CHECK(inet_aton(argv[1], &serv_addr.sin_addr));

  // int atoi(const char *str) 把参数 str 所指向的字符串转换为一个整数（类型为 int 型）
  int port = atoi(argv[2]);
  if (port < 1024 || port > 65535) {
    CERR_EXIT("atoi port = %s is error!", argv[2]);
  }
  serv_addr.sin_port = htons(port);

  IF_CHECK(sfd = socket(AF_INET, SOCK_DGRAM, 0));
  IF_CHECK(bind(sfd, (struct sockaddr*)&serv_addr, serv_addr_len));
    
  // 开始判断处理
  head = new_ucnode(&serv_addr); 
  while(1) {
    bzero(&msg, sizeof(msg));
    IF_CHECK(recvfrom(sfd, &msg, sizeof(msg), 0, (struct sockaddr*)&serv_addr, &serv_addr_len));
    msg.text[strlen(msg.text) -1] = '\0';
    printf("%s", msg.text, strlen(msg.text));
    fprintf(
      stdout,
      "msg is [%s:%d] => [%d:%s:%s]\n",
      inet_ntoa(serv_addr.sin_addr),
      ntohs(serv_addr.sin_port),
      msg.type,
      msg.name,
      msg.text
    );
    switch(msg.type) {
      case 1:
        login_ucnode(head, sfd, &serv_addr, &msg);
        break;
      case 2:
        broadcast_ucnode(head, sfd, &serv_addr, &msg);
        break;
      case 3:
        quit_ucnode(head, sfd, &serv_addr, &msg);
        break;
      default:
        // 未识别的异常报文,程序把其踢走
        fprintf(
          stderr,
          "msg is error! [%s:%d] => [%d:%s:%s]\n",
          inet_ntoa(serv_addr.sin_addr),
          ntohs(serv_addr.sin_port),
          msg.type,
          msg.name,
          msg.text
        );
        quit_ucnode(head, sfd, &serv_addr, &msg);
    }
  }

  close(sfd);
  destroy_ucnode(&head);
  return 0;
}