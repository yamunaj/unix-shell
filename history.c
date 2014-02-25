#include <stdio.h>
#include <string.h>

#include "var.h"

char history[HISTORYSIZE][MAX];
char data[HISTORYSIZE][MAX + 3];
int in = 0, size = 0;

/* to print history data
The history is copied to an array data in proper order to be printed*/
void printHistory()
{
	int i;
	
	memset(data,'\0',sizeof(data));

	if(size ==0)
	{
		printf("\n No history data " );
	}
	if(size <= 10)
	{
		for ( i = 0; i < size ; i++)
			sprintf(data[i], "%d %s",i, history[i]);
			//strcpy(data[i],history[i]);
	}
	else
      	{
		int j = 0;
		i = (in) % HISTORYSIZE;

		for (j = 0; j < (size - 1); j++)	
		{
			sprintf(data[j], "%d %s",j, history[i]);

		//	strcpy(data[j], history[i]);
			i = (i+ 1) % HISTORYSIZE;
		}
	}
}

/* To maintain last 10 history commands
Maintained as a circular queue */

void maintainHistory(char input[],int inputSize)
{
	if(size <= 10) /* if there are only less than 10 commands*/
		{
			memcpy(history[in],input,inputSize);
			in = (in + 1) % HISTORYSIZE;
			size++;
		}
		else
		{
			memcpy(history[in],input,inputSize);
			in = (in + 1) % HISTORYSIZE;
		}

		


}

/* Get corresponding history command when user inputs !n
	(n can be 0-9)*/
void getHistory(char input[], int n)
{
	if(size <=10)
	{
		strcpy(input,history[n]);
	}
	else
	{
		strcpy(input,history[((n) + in) % HISTORYSIZE]);
	}
}
