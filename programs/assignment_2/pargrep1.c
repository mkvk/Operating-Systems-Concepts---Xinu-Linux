
#include<pthread.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

char* grepstr;
FILE *fp;
long int lcount; int tc;
int limit;
long int linecount;
pthread_mutex_t thmutex;

void *grepcmd(void *f) {
	pthread_mutex_lock(&thmutex); tc++;
	char line[200];
	int llimit=(tc) * limit; 
	while( (fscanf( f, "%[^\n]\n", line )!= EOF )&&(lcount<=llimit) ) { 
                if(strstr(line, grepstr) != NULL ) {
                        printf("%s\n",line);
                }
		lcount++;
	} 
	pthread_mutex_unlock(&thmutex);
	pthread_exit((void *)f);
}

int main(int argc, char* argv[]) {

	char ch;
	int th,rvt,maxth;
	lcount=0;limit=0;linecount=0;th=0;rvt=0;maxth=0;tc=0;
		
	if(argc==5) {
		maxth=atoi(argv[2]);
		grepstr=argv[3];
		fp=fopen(argv[4],"r");
	}
	else {
		limit=10;
		fp=fopen(argv[2],"r");
		grepstr=argv[1];
		
	}
	for(ch=getc(fp);ch!=EOF;ch=getc(fp)) {
		if(ch=='\n') linecount++; }	

	if(argc==5) {
		limit=linecount/maxth;fp=fopen(argv[4],"r"); }
	else {
		maxth=linecount/limit;
		maxth++;
		fp=fopen(argv[2],"r");
	}

	pthread_t threads[maxth];
	pthread_attr_t attr;
	void *status;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
 	

	if(pthread_mutex_init(&thmutex,NULL)!=0) {
		printf("Mutex init failed\n");
		return 1;
	}


	for(th=1;th<=maxth;th++) {
		rvt =  pthread_create( &threads[th], &attr, grepcmd, (void *) fp ) ;
		if(rvt) {
			fprintf(stderr," Return code from pthread_create() is %d\n", rvt);
          		return -1;
		}
	}

	pthread_attr_destroy(&attr);
	for(th=1;th<=maxth;th++) {
		rvt = pthread_join(threads[th], &status );
		if(rvt) { return -1; }
	}
	pthread_mutex_destroy(&thmutex);
	pthread_exit(NULL);
	fclose(fp);
	return 0;
}
