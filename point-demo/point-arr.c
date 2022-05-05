#include <stdio.h>

const int MAX = 3;

int main(int argc, char **argv) {
  int arr[] = {10, 100, 1000};
  int i, *ptr[MAX];

  for(i = 0; i < MAX; i++) {
    ptr[i] = &arr[i];
  }
  for(i = 0; i < MAX; i++) {
    printf("地址: %p, 值: %d\n", ptr[i], *ptr[i]);
  }

  char *names[] = {
    "Zara Ali",
    "Hina Ali",
    "Nuha Ali"
  };
  for(i = 0; i < MAX; i++) {
    printf("Value of names[%d] = %s\n", i, names[i]);
  }
  return 0;
}