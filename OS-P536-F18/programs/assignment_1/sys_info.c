#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

void main(int nargs, char *args[]) {

 pid_t pid;
 unsigned long int ret_val_fork; 
 int pipe1[2]; 
 int ret_val_pipe,i;
 char cmd_str[200];
 char msg[12]="Hello World!";

 /* Display error message if the total number of arguments is more than 2 */
 if(nargs > 2) 
	fprintf(stderr,"Too many arguments\nYou may enter maximum a single command you want to execute after ./sys_info\nFormat: ./sys_info </bin/><command>\nE.g: ./sys_info /bin/date or simply ./sys_info date\n");

 /* Continue to create pipe and call fork since the arguments are given in the expected format */
 else if(nargs==2) { 
	 int len=strlen(args[1]); 
	 fflush(stdout);
	 ret_val_pipe=pipe(pipe1);	/* Calling pipe() by passing integer array as argument to use it as pipe */
	 ret_val_fork=fork(); 		/* Child process is created here by calling Fork()*/
	 pid=getpid();
 
	/* Ensure the process ID and return value of fork are non-negative */
     	 if((pid>=0)&&(ret_val_fork>=0)) {

		/* Check if the return value of pipe is negative */
		if(ret_val_pipe<0) 
			fprintf(stderr, "Pipe conversion failed");
 	        
		else { 
			/* Execute this block if it is a Child process */
			if(ret_val_fork==0) {
				close(pipe1[1]);		/* Close the writing end of pipe before reading */
				read(pipe1[0],cmd_str,len+1);	/* Read the contents of pipe written by the Parent process */	
				close(pipe1[0]);		/* Close the reading end of pipe after reading contents */
				printf("Child PID = %d\n",pid);
				/* if the argument is directly given (say date) then prepend it with '/bin/' (to make /bin/date) before executing */
				if(cmd_str[0]!='/') {
					for(i=len-1;i>=0;i--) 
						cmd_str[i+5]=cmd_str[i];
					cmd_str[0]='/';cmd_str[1]='b';cmd_str[2]='i';cmd_str[3]='n';cmd_str[4]='/';
				}
				/* Incase of the command echo, pass Hello World! as parameter in addition to execl() */
				if(cmd_str[5]=='e'&&cmd_str[6]=='c'&&cmd_str[7]=='h'&&cmd_str[8]=='o') 
					execl(cmd_str,"",msg,NULL);
				
				else
					execl(cmd_str,"",NULL);	/* Executes the command along with parameters passed. Argument systax here might be different depending on Makefile*/
	        	}  
		
			else {	
				close(pipe1[0]);		 /* Close the reading end of pipe before writing */
				write(pipe1[1], args[1], len+1); /* Pass the argument command to be written */
				close(pipe1[1]);		 /* Close the writing end before Child process could read it */
				printf("Parent PID = %d\n",pid);
				wait(NULL);			 /* Wait for Child process to finish */
  			}   
		}
 	}

	/* Display error message if fork() failed */
 	else if(ret_val_fork<0) 
		fprintf(stderr,"Fork system call failed");
	
	/* Display error message if pid returned value is negative */
	else 
		fprintf(stderr,"Process could not be created");
  }
 
 /* Incase there is no command passed after ./sys_info */
 else 
	fprintf(stderr,"Please provide a command after ./sys_info\nFormat: ./sys_info </bin/><command>\nE.g: ./sys_info /bin/date or simply ./sys_info date\n"); 
 
 exit(0);
}
