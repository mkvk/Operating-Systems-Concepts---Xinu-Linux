// future_prodcons.c file

#include <future.h>
#include <xinu.h>

uint future_prod(future_t* fut,int n) {
  if((fut->pid > -1) && (fut->mode==1)) { // if the pid is different from what was manually assigned, indicates more than 1 producer for mode shared
    //printf("More than 1 producer\nOnly 1 producer allowed for FUTURE_SHARED\n");
    return SYSERR;
  }
  printf("Produced %d\n",n);
  future_set(fut, n);
  return OK;
}

uint future_cons(future_t* fut) {
  int i, status;
  status = (int)future_get(fut, &i);
  if (status < 1) {
    printf("future_get failed\n");
    return -1;
  }
  printf("Consumed %d\n", i);
  return OK;
}