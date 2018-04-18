#include <stdio.h>
#include <stdlib.h>

int main(){
  
  int fish = 0;
  if(--fish > 0){
    printf("dog\n");
    printf("inside %d\n", fish);
  }
    printf("outside: %d\n", fish);
  return 0;
}