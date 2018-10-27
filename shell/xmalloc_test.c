// xsh_xmemory.c

#include <xmalloc.h>
#include <xinu.h>

shellcmd xmalloc_test(int nargs, char *args[] )
{

xmalloc_init();	// init method
xheap_snapshot();

void* ptr = xmalloc(7);	// allocates memory of bytes passed as argument and returns pointer to the alocated memory
char *s = xheap_snapshot();
//printf("%s\n",s);
xfree(ptr);		// frees the allocated memory by passing specific pointer returned by xmalloc
xheap_snapshot();	// displays snapshot of memory occupancy

void* ptr1 = xmalloc(3);
xheap_snapshot();
//xfree(ptr1);
//xheap_snapshot();

void* ptr2 = xmalloc(2);
xheap_snapshot();
xfree(ptr2);
//xfree(ptr);
xheap_snapshot();
xfree(ptr1);
xheap_snapshot();

}
