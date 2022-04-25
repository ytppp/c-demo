#include <stdio.h>

const int MAX = 3;

int main(int argc, char **argv) {
  int var[] = {10, 100, 1000};
  int i, *ptr;
  ptr = var;

  for(i = 0; i < MAX; i++) {
    printf("存储地址：var[%d] = %p\n", i, ptr);
    printf("存储值：var[%d] = %d\n", i, *ptr);
    ptr++;
  }
  printf("======\n");
  ptr = &var[MAX-1];
  for(i = MAX; i > 0; i--) {
    printf("存储地址：var[%d] = %p\n", i - 1, ptr);
    printf("存储值：var[%d] = %d\n", i - 1, *ptr);
    ptr--;
  }
  printf("======\n");
  ptr = var;
  i=0;
  while(ptr <= &var[MAX-1]) {
    printf("存储地址：var[%d] = %p\n", i, ptr);
    printf("存储值：var[%d] = %d\n", i, *ptr);
    ptr++;
    i++;
  }
  printf("======\n");
  ptr = &var[MAX - 1];
  i = MAX - 1;
  while(ptr >= var){
    printf("存储地址：var[%d] = %p\n", i, ptr);
    printf("存储值：var[%d] = %d\n", i, *ptr);
    ptr--;
    i--;
  }
  return 0;
}