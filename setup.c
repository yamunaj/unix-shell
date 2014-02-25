#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"

int fileInFlag = 0,fileOutFlag = 0;
char inFileName[BUFFERSIZE],outFileName[BUFFERSIZE];
//if pipe count is 0 number of command is 1
//if pipe count is 1, number of commands are 2
//int pipeCount = 0;

/*check if input is !n*/

int checkInput(char inputBuffer[])
{
	int i,n,n2,flag = 0;
	char input[MAX];
	// check if inputBuffer has < 2 or > 4 characters
	if (strlen(inputBuffer) < 2 || strlen(inputBuffer) > 4)
		return -1;
	//![0-9]
	n = inputBuffer[1] - '0';
	n2 = inputBuffer[2] - '0';

	memset(input,'\0',sizeof(input));

	//check if input is !12 
	//then return invalid input
	if(n2 >=0 && n2 <=9)
		return -1;
	if(size > 10)
	{

		if (n >= 0 && n <=9)
		{
			strcpy(input,inputBuffer);
			//get the nth history
			getHistory(input,n);
		}
		else 
			return -1;
	}
	else if(size <=10)
	{	
		if(n < size)
		{
			strcpy(input,inputBuffer);
			//get the nth history
			getHistory(input,n);

		}
		else
			return -1;
	}
	for( i = 2; i <  strlen(inputBuffer); i++)
	{
		switch(inputBuffer[i])
		{
			case '!' :
				break;
			case ' ':
			case '\t':
			case '\n':
				continue;
			case '\0':
				break;
			case '&' : // if background
#ifdef DEBUG
				printf("Size of input %d %s --[%c %c]--\n",strlen(input), input,
						input[strlen(input)-2],
						input[strlen(input)-1]);
#endif
				//append & at the end of input string
				if(strlen(input) < 80 )
				{
					input[strlen(input) - 1] = '&';
					input[strlen(input)] = '\n';
				}
				else
					return -1;
#ifdef DEBUG
				printf("Size of input %d %s --[%c %c]--\n",strlen(input), input,
						input[strlen(input)-2],
						input[strlen(input)-1]);
#endif

				break;
			default : 
				return -1;
		}
	}

	strcpy(inputBuffer,input);
	return 0;
}

/* The setup function below will not return any value, but it will just: read
   in the next command line; separate it into distinct arguments (using blanks as
   delimiters), and set the args array entries to point to the beginning of what
   will become null-terminated, C-style strings. */

