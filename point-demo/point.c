#include <stdio.h>

int main(int argc, char **argv) {
  int var = 20; // var 这个变量存在某个地址，该地址对应某个内存单元，该内存单元存储了数据20
  
  int *ip = &var;
  
  printf("var 变量的地址: %p\n", &var);

  printf("ip 变量存储的地址: %p\n", ip);

  printf("*ip 变量的值: %d\n", *ip);

  return 0;
}