#include <stdio.h>

double get_average(int *arr, int size);

int main(int argc, char **argv) {
  int balance[5] = {1000, 2, 3, 17, 50};
  double average;

  average = get_average(balance, 5);
  printf("average = %f\n", average);
  return 0;
}

double get_average(int *arr, int size) {
  int i, sum = 0;
  double avg;
  printf("size=%d\n", size);
 
  for (i = 0; i < size; ++i){
    sum += arr[i];
  }
 
  avg = (double)sum / size;
 
  return avg;
}