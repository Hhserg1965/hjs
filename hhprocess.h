#ifndef HHPROCESS_H
#define HHPROCESS_H

//#include <QtGui>

#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#else

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>

#endif

#include <malloc.h>

class hhProcess
{

#ifdef _WIN32

	HANDLE 	wr;
	HANDLE 	rd;
	HANDLE	hProcess;

	HANDLE hChildStdinRd, hChildStdinWr,
		hChildStdoutRd, hChildStdoutWr;


#else
	__pid_t	prc_id;
	int to_par[2],to_chil[2];

#endif

public:
	hhProcess();
	~hhProcess();

	int	start(char *fn);
	void close();
	int isActiv();
    int isRd();

//	char getc();
//	putc(char c);

	int write(char *in);
	int read(char *ou,int max_len);
	int readLine(char *ou,int max_len);

};

#endif // HHPROCESS_H
