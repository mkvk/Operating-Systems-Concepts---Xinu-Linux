// Program to implement grep command using threads

#include<pthread.h>	// import pthread library
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

char* grepstr;		// grepstring to be matched
FILE *fp;		// pointer to the input text file
long int lcount;
int limit,tc;
long int linecount;
pthread_mutex_t thmutex;
size_t linesize = 0;
char* line=NULL;
ssize_t chars;

void *grepcmd(void *f) {

	pthread_mutex_lock(&thmutex);	//lock 
	tc++;
	int llimit=(tc) * limit;
	// get each line and check if the matching word is present
	while(((chars=getline(&line,&linesize,f)) != -1)&&(lcount<=llimit) ) { 
                if(strstr(line, grepstr) != NULL ) {
                     fwrite(line,chars,1,stdout);	//output the matching line
                }
		lcount++;
	} 
	pthread_mutex_unlock(&thmutex);	//unlock
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
	else if(argc==3) {
		limit=50;
		fp=fopen(argv[2],"r");
		grepstr=argv[1];
	}

	for(ch=getc(fp);ch!=EOF;ch=getc(fp)) 
		if(ch=='\n') linecount++; 	// count total no.of lines in text file	

	if(argc==5) {
		limit=linecount/maxth; 
		fp=fopen(argv[4],"r"); 
	}
	else if(argc==3) {
		maxth=linecount/limit;
		maxth++;
		fp=fopen(argv[2],"r");
	}

	pthread_t threads[maxth];	// declare required #threads as per the equation above
	pthread_attr_t attr;
	void *status;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
 	
	if(pthread_mutex_init(&thmutex,NULL)!=0) {	//initialize
		printf("Mutex init failed\n");
		return 1;
	}

	for(th=1;th<=maxth;th++) {	// invoke create function for each thread
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
