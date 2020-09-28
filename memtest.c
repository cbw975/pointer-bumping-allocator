#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main (int argc, char **argv){

  char* x = malloc(24);
  char* y = malloc(19);
  char* z = malloc(32);
  
  printf("x = %p\n", x);
  printf("y = %p\n", y);
  printf("z = %p\n", z);
  

  /*TEST: not copying when new_size == old_size (confirming for ≤)*/
  char* x_new = realloc(x, 24);
  printf("x_old = %p\n", x);
  printf("x_new = %p\n", x);
  if (x == x_new) {
  	printf("TEST_1 (without copying when new_size == old_size) PASSES\n");
  } else {
  	printf("TEST_1 (without copying when new_size == old_size) FAILS\n");
  }

  /*TEST: not copying when new_size < old_size (confirming for ≤)*/
  x_new = realloc(x, 22);
  printf("x_old = %p\n", x);
  printf("x_new = %p\n", x);
  if(x == x_new){
  	printf("TEST_2 (without copying when new_size < old_size) PASSES\n");
  } else {
  	printf("TEST_2 (without copying when new_size < old_size) FAILS\n");
  }

  /*TEST: copying when new_size > old_size*/
  char* y_new = realloc(y, 23);
  if (y == y_new) {
  	printf("TEST_3 (with copying when new_size > old_size) PASSES\n");
  } else {
  	printf("TEST_3 (with copying when new_size > old_size) FAILS\n");
  }

  /*TEST: reallocation copies contents correctly*/
  size_t* arr = malloc(13); // allocate memory for array of chars of length 9
  for(size_t ct = 0; ct < 13; ct++) { //fill in array
  	*(arr + ct) = ct;
  }
  size_t* arr_ = realloc(arr, 17); //realloc() to do memcpy()
  printf("TEST 4....\n");
  for(size_t i = 0; i < 13; i++){
    if((*arr_ + i) != (*arr + i)){
		  printf("TEST_4 (reallocation copies contents correctly) FAILS\n");
    }
  }
  printf("TEST 4....\n");
  // printf("TEST_4 (reallocation copies contents correctly) PASSES\n");

  /*TEST: alignment of malloced pointers*/
  char* ptr1 = malloc(111);
  char* ptr2 = malloc(222);
  char* ptr3 = malloc(12);

  if ((intptr_t)ptr1 % 16 == 0 && (intptr_t)ptr2 % 16 == 0 && (intptr_t)ptr3 % 16 == 0){
    printf("TEST_5 (malloc alignment) PASSES\n");
  }else{
    printf("TEST_5 (malloc alignment) FAILS\n");
  }

  /*TEST: alignment of realloced pointers*/
  char* ptr1_ = realloc(ptr1, 92);
  char* ptr2_ = realloc(ptr2, 141);
  char* ptr3_ = realloc(ptr3, 1);

  if ((intptr_t) ptr1_ % 16 == 0 && (intptr_t) ptr2_ % 16 == 0 && (intptr_t) ptr3_ % 16 == 0) {
    printf("TEST_6 (realloc alignment) PASSES\n");
  }else{
    printf("TEST_6 (realloc alignment) FAILS\n");
  }
}
