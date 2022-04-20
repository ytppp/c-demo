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

void perror_exit(const char *str) {
  perror(str);
  exit(1);
}

int main(int argc, char **argv){
  int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket_fd < 0) {
    perror_exit("Creating socket failed");
  }
  printf("Socket has been created\n");

  // 设置目的IP地址
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET; // 使用IPv4协议
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = inet_addr(argv[1]); // 设置接收方IP

  char message[BUF_SIZE];
  while(1) {
    printf("Please input msg:");
    scanf("%s", message);
    sendto(udp_socket_fd, message, sizeof(message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (strcmp(message, "exit") == 0) {
      break;
    }
    memset(message, 0, sizeof(message));
  }

  close(udp_socket_fd);
  return 0;
}