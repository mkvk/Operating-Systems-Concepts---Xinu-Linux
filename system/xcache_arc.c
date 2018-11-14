// cache implemented using ARC

#include<xinu.h>
#include<xmalloc.h>
#include<kv.h>
#include<stdlib.h>

xcache_k* caches_k[25]; 	// pointer array to set of keys structure
xcache_v* caches_v[25]; 	// pointer array to set of values
xcache_k* g1[25];			// pointer array to set of ghost cache 1
xcache_k* g2[25];			// pointer array to set of ghost cache 2
char keys[512][65];			
int caches_count[25];
int tss, tg, tsg, tge, ev, nk, cs, P;

// 25*91 cache size - tunable parameters - can be adjusted depending on the input, to decrease error rate

char* kv_get(char* key) {	
	// get function returns the value associated with the key. If the key doesn't exist it returns NULL.
	int index = 0;
	int i = 0;
	char key_copy[65] = {'\0'};
	// calculate index of the hash table
	while(*key!=NULL) {			
		index += (int)*key;
		key_copy[i] = (char)*key;
		keys[tg][i] = (char)*key;
		key++;
		i++;
	}
	index %= 25;
	tg++;	// total gets
	int collisions = 91; 	// max #collisions allowed - whose index value is same for different keys
	while(collisions>0) {
		collisions--;
		if(strcmp(caches_k[index][collisions].key,key_copy)==0) { 
			tsg++;	// total # successful gets
			return caches_v[index][collisions].value;	// return the value found using given key
		}
	} 
	tge++;	// update error count , if we could not find the key in cache
	collisions = 46; // max. size of ghost list
	while(collisions>0) {
		collisions--;
		if(strcmp(g2[index][collisions].key,key_copy)==0) { 
			if(P<25) P += 1;	// increase T2 size by 1 - decreases size of T1
			//get victim from T1 that must be evicted
			int c = 91;
			int min = 10000; 
			int ind = c-1;
			while(c>0)
			{	// using LRU policy find the page that must be evicted from T1
				c--;
				if(caches_k[index][c].time>0 && caches_k[index][c].time < min && caches_k[index][c].T == 1) {
					min = caches_k[index][c].time;
					ind = c;
				}
			}
			char *k = caches_k[index][ind].key;
			kv_delete(k);	// delete the key of the victim obtained and use this cache for T2
			caches_k[index][ind].T = 2; 
			return NULL;	// since we could not find the key in cache
		}
	} 
	// repeat the above process by interchanging T1 and T2
	collisions = 46; 
	while(collisions>0) {
		collisions--;
		if(strcmp(g1[index][collisions].key,key_copy)==0) { 
			if(P>1) P -= 1;		// decrease T2 size by 1
			int c = 91;
			int min = 10000; 
			int ind = c-1;
			while(c>0)
			{
				c--;
				if(caches_k[index][c].time>0 && caches_k[index][c].time < min && caches_k[index][c].T == 2) {
					min = caches_k[index][c].time;
					ind = c;
				}
			}
			char *k = caches_k[index][ind].key;
			kv_delete(k);
			caches_k[index][ind].T = 1; 
			return NULL;
		}
	}

	return NULL;
}

