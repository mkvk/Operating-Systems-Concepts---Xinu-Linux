// include/kv.h
#include<xinu.h>
#include<xmalloc.h>

//const int no_of_keys = 1024;
//const int grow = 128;

typedef struct {
char key[65]; 
int key_count;
int T;
int time;
} xcache_k; 

typedef struct {
char value[1025];
} xcache_v; 

char * kv_get(char*);
int kv_set(char*, char*);
bool kv_delete(char*);
void kv_reset();
int kv_init( ); // pass arguments

int get_cache_info(char*);
char** most_popular_keys(int);