int setup(char inputBuffer[], char *args[],int *background,int *pipeCount)
{

	int length, /* # of characters in the command line */
	    i,      /* loop index for accessing inputBuffer array */
	    start,  /* index where beginning of next command parameter is */
	    ct;     /* index of where to place the next parameter into args[] */
	char check[2] = "!!";
	int ret,validInput = 1;
	int inFileIndex = 0,outFileIndex = 0, flagIn = 0;
	int j = 0;

	//ct = j * (MAX / 2 + 1);
	ct = 0;

	memset(inputBuffer,'\0',sizeof(inputBuffer));
	*background = 0;
	/* read what the user enters on the command line */
	length = read(STDIN_FILENO,inputBuffer,MAX); 
	/* 0 is the system predefined file descriptor for stdin (standard input),
	   which is the user's screen in this case. inputBuffer by itself is the
	   same as &inputBuffer[0], i.e. the starting address of where to store
	   the command that is read, and length holds the number of characters
	   read in. inputBuffer is not a null terminated C-string. */

	start = -1;
	if (length == 0 )
		exit(0);            /* ^d was entered, end of user command stream */

	/* the signal interrupted the read system call */
	/* if the process is in the read() system call, read returns -1
	   However, if this occurs, errno is set to EINTR. We can check this  value
	   and disregard the -1 value */
	if ( (length < 0) && (errno != EINTR) )
	{
		perror("error reading the command");
		exit(-1);           /* terminate with error code of -1 */
	}

	// if input is > 80 characters then terminate the input string it with \n and return invalid
	if(strlen(inputBuffer) >= 80) 
	{
		inputBuffer[MAX - 1] = '\n'; 
		return -1;	
	}

	//if ^C is pressed it is not considered as an input
	if(signalhandle == 1)
		return -1;

	// check if input is !!
	if(strncmp(inputBuffer,check,2) == 0)
	{
		if(strlen(inputBuffer) > 3)
		{ 
			printf("\n Invalid input. \n Input must be !! ");
			return -1;
		}
		else
		{
			if(size != 11) // size < 10
			{
				//get the last entered command from history
				getHistory(inputBuffer,size-1);
			}
			else
			{
				// get the last entered command from history
				getHistory(inputBuffer,9);
			}
			length = strlen(inputBuffer);
		}
	}
	//check if input is !n
	else
		if(inputBuffer[0] == '!')
		{	
			//call check input to check if input is !n 
			ret = checkInput(inputBuffer);
			if(ret == -1 ) //invalid input
			{
				validInput = 0;
				printf("\n Invalid input. \n Input must be !<n> or !<n>&");
				return -1;
			}
			else
			{
				validInput = 1;
				length = strlen(inputBuffer);
			}
		}
	//printf("\nInside parse input string");
	if(validInput == 1)
	{
		//add new command to history
		if (strcmp(inputBuffer, "\n")) {
#ifdef DEBUG
			printf("not a slash n\n");
#endif
			//maintainHistory(inputBuffer,MAX * sizeof(char));
			maintainHistory(inputBuffer,strlen(inputBuffer));

		}

		/* To check if input has escape sequence 
		   (Eg : a\ s, a\&s, a\>s, a\<s or a\|s)*/

		int found=0, count = 0;
		for(i = 0; i<length;i++)
		{
			if(inputBuffer[i] == '\\')
				count ++;
		}
		int x=0;
		while(count > 0)
		{
			i=x;
			for(i; i<length; i++) 
			{
				if(inputBuffer[i] == '\\')
				{
					found = 1;
					continue;
				}
				if (found == 1) 
				{
					if (inputBuffer[i] == ' ') 
					{
						found = 2;
					}
					if(inputBuffer[i] == '>')
					{
						//replace > with '\a'
						inputBuffer[i] = GREATERTHAN;
					}
					if(inputBuffer[i] == '<')
					{
						//replace < with '\b'
						inputBuffer[i] = LESSTHAN;
					}
					if(inputBuffer[i] == '|')
					{
						//replace | with '\|'
						inputBuffer[i] = PIPE;
					}
					if(inputBuffer[i] == '&')
					{
						//replace & with '\r'
						inputBuffer[i] = AMPERSAND;
					}
					break;
				}
			}
			x=i+1;

			if (found == 1)
			{
				i--;
			}

			if (found) 
			{
				i += 1;
				for(; found && i<=length; i++)
				{
					if(i!=0)
						// removes the space
						//(Eg: a\ s is changed to a\s,
						//a\&s is changed to a&s,
						//a\<s is changed to a<s etc.
						inputBuffer[i-1] = inputBuffer[i];
				}

			}
			count --;
			found = 0;
		}
#ifdef DEBUG
		printf("input buffer is now %s\n Length : %d\n", inputBuffer,strlen(inputBuffer));
		fflush(stdout);
#endif
		//get string length of inputBuffer
		length = strlen(inputBuffer);
		//check for pipes in inputBuffer
		for( i=0;i< length;i++)
		{
			if(inputBuffer[i] == '|')
			{
				(*pipeCount)++;
				inputBuffer[i] = '\n';
			}
		}
#ifdef DEBUG
		printf("PipeCount : %d ",*pipeCount);
		printf("Input Buffer : %s",inputBuffer);
		fflush(stdout);
#endif
		while( j < (*pipeCount)+1)
		{
			for (i=0;i<length;i++)
			{ /* examine every character in the inputBuffer */

				switch (inputBuffer[i])
				{
					case '>' :
						//case when there is no space before >
						if(inputBuffer[i-1] != ' ')
						{
							if(start != -1)
							{
								args[ct] = &inputBuffer[start];    /* set up pointer */
#ifdef DEBUG
								printf("ct is set at %d, %s\n", ct,args[ct]);
								fflush(stdout);
#endif
								ct++;
							}
							inputBuffer[i] = '\0'; /* add a null char; make a C string */
							args[ct] = NULL;
							start = -1;
							fileOutFlag = 1;
							break;

						}
						else
						{
						fileOutFlag = 1;
						break;
						}
					case '<' :
						//when there is not space before <
						if(inputBuffer[i-1] != ' ')
						{
							if(start != -1)
							{
								args[ct] = &inputBuffer[start];    /* set up pointer */
#ifdef DEBUG
								printf("ct is set at %d, %s\n", ct,args[ct]);
								fflush(stdout);
#endif
								ct++;
							}
							inputBuffer[i] = '\0'; /* add a null char; make a C string */
							args[ct] = NULL;
							start = -1;
							flagIn = 1;
							fileInFlag = 1;
							break;

						}
						else
						{
						flagIn = 1;
						fileInFlag = 1;
						break;
						}

					case ' ':
					case '\t' :               /* argument separators */
						if((fileOutFlag == 1 || fileInFlag == 1) && flagIn == 1)
							break;
						else
						{
							if(start != -1)
							{
								args[ct] = &inputBuffer[start];    /* set up pointer */
#ifdef DEBUG
								printf("ct is set at %d, %s\n", ct,args[ct]);
								fflush(stdout);
#endif
								ct++;
							}
							inputBuffer[i] = '\0'; /* add a null char; make a C string */
							start = -1;
							break;
						}
					case '\n':                 /* should be the final char examined */
						if (start != -1 && inputBuffer[i-2] != '\0' )
						{
							args[ct] = &inputBuffer[start];   
#ifdef DEBUG
							printf("ct is set at %d, %s\n", ct,args[ct]);
							fflush(stdout);
#endif
							ct++;
						}	
						inputBuffer[i] = '\0';
						args[ct] = NULL; /* no more arguments to this command */
						j++;
						ct = (j * (MAX / 2 + 1)); //ct set to next command position
						start = -1;
						if(flagIn == 1)
							flagIn = 0;
						break;

					case '\\': // if escape sequence replace it with space
						inputBuffer[i] = ' ';
						break;
					default :             /* some other character */
						if(flagIn == 1 && inputBuffer[i] != '&' 
								&& inputBuffer[i] != '>')
						{
							if(fileOutFlag != 1)
							{
							inFileName[inFileIndex] = inputBuffer[i];
							inFileIndex++;
							break;
							}
						}
						if(fileOutFlag == 1 && inputBuffer[i] != '&' && inputBuffer[i] != '<')
						{
							outFileName[outFileIndex] = inputBuffer[i];
							outFileIndex++;
							break;

						}
						if (inputBuffer[i] == '&') // check if background
						{
							if (length > i)
							{
								if ((inputBuffer[i+1] == ' ') 
										|| (inputBuffer[i+1] == '\t')
										|| (inputBuffer[i+1] == '\n'))
								{
									if(j == *pipeCount)
									{
										*background  = 1;
										inputBuffer[i] = '\0';
										break;
									}
								}
							}
						}
						else
						{

							if (start == -1)
								start = i;
						}
				} /* end of switch */
			}    /* end of for */
			args[ct] = NULL; /* just in case the input line was > 80 */
		}
#ifdef DEBUG	
		j=0;
		while(j < (*pipeCount) + 1)
		{	
			i = (j * (MAX / 2 + 1));
			while(args[i])
			{
				printf("args %d = %s\n",i,args[i]);
				fflush(stdout);
				i++;
			}
			j++;
		}

		printf( "\n HAAAAAAAAAAAAAIIIIIIIII \n");
		printf("\n In file name : %s, Size : %d", inFileName,strlen(inFileName));
		fflush(stdout);
		printf( "\n HAAAAAAAAAAAAAIIIIIIIII \n");
		fflush(stdout);
		printf("\n Out file name : %s, Size : %d", outFileName,strlen(outFileName));
		&fflush(stdout);
#endif
		//replace special characters in inputBuffer with >, <, | and &
		for(i=0;i<length;i++)
		{
			if(inputBuffer[i] == GREATERTHAN)
				inputBuffer[i] = '>';
			else if(inputBuffer[i] == LESSTHAN)
				inputBuffer[i] = '<';
			else if(inputBuffer[i] == PIPE)
				inputBuffer[i] = '|';
			else if(inputBuffer[i] == AMPERSAND)
				inputBuffer[i] = '&';
		}
		//replace special characters in inFileName or outFileName with <, >, | and &
		for(i =0;i<strlen(outFileName);i++)
		{
			if(outFileName[i] == GREATERTHAN)
				outFileName[i] = '>';
			else if(outFileName[i] == LESSTHAN)
				outFileName[i] = '<';
			else if(outFileName[i] == PIPE)
				outFileName[i] = '|';
			else if(outFileName[i] == AMPERSAND)
				outFileName[i] = '&';
		}
		for(i =0;i<strlen(inFileName);i++)
		{
			if(inFileName[i] == '\a')
				inFileName[i] = '>';
			else if(inFileName[i] == '\b')
				inFileName[i] = '<';
			else if(inFileName[i] == '\v')
				inFileName[i] = '|';
			else if(inFileName[i] == '\'')
				inFileName[i] = '&';
		}

		return 0;
	}
	else
		signalhandle = 0;
	return -1;
} /* end of setup routine */
