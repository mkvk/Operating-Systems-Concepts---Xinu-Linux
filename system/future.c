// future.c in system

#include <xinu.h>
#include <future.h>

future_t* future_alloc(future_mode_t mode) {	// to allocate memory to future of given mode
	future_t* f = (future_t*)getmem(sizeof(future_t));
	if((int*)f==-1) return NULL;
	f->mode = mode;
	f->state = FUTURE_EMPTY;
	if(mode==FUTURE_EXCLUSIVE) {
		f->get_queue_head->pid=-1;
		f->set_queue_head->pid=-1;
	}
	if(mode==FUTURE_SHARED) {
		f->get_queue_head=NULL;
		f->get_queue_tail=NULL;
		f->pid=-1;	// set to a value to block at a later point of time, if more then 1 producer exists for FUTURE_SHARED
	}
	if(mode==FUTURE_QUEUE) {
		f->get_queue_head=NULL;
		f->get_queue_tail=NULL;
		f->set_queue_head=NULL;
		f->set_queue_tail=NULL;
	}
	return f;
}

syscall future_free(future_t* f){	// free the memory allocated to future
	return freemem(f,sizeof(future_t)); 
}

syscall future_get(future_t* f, int* val) { // called by consumer to get the value from producer and store it in a pointer variable
	intmask mask;
	mask = disable();					// disable interrupt
	if(f->mode==FUTURE_EXCLUSIVE) { 	// when mode is exclusive
		if(f->state==FUTURE_EMPTY) {	// if consumer is called before producer
			f->state=FUTURE_WAITING;	// state is changed from empty to waiting
			f->pid=getpid();			// saving process ID to invoke it later
			suspend(getpid());			// suspend the consumer and resume after producer sets the value
			*val = f->value;			// the value set by producer is saved in pointer variable and printed
			f->get_queue_head->pid=getpid();
			restore(mask);				// restore the mask
			return OK;
		}
		else if(f->state==FUTURE_READY) {	// if consumer is called after producer
			*val = f->value;
			f->get_queue_head->pid=getpid();
			restore(mask);	
			return OK;
		}	
		else {
			f->get_queue_head->pid=getpid();	// to prevent consumers from executing, if there are more than 1 consumer
			restore(mask);	
			return SYSERR;
		}
	}
	else if(f->mode==FUTURE_SHARED) {		// if mode is shared
		node* new = (node*)getmem(sizeof(node));	// create a new node for each consumer, save it's PID and maintain linked list
		new->pid=getpid();
		new->next=NULL;
		if(f->state==FUTURE_EMPTY) {		// if state is empty - if consumer comes prior to producer and for the first consumer
			f->state=FUTURE_WAITING; 		// change state to waiting
			f->get_queue_head=new;			// since it is first node, head and tail point to it
			f->get_queue_tail=new;	
			suspend(getpid());				// supend the consumer until producer produces
		}
		else if(f->state==FUTURE_WAITING) {	// for other consumers ( excluding first one above )
			f->get_queue_tail->next=new;	// add new node to linked list
			f->get_queue_tail=new; 			// update tail pointer of consumers' queue
			suspend(getpid());				// suspend the consumer
		}
		*val = (f->value);					// after the consumer resumes set the value produced by producer
		restore(mask);	
		return OK;
	}	
	else if(f->mode==FUTURE_QUEUE){ 		// for mode queue 
		if(f->set_queue_head==NULL) {		// if the producer queue is empty
			node* new = (node*)getmem(sizeof(node));
			new->pid=getpid();
			new->next=NULL;
			if(f->get_queue_head==NULL) {
				f->state=FUTURE_WAITING; 
				f->get_queue_head=new;
				f->get_queue_tail=new;	
				suspend(getpid());			// suspend the consumer
				*val = (f->value);			// after consumer resumes, get the respective value of producer
			}
			else {
				f->state=FUTURE_WAITING;	// change state to waiting and add the new consumer node to the linked list
				f->get_queue_tail->next=new;
				f->get_queue_tail=new; 
				suspend(getpid());			// suspend the consumer
				*val = (f->value);			// after consumer resumes, get the respective value of producer
			}
		}
		else {
			*val = (f->value);				// if producer(s) are present in set queue
			f->pid=f->set_queue_head->pid;	// get PID of producer to resume
			f->set_queue_head=f->set_queue_head->next; // update the producer's linked list by moving producer's head pointer to the next one in list
			if(f->set_queue_head==NULL) f->set_queue_tail=NULL; // if there is only one in list, head and tail point to same one and both need to be updated
			resume(f->pid);					// resume the suspended producer
		}
		restore(mask);	
		return OK;
	}
	restore(mask);	
	return SYSERR;
}

syscall future_set(future_t* f, int i) { 	// called by producer to set the value and provide it to consumer
	intmask mask;
	mask = disable();						// disable interrupt
	f->value=i;								// value is assigned here
	if(f->mode==FUTURE_EXCLUSIVE) {
		if(f->state==FUTURE_WAITING) {		// if consumer is already suspended, change state and resume the consumer process
			f->state=FUTURE_READY;
			resume((f->pid));		
		}
		else f->state=FUTURE_READY;			// simply change the state to raedy if consumer is not already suspended
		f->set_queue_head->pid=getpid();	// to prevent producers from executing, if there are more than 1 producer
		restore(mask);	
		return OK;	
	}
	else if(f->mode==FUTURE_SHARED) {
		f->pid=getpid();
		f->state=FUTURE_READY;
		while(f->get_queue_head!=NULL) {	// traverse till all suspended consumers are resumed
			f->pid=f->get_queue_head->pid;
			f->get_queue_head=f->get_queue_head->next;
			resume(f->pid);
		}
		f->get_queue_tail=NULL;				// tail and head are set to NULL after all consumers are resumed
		restore(mask);	
		return OK;
	}	
	else if(f->mode==FUTURE_QUEUE){
		f->state=FUTURE_READY;
		if(f->get_queue_head!=NULL) {		// if there are consumers already suspended, resume the first consumer suspended by traversing list
			f->pid=f->get_queue_head->pid;
			f->get_queue_head=f->get_queue_head->next;
			if(f->get_queue_head==NULL) f->get_queue_tail=NULL;
			resume(f->pid);
		}
		else {
			node* new = (node*)getmem(sizeof(node));	//if there are no consumers suspended, create a producer node and append it to set list
			new->pid=getpid();
			new->next=NULL;
			if(f->set_queue_head!=NULL) {
				f->set_queue_tail->next=new;
				f->set_queue_tail=new;
			}
			else {
				f->set_queue_head=new;	// if the producer list is empty, set both head and tail to the newly created node
				f->set_queue_tail=new;
			}
			suspend(getpid());			// suspend the producer
			f->value=i;
		}
		restore(mask);	
		return OK;
	}	
	else {
		restore(mask);
		return SYSERR;
	}
}