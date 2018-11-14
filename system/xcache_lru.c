// cache implemented using LRU
// this code is commented to avoid conflict with arc version while calling test file

/*
#include<xinu.h>
#include<xmalloc.h>
#include<kv.h>
#include<stdlib.h>

xcache_k* caches_k[25]; 
xcache_v* caches_v[25]; 
char keys[500][65];
int caches_count[25];
int tss, tg, tsg, tge, ev, nk, cs;

char* kv_get(char* key) {	

	int index = 0;
	int i = 0;
	char key_copy[65] = {'\0'};
	
	while(*key!=NULL) {			
		index += (int)*key;
		key_copy[i] = (char)*key;
		keys[tg][i] = (char)*key;
		key++;
		i++;
	}
	index %= 25;
	tg++;
	int collisions = 91; 
	while(collisions>0) {
		collisions--;
		if(strcmp(caches_k[index][collisions].key,key_copy)==0) { 
			tsg++;
			return caches_v[index][collisions].value;
		}
	} 
	tge++;
	return NULL;
}

int kv_set(char* key, char* val) {

	int index = 0; 
	int i = 0;
	char key_copy[65] = {'\0'};

	while(*key!=NULL) {
		index += (int)*key;
		key_copy[i] = (char)*key;
		key++;
		i++;
	}
	index %= 25; 

	cs += i*sizeof(char);
	int ky = i;
	i = caches_count[index]; 
	if(i<91) {  
		nk++;
		caches_count[index]++; 	
		while(ky>-1) { 
			caches_k[index][i].key[ky] = key_copy[ky]; 
			ky--; 
		}
		int v=0;
		while(*val!=NULL) {
			caches_v[index][i].value[v] = (char)*val;
			val++;
			v++;
		}
		cs += v*sizeof(char);
		tss++;
		return 0;
	}
	else { 		// eviction using LRU
		ev++;
		int turn = caches_count[index]%91;
		caches_count[index]++; 	
		int z = ky+1;
		while(ky>-1) { 
			caches_k[index][turn].key[ky] = key_copy[ky]; 
			ky--; 
		}
		for(;z<65;z++) {
				caches_k[index][turn].key[z] = '\0';
		}
		int v=0;
		while(*val!=NULL) {
			caches_v[index][turn].value[v] = (char)*val;
			val++;
			v++;
		}
		z = v;
		for(;z<1025;z++) {
			caches_v[index][turn].value[z] = '\0';
		}
		cs += v*sizeof(char);
		tss++;
		return 0;
	} 
	return 1;
} 

bool kv_delete(char* key) {
	
	int index = 0, i = 0;
	char key_copy[65] = {'\0'};
	
	while(*key!=NULL) {			
		index += (int)*key;
		key_copy[i] = (char)*key;
		key++;
		i++;
	}
	index %= 25;
	
	int collisions = 91; 
	while(collisions>0) {
		collisions--;
		if(strcmp(caches_k[index][collisions].key,key_copy)==0) { 
			caches_count[index]--;	
			nk--;
			int x=0;
			while(x<1025) {
				if(x<65 && caches_k[index][collisions].key[x] != '\0') {
					caches_k[index][collisions].key[x] = '\0';
					cs -= sizeof(char);
				}
				caches_v[index][collisions].value[x] = '\0';
				x++;
			}
			return TRUE;
		}
	} 
	return FALSE;
}

void kv_reset() {

	 int i=0;
	 for(i=0;i<25;i++) {
	 		xfree(caches_k[i]); 
	 		xfree(caches_v[i]);
	 	caches_count[i]=0;	
	 } 
	 tg = 0; tsg = 0; tge = 0; tss = 0; ev = 0, nk = 0, cs = 0;
	 xheap_snapshot();
}

int kv_init( ) { 
	
	xmalloc_init();
	int i=0;
	while(i<25) { 
		caches_count[i]=0; 
		caches_k[i] = (xcache_k*)xmalloc(91*65); 
		caches_v[i] = (xcache_v*)xmalloc(91*1025); 
		i++; 
	} 
	tg = 0; tsg = 0; tge = 0; tss = 0; ev = 0, nk = 0, cs = 0;
	xheap_snapshot();
	return 0; 
}

int get_cache_info(char* kind) {
	
	if(strcmp(kind,"total_hits")==0) {
		return  tsg;
	}
	else if(strcmp(kind,"total_accesses")==0) {
		return  tsg+tge;
	}
	else if(strcmp(kind,"total_set_success")==0) {
		return  tss;
	}
	else if(strcmp(kind,"cache_size")==0) {
		return  cs;
	}
	else if(strcmp(kind,"num_keys")==0) {
		return  nk;
	}
	else {
		return  ev;
	}
}

char** most_popular_keys(int k) { 
	
	int z = tg-1, i = 0;
	while(z) {
		if(keys[z][0]!='\0') {
			int y = z-1;
			while(y>-1) { 
				if(keys[y][0]!='\0' && strcmp(keys[z],keys[y])==0) {
					i=0; 
					while(i<65 && keys[y][i]!='\0') {
						keys[y][i] = '\0';
						i++;
					}
				}
				y--;
			}
		}
		z--;
	}
	z = tg-1;
	i = 0;
	printf("\nTop %d keys after removing duplicates are : \n",k);
	while(z) {
		if(z<0 || i==k) return keys[z+1];
		if(keys[z][0]!='\0') {
			printf("%s\n",keys[z]);
			i++;	
		}
		z--;
	}
	return keys[z+1];
}
*/
