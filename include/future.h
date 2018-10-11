// future.h in include
#ifndef _FUTURE_H_
#define _FUTURE_H_  

#include <xinu.h>

typedef enum {
  FUTURE_EMPTY,
  FUTURE_WAITING,
  FUTURE_READY
} future_state_t;

typedef enum {
  FUTURE_EXCLUSIVE,
  FUTURE_SHARED,
  FUTURE_QUEUE
} future_mode_t;

typedef struct {
  pid32 pid;         // to save the process ID 
  struct node* next; // pointer to the next node structure to maintain link - single linked list
} node;

typedef struct {
  int value;
  future_state_t state;
  future_mode_t mode;
  pid32 pid;
  node *set_queue_head, *set_queue_tail;  // head and tail pointers for node structure - used for producers in set function
  node *get_queue_head, *get_queue_tail;  // head and tail pointers for node structure - used for consumers in get function
} future_t;

/* Interface for the Futures system calls */
future_t* future_alloc(future_mode_t mode);

syscall future_free(future_t*);
syscall future_get(future_t*, int*);
syscall future_set(future_t*, int);

uint future_prod(future_t* fut,int n);
uint future_cons(future_t* fut);
  
#endif /* _FUTURE_H_ */