#include <xinu.h>
#include <xmalloc.h>
#include <stdlib.h>

int powers2[] = {0,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192};
bpid32 pool_id[20];
int alloc_pid[20] = {0};
int frag_pid[20] = {0};
int alloc_buf[20] = {0};
bpid32 bufadd;
void* tests[100];
int tests_val[100];
int tests_pid[100];
int t=0;
bpid32 pid;

// init method for xmalloc
void xmalloc_init() {	
	int i=3, j=0;
	t=0;
	bufinit();	// initialising buffer pool table
	while( i < 14 ) {	// 2^14 = 8192, since 8192 is the maximum buffer sixe in Xinu
		pool_id[j] =  mkbufpool(powers2[i],BP_MAXN);	// allocate memory for buffer pool of different buffer sizes
		i++;
		j++;	
	}
}

// xmalloc which mimics malloc function
void* xmalloc(size_t ) {
	int i=3, frag=0, alloc=0; 
	void* ptr = NULL;
	while( i < 14 ) {
		if(size_t <= powers2[i]) { 
			ptr = getbuf(pool_id[i-3]);	// get buffer from the buffer pool
			if(ptr==SYSERR) { 		// if an error is returned by the getbuf method
				printf("Request cannot be accomodated at this moment\n"); return NULL;
			} 
			frag = powers2[i]-size_t;	// fragmented bytes
			alloc_pid[i-3] += size_t;	// allocated bytes
			frag_pid[i-3] += frag;
			alloc_buf[i-3] += 1;  		// allocated buffer(s)
			tests[t] = ptr; 
			tests_val[t] = size_t;
			tests_pid[t] = i-3;
			t++;
			break;
		}		
		i++;
	}
	if(size_t>8192) { 	// if requested memory is greater than 8192 bytes
		int c=10;
		while(c>-1) { 	// iterate from highest buffer size pool till the least until the requested memory could be allocated
			if((2048-alloc_buf[c])*powers2[c+3] >= size_t ) { 	// if requested memory could be allocated by using multiple buffers
	 		ptr = getbuf(pool_id[c]);
                        if(ptr==SYSERR) { 
				printf("Request cannot be accomodated at this moment\n"); return NULL;
			}
			int size=(int)size_t;
			int n=0; 
			while(1) { if(size>=powers2[c+3]) {size-=powers2[c+3];n++; } else {n++;break; } getbuf(pool_id[c]); } 
			frag = powers2[c+3]*n-size_t;
                        alloc_pid[c] += size_t;
                        frag_pid[c] += frag;
                        alloc_buf[c] += n;
                        tests[t] = ptr;
                        tests_val[t] = size_t;
                        tests_pid[t] = c;
			t++;
                        break;
        		}	
                	c--;
        	}
		// if none of the pool can accomodate return the error message
		if(c==-1) { printf("Memory full\nRequest cannot be accommodated at this moment\n"); return NULL; }
	}
	return ptr;
}

// free the memory allocated by getbuf by passing pointer as argument returned by getbuf
void xfree(void* ptr) {
	if(ptr==SYSERR || ptr==NULL) { printf("Request cannot be processed at this moment\n"); return NULL; }
	bufadd = ptr-sizeof(bpid32);	// get the buffer address which is stored in first 4 bytes
	pid = *(bpid32 *)bufadd;	// convert the address to pool ID
	int k=0,size=0,p=0,id=0;
	while(1) { 
		if(tests[k] == ptr) {	// get the respective address and memory size allocated
			size = tests_val[k];
			id = tests_pid[k]; 
			tests_val[k] = 0; 
			tests_pid[k] = 0; 
			tests[k] = NULL;
			break;
		}
		else k++;
	} 
	int sz=size;
	p=powers2[id+3];
        while(sz>=p) { sz -= p; alloc_buf[id] -= 1; }
                if(sz>0) { frag_pid[id] -= p-sz; alloc_buf[id] -= 1; } // decrease the fragmented size, since memory is freed
        alloc_pid[id] -= size;          // re-allocate the memory back to respective pool
	char* msg = freebuf(ptr);
	if(*msg==SYSERR) {
		printf("Memory cannot be freed at this moment\n");
	}
	freebuf(ptr); 
}

// snapshot to get the status of the memory allocated and available to be used
char* xheap_snapshot() { 
	int i = 3; char* s; 
	while( i < 14 ) {	
		printf("pool_id=%d, buffer_size=%d, total_buffers=%d, allocated_bytes=%d, allocated_buffers=%d, fragmented_bytes=%d\n", pool_id[i-3], powers2[i], BP_MAXN, alloc_pid[i-3], alloc_buf[i-3], frag_pid[i-3]); 	 
/*
	char st[10000] = {'\0'};
	strncat(st,"pool_id=",20);
	char* s1=itoa(pool_id[i-3]);
	strncat(st,s1,20);
	strncat(st,", buffer_size=",20);
        char* s2=itoa(powers2[i]);
        strncat(st,s2,20);
	strncat(st,", total_buffers=",20);
        char* s3=itoa(BP_MAXN);
        strncat(st,s3,20);
	strncat(st,", allocated_bytes=",20);
        char* s4=itoa(alloc_pid[i-3]);
        strncat(st,s4,20);
	strncat(st,", allocated_buffers=",20);
        char* s5=itoa(alloc_buf[i-3]);
        strncat(st,s5,20);
	strncat(st,", fragmented_bytes=",20);
        char* s6=itoa(frag_pid[i-3]);
        strncat(st,s6,20);	
	s=st;
*/	
	i++;
	}
	return s;
}
