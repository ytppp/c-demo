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

void perr_exit(const char *str)
{
  perror(str);
  exit(1);
}

int main(int argc, char **argv)
{
  int sfd, cfd;
  // 1.创建套接字
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr, client_addr;
  int len, i;
  socklen_t addr_len;

  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERV_PORT);
  // INADDR_ANY表示任意都可连接（因为客户端不是来自同一个网络地址）
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // 2.绑定本地某一端口
  if (bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perr_exit("bind");
  }
  // 3. 在套接字上设置监听
  listen(sfd, 128);
  printf("wait for connect-------\n");
  addr_len = sizeof(client_addr);
  // 第二个参数保存发送连接请求的主机和端口。
  cfd = accept(sfd, (struct sockaddr*)&client_addr, &addr_len);
  // 返回代表客户端的套接字，可以利用这个套接字与客户端交换数据
  if (cfd == -1)
  {
    perr_exit("accept");
  }

  char buf[256];

  printf("client IP: %s %d\n",
  	inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, buf, sizeof(buf)),
  	ntohs(client_addr.sin_port)
  );

  while (1)
  {
    // 读取客户端数据
    len = recv(cfd, buf, sizeof(buf), 0);
    if (len == -1)
    {
        perr_exit("recv");
    }
    if (write(STDOUT_FILENO, buf, len) < 0)
    {
      perr_exit("send dddd");
    }
    if (send(cfd, buf, len, 0) < 0)
    {
      perr_exit("write");
    }
  }
  close(sfd);
  close(cfd);
  return 0;
}