int kv_set(char* key, char* val) {
	//Returns 1 on error if unable to store, 0 otherwise. 
	//Set operations either store a new key-value pair, or overwrite the previous value associated with the key.
	int index = 0; 
	int i = 0;
	char key_copy[65] = {'\0'};	// temporary array to copy the key
	// get the index of hash table where key has to be placed
	while(*key!=NULL) {
		index += (int)*key;
		key_copy[i] = (char)*key;
		key++;
		i++;
		if(i>64) return 1; // if key is of size greater than 64 bytes , ignore it
	}
	index %= 25; 
	// increment the cache size for monitoring operations
	cs += i*sizeof(char);
	int ky = i;
	i = caches_count[index]; 
	int n = i;
	while(n>-1) {	
		// if the key is already existing, then just re-write the value
		if(strcmp(key_copy,caches_k[index][n].key)==0) {
			caches_k[index][n].key_count += 1;
			caches_k[index][n].T = 2;
			caches_k[index][n].time = tss;
			int v=0;
			while(*val!=NULL) {
				caches_v[index][n].value[v] = (char)*val;
				val++;
				v++;
			}
			int m = v;
			for(;m<1025;m++) {
				caches_v[index][n].value[v] = '\0';
			}
			tss++;
			return 0;
		}
		n--;
	}
	// if key is not already present , add it to cache if space is present
	if(i<91) {	
		nk++;
		caches_count[index]++; 	
		while(ky>-1) { 
			caches_k[index][i].key[ky] = key_copy[ky]; 
			ky--; 
		}
		caches_k[index][i].time = tss;
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
	// if space is not present to add the key to cache , find a victim and evict it
	else { 		// eviction implemented using ARC
		ev++;
		int turn = -1;
		int x = 91;
		int tu = caches_count[index]%91;
		while(x>0) {
			x--;
			if(caches_k[index][tu].T==1) break;
			else {tu+=1; tu%=91;}
		}
		int g_turn = turn%46;
		int z = ky+1;
		caches_count[index]++;
		int b=0;
		// check if the key is found in ghost 1 cache
		if(caches_k[index][turn].T==1) {
			while(b<65) {
				g1[index][g_turn].key[b] = caches_k[index][turn].key[b]; 
				b++;
			} 	
		}
		// if the key is found in ghost 2 cache
		else if(caches_k[index][turn].T==2) {
			while(b<65) {
				g2[index][g_turn].key[b] = caches_k[index][turn].key[b]; 
				b++;
			} 	
		}
		// update the key in the victim's cache space 	
		while(ky>-1) { 
			caches_k[index][turn].key[ky] = key_copy[ky]; 
			ky--; 
		}
		for(;z<65;z++) {
				caches_k[index][turn].key[z] = '\0';
		}
		caches_k[index][turn].time = tss;
		// update the value of the key
		int v=0;
		while(*val!=NULL) {
			caches_v[index][turn].value[v] = (char)*val;
			val++;
			v++;
			if(v>1024) return 1; // if size of value is greater than 1KB ignore it
		}
		z = v;
		for(;z<1025;z++) {
			caches_v[index][turn].value[z] = '\0';
		}
		tss++; // total # successful sets
		return 0;
	} 
	return 1;
} 

bool kv_delete(char* key) {
	// Removes key from the cache. Returns false if key was not found, and true for successful deletion.
	int index = 0, i = 0;
	char key_copy[65] = {'\0'};
	// get index of hash table
	while(*key!=NULL) {			
		index += (int)*key;
		key_copy[i] = (char)*key;
		key++;
		i++;
	}
	index %= 25;
	// find the respective key and delete if found
	int collisions = 91; 
	while(collisions>0) {
		collisions--;
		if(strcmp(caches_k[index][collisions].key,key_copy)==0) { 
			caches_count[index]--;	// potential
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
			caches_k[index][collisions].time = 0;
			return TRUE;
		}
	} 
	return FALSE;
}

void kv_reset() {
// deletes and resets everything
	P = 0;
	int i=0;
	for(i=0;i<25;i++) {
		xfree(caches_k[i]); 
		xfree(caches_v[i]);
		xfree(g1[i]); 
		xfree(g2[i]); 
		caches_count[i]=0;	
	} 
	tg = 0; tsg = 0; tge = 0; tss = 0; ev = 0, nk = 0, cs = 0;
	xheap_snapshot();
}

int kv_init( ) { 
	// to initialize cache
	xmalloc_init();
	P = 0;
	int i=0;
	while(i<25) { 
		caches_count[i]=0; 
		caches_k[i] = (xcache_k*)xmalloc(91*65); 
		caches_v[i] = (xcache_v*)xmalloc(91*1025); 
		g1[i] = (xcache_k*)xmalloc(46*65); 
		g2[i] = (xcache_k*)xmalloc(46*65); 
		int j = 0;
		while(j<91) {
			caches_k[i][j].key_count = 0;
			caches_k[i][j].T = 1;
			caches_k[i][j].time = 0;
			j++;
		}
		i++; 
	} 
	tg = 0; tsg = 0; tge = 0; tss = 0; ev = 0, nk = 0, cs = 0;
	xheap_snapshot();
	return 0; 
}

int get_cache_info(char* kind) {
	// information about the cache performance
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
	// Returns a sorted array of the k most popular keys after removing duplicates
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