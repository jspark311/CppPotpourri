#include <stdlib.h>
#include <stdio.h>
#include "../StringBuilder/StringBuilder.h"


int main(void) {
  StringBuilder *heap_obj = new StringBuilder("This is datas we want to transfer.");
  StringBuilder stack_obj;
  
  
  
  stack_obj.concat("a test of the StringBuilder ");
  stack_obj.concat("used in stack. ");
  stack_obj.prepend("This is ");
  stack_obj.string();
  
  
  
  printf("Heap obj before culling:   %s\n", heap_obj->string());
  
  while (heap_obj->length() > 10) {
    heap_obj->cull(5);
    printf("Heap obj during culling:   %s\n", heap_obj->string());
  }
  
  printf("Heap obj after culling:   %s\n", heap_obj->string());
  
  heap_obj->prepend("Meaningless data ");
  heap_obj->concat(" And stuff tackt onto the end.");

  stack_obj.concatHandoff(heap_obj);

  delete heap_obj;
  
  stack_obj.split(" ");
  
  printf("Final Stack obj:          %s\n", stack_obj.string());

}

