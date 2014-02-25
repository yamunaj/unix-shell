#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <wait.h>

#include "shell.h"
#include "history.h"

char buffer[BUFFERSIZE];
int signalhandle = 0;

/* the ^D signal handling function */
void handle_SIGHUP()
{
	exit(0);
}

/* the ^C signal handling function */
void handle_SIGINT()
{
	//printf("\nInside ^c handle \n");
	//	fflush(stdout);
	char msg[1000];
	int i;
	signalhandle = 1;
	write(STDOUT_FILENO, buffer, strlen(buffer));
	printHistory();	
	for( i=0; i < HISTORYSIZE; i++)
		write(STDOUT_FILENO, data[i], MAX);					
}

/*Set the signals generated when ^C and ^D are entered
  ^C generates signal SIGINT
  ^D genrates signal SIGQUIT*/
void setup_term(void)
{

	struct termios old_termios, new_termios;
	tcgetattr(0,&old_termios);

	new_termios = old_termios;
	new_termios.c_cc[VINTR]  = 3; // ^C
	new_termios.c_cc[VQUIT] = 4; // ^D
	tcsetattr(0,TCSANOW,&new_termios);

}
int main()
{
	//input string
	char input[MAX];
	//command parsed
	char *args[COMMANDCOUNT * (MAX / 2 + 1)];
	//to check if the process has to run in background
	int background = 0;
	//size of input string
	int inputSize;
	//to store pid's after calling fork()
	int pid[6] = {0,0,0,0,0,0};
	//Number of commands / forks() called
	int pidCount = 0;
	int ret,status = 0;
	//descriptors for inFile and outFile
	int inDescriptor,outDescriptor;
	int j = 0;
	//Number of pipes in input string
	int pipeCount =0;

	//mask set
	sigset_t newmask,oldmask;

	/*call setup_term to set the signals generated when
	  ^C and ^D is pressed*/
	setup_term();

	/* set up the signal handler for signal SIGQUIT */
	struct sigaction handle;
	handle.sa_handler = handle_SIGHUP;
	handle.sa_flags = 0;
	sigemptyset(&handle.sa_mask);
	//catch ^D signal and handle it
	sigaction(SIGQUIT , &handle, NULL);

	/* set up the signal handler for signal SIGINT */
	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	/*generate the output message */
	strcpy(buffer, "\nCaught Control C\n");

	printf("\n\x1B[31mCommand ->\x1B[37m ");
	fflush(stdin);
	fflush(stdout);
	//catch ^C signal and handle it
	sigaction(SIGINT, &handler, NULL);

	sigemptyset(&newmask);
	sigaddset(&newmask, SIGINT);

	while(1)
	{
		inputSize = sizeof(input);
		//setting input buffer to \0
		memset(input,'\0',inputSize);
		memset(outFileName,'\0',sizeof(outFileName));
		memset(inFileName,'\0',sizeof(inFileName));
#ifdef DEBUG
		//	printf("\n Main : In file name : %s, Size : %d", inFileName,strlen(inFileName));
		//	printf("\n Main : Out file name : %s, Size : %d", outFileName,strlen(outFileName));
#endif

		pipeCount = 0;
		background = 0;
		fileOutFlag = 0;
		fileInFlag = 0;
		//call setup function to parse input
		ret = setup(input, args, &background,&pipeCount);
		signalhandle = 0;
		//file descriptors for pipe
		int pipes[5 * 2];
		int k;
#ifdef DEBUG
		printf("\nRET : %d\n",ret);
		fflush(stdout);
#endif

		//creating 5 pipes
		pipe(pipes);
		pipe(pipes + 2);
		pipe(pipes + 4);
		pipe(pipes + 6);
		pipe(pipes + 8);

		//open out file
		if(fileOutFlag == 1)
		{
			outDescriptor = 
			//	fopen(outFileName,"wb");
				open(outFileName,O_WRONLY | O_TRUNC | O_CREAT,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		}
		//open in file
		if(fileInFlag == 1)
		{
			inDescriptor = 	open(inFileName,O_RDONLY);
				//fopen(inFileName,"rb");

		}
		if(ret == 0)
		{
			j = 0;
			pidCount = 0;
			for(k=0;k<6;k++)
				pid[k] = 0;
			while ( j < pipeCount + 1)
			{
				//create child process
				pid[j] = fork();
				pidCount++;
#ifdef DEBUG
				//printf("args[0] = %s\n", args[0]);
				printf("pidCount of fork%d: %d",j,pidCount);
				fflush(stdout);
#endif

				//block ^C signal if child is running
				sigprocmask(SIG_BLOCK, &newmask, &oldmask);
				if(pid[j] == 0)
				{
					//execute command in child process
#ifdef DEBUG
					printf("\nargs[0] = %s,fork%d", args[0],j);
					printf("\n Inside execute..fork%d\n",j);
					printf("\noutflag : %d, inflag : %d\n",fileOutFlag,fileInFlag);
					fflush(stdout);
#endif
					//redirecting standard output
					if(fileOutFlag == 1 )/*&& pipeCount == 0)*/
					{
						dup2(outDescriptor,STDOUT_FILENO);
						close(outDescriptor);
					}
					//redirecting standard input
					if(fileInFlag == 1 )/*&& pipeCount == 0)*/
					{
						dup2(inDescriptor,STDIN_FILENO);
						close(inDescriptor);
					}
					//if first command
					if(j == 0 && pipeCount > 0)
					{
						// redirecting stdout to write end of pipe
						dup2(pipes[j*2 + 1],1);

					}
					//if command between to pipes(|)
					if(j > 0 && j < pipeCount && pipeCount > 0)
					{
						//redirecting stdin and stdout to read and write end of pipe
						dup2(pipes[j*2 - 2],0);
						dup2(pipes[j*2 + 1],1);
					}
					//if last command
					if(j == pipeCount && pipeCount > 0)
					{
						//redirecting stdin to read end of pipe
						dup2(pipes[j*2 - 2],0);

					}
					//close all unused pipes
					for(k = 0;k < 10; k++)
					{
						close(pipes[k]);
					}
					//execute command
					int retval =
						execvp(args[(j * (MAX / 2 + 1))] ,&(args[(j * (MAX / 2 + 1))]));

					// If execvp returns, it means execution of that command failed.
					// So we manually terminate the child (which wakes up the parent).
					if (retval) {
						printf("\nInvalid command %s", args[0]);
						exit(0);
					}
					exit(0);

				}
				j++;
			}
			for(k = 0;k < 10; k++)
			{
				close(pipes[k]);
			}
#ifdef DEBUG
			printf("\nBackground : %d\n\n",background);
#endif


			//if child process is not running in background parent waits
			if(background != 1) 
			{
				status = 0;
#ifdef DEBUG
				printf("\nParent waiting..\n\n");
#endif
				for( k = 0; k < pidCount; k++)
				{
					waitpid(pid[k],&status,0);

				}

#ifdef DEBUG
				//wait(&status);
				//waitid(P_ALL,pid[pidCount - 1],NULL ,WEXITED | WSTOPPED);
				//waitpid(pid[k],&status,WEXITED | WSTOPPED);
				//sigprocmask(SIG_SETMASK, &oldmask, NULL);
				printf("\n%s: done\n", input);
				fflush(stdin);
				fflush(stdout);

#endif

			}
			background = 0;
			//unblock ^C signal
			sigprocmask(SIG_UNBLOCK, &newmask, &oldmask);
			printf("\n\x1B[31mCommand ->\x1B[37m ");
			fflush(stdin);
			fflush(stdout);
		}
		else
		{
			background = 0;
			printf("\n\x1B[31mCommand ->\x1B[37m ");
			fflush(stdin);
			fflush(stdout);
		}

	}
	return 0;
}

