#ifndef var_h
#define var_h

#define HISTORYSIZE 10
#define MAX 80
#define BUFFERSIZE 50
#define COMMANDCOUNT 6
#define GREATERTHAN '\a'
#define LESSTHAN '\b'
#define PIPE '\v'
#define AMPERSAND '\r'

extern int size;
extern int signalhandle;
extern char data[HISTORYSIZE][MAX + 3];
extern int fileInFlag,fileOutFlag,flagIn;
char inFileName[BUFFERSIZE],outFileName[BUFFERSIZE];

#endif
