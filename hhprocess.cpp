#include "hhprocess.h"
#include <errno.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif


#ifdef _WIN32
#define MSLEEP(x) Sleep(x)
#else
#define MSLEEP(x) usleep(x*1000)
#endif

int c_scselect( int sock);


/////////////////////////////////////////////////////
char ** get_wds(char *in)
{
    int	sz = strlen(in);
//	if(sz < min_wd_len) return(0);

    ++sz;
    sz *= sizeof(void*);

    char **wds = (char **)malloc(sz);
    memset(wds,0,sz);

    char *p,*pp;
    int	i=0;

    p = in;
    for(i = 0; (pp = strchr(p,' ')); ++i) {
        *pp = 0;
        ++pp;
        wds[i] = p;
        p = pp;
    }
    wds[i] = p;

    return(wds);
}

char ** get_cmd_par(char *in)
{
    int	sz = strlen(in);
//	if(sz < min_wd_len) return(0);

    ++sz;
    sz *= sizeof(void*);

    char **wds = (char **)malloc(sz);
    memset(wds,0,sz);

    char *p,*pp,*ppp;
    int	i=0;

    p = in;
    for(i = 0; (pp = strchr(p,' ')); ++i) {
        if( *p == '\'') {
            *p = 0;++p;
            if( ppp = strchr(p,'\'')) {
                *ppp = 0;++ppp;
                wds[i] = p;
                p = ppp;
            }

            continue;
        }

        *pp = 0;
        ++pp;
        wds[i] = p;
        p = pp;
    }
    wds[i] = p;

    return(wds);
}


hhProcess::hhProcess()
{

#ifdef _WIN32

    hChildStdoutRd = hChildStdinWr = (HANDLE)-1;
    hProcess = 0;

#else
    prc_id = 0;


#endif
}

hhProcess::~hhProcess()
{
    close();
}

void hhProcess::close()
{

#ifdef _WIN32

    if( hProcess) {
        TerminateProcess( hProcess,0);
        CloseHandle(hProcess);

        if( hChildStdinWr >= 0) {
            CloseHandle(hChildStdinWr);
        }
        if( hChildStdoutRd >= 0) {
            CloseHandle(hChildStdoutRd);
        }

        hChildStdoutRd = hChildStdinWr = (HANDLE)-1;
        hProcess = 0;
    }

#else

    if( prc_id ) {
        ::close(to_par[0]);
        ::close(to_par[1]);
        ::close(to_chil[0]);
        ::close(to_chil[1]);

        int r = kill(prc_id,SIGKILL);
        if( r < 0 ) {
//            fprintf(stderr,"!!!!!!!! kill errno %d <%s> prc_id %d\n",errno,strerror(errno),prc_id);fflush(stderr);
        }
        int status;
        waitpid(prc_id,&status,0);
        prc_id = 0;
    }


#endif
}

int hhProcess::isActiv()
{

#ifdef _WIN32

    if( hProcess >= 0) {
        int nExitCode = STILL_ACTIVE;
        GetExitCodeProcess(hProcess,(unsigned long*)&nExitCode);
        if( nExitCode == STILL_ACTIVE) return(1);
    }

    return (0);

#else
    if( !prc_id) return(0);

    errno = 0;
    int er = getpriority(PRIO_PROCESS,prc_id);
//    fprintf(stderr,"getpriority %d %d\n",prc_id,er,strerror(er));fflush(stderr);

    if( er == ESRCH || er == -1) {
        close();
        prc_id = 0;
        return (0);
    }

    MSLEEP(1);

    return(1);

#endif
}

int hhProcess::write(char *in)
{

#ifdef _WIN32
//	int ret = _write(wr, in, strlen(in));
//	_commit(wr);

    if( hChildStdinWr < 0 ) {
        return(0);
    }

    DWORD	rtn;

    WriteFile(
        hChildStdinWr,
        in,
        strlen(in),
        &rtn,
        NULL
    );

    FlushFileBuffers(hChildStdinWr);


    return( rtn);

#else
    if( !isActiv()) return(0);

    ssize_t sz = ::write(to_chil[1],in,strlen(in));
    if( sz <= 0 ) {
//            fprintf(stderr,"!!!!!!!! write sz %d errno %d <%s>\n",sz,errno,strerror(errno));fflush(stderr);
    }
    if(sz != strlen(in)) {
//           fprintf(stderr,"!!!!!!!! write sz %d len %d \n",sz,strlen(in));fflush(stderr);
           sz = -1;
    }

    fsync(to_chil[1]);

    return( sz);

#endif
}

int hhProcess::read(char *ou,int max_len)
{
    ou[0] = 0;

#ifdef _WIN32

//	return ( _read(rd, ou, max_len));

    if( hChildStdoutRd < 0 ) {
        ou[0] = 0;
        return(0);
    }

    DWORD	rtn;

    int r = ReadFile(
        hChildStdoutRd,
        ou,
        max_len,
        &rtn,
        0
    );

    if( !r) return -1;

    return( rtn);

#else
    if( !isActiv()) return(0);

    ssize_t sz = ::read(to_par[0],ou,max_len);
    return( sz);

#endif
}

int hhProcess::isRd()
{

#ifdef _WIN32

    return( c_scselect(hChildStdoutRd));

#else

    return( c_scselect(to_par[0]));

#endif
}


int hhProcess::readLine(char *ou,int max_len)
{
    char	c=0;
    int		i=0;

    ou[0] = 0;

//    MSLEEP(2);
    while( 1 ) {
        if( !isRd()) {
//            MSLEEP(1);
            if( !isActiv() && !isRd()){

                fprintf(stderr,"!!!-- prc OUT!\n");fflush(stderr);

                ou[i] = 0;
                return(-1);
            }
            continue;
        }

        if( !isRd()) continue;
        int r = read(&c,1);
//        fprintf(stderr," | r %x  c %x %c",r,c,c);fflush(stderr);

        if( r <= 0) {
//            fprintf(stderr,"readLine r %d errno %d <%s> isActiv() %d\n",r,errno,strerror(errno),isActiv());fflush(stderr);
        }

        if( r <= 0 && !isActiv()) return -1;
//		if( r == 0) break;

        if(c == 0x0d) continue;
        if(c == 0x0a) break;

        if(c == '\n') {	break;}

        ou[i++] = c;
        if(i >= (max_len-1)) break;
    }

    ou[i] = 0;

    return(i);
}

#define   HH_PIPE_BUFF_SIZE 512
#define   READ_FD 0
#define   WRITE_FD 1

#define   stdin_desc  0
#define   stdout_desc  1

/*
int hhProcess::start(char *fn,char *args)
{
    int ret = 0;

    close();

#ifdef _WIN32

    char	targs[15000];
    targs[0] = 0;
    if( args) strcpy(targs,args);
    char *wds[200];
    memset(wds,0,sizeof(wds));
    wds[0] = fn;

    char *p,*pp;
    int	i;

printf("::start >>\n");;

    p = targs;
    for( i = 1; (pp = strchr(p,' ')); ++i) {
        *pp = 0;
        ++pp;
        wds[i] = p;
        p = pp;
    }
    wds[i] = p;

fprintf(stderr,"::start 1\n");fflush(stderr);


//	HANDLE fdStdOut;
    HANDLE fdStdOutPipe[2];

//	HANDLE fdStdIn;
    HANDLE fdStdInPipe[2];


    SECURITY_ATTRIBUTES secAtt = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

    HANDLE tmpHandle;
//    if (in) {                   // stdin
        if (!CreatePipe(&fdStdInPipe[0], &tmpHandle, &secAtt, 1024 * 1024))	return -1;
        if (!DuplicateHandle(GetCurrentProcess(), tmpHandle, GetCurrentProcess(),
                             &fdStdInPipe[1], 0, FALSE, DUPLICATE_SAME_ACCESS))	return -2;

        CloseHandle(tmpHandle);

//    } else {                    // stdout or stderr
        if (!CreatePipe(&tmpHandle, &fdStdOutPipe[1], &secAtt, 1024 * 1024)) return -3;
        if (!DuplicateHandle(GetCurrentProcess(), tmpHandle, GetCurrentProcess(),
                             &fdStdOutPipe[0], 0, FALSE, DUPLICATE_SAME_ACCESS)) return-4;
//    }

        CloseHandle(tmpHandle);

        wr = fdStdInPipe[1];
        rd = fdStdOutPipe[0];


        PROCESS_INFORMATION pi;
        ZeroMemory( &pi, sizeof( pi ) );

        DWORD dwCreationFlags = CREATE_NO_WINDOW;
        STARTUPINFOA si = { sizeof( STARTUPINFO ), 0, 0, 0,
                                     (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                     (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                     0, 0, 0,
                                     STARTF_USESTDHANDLES,
                                     0, 0, 0,
                                     wr, rd , 0
        };
        if( CreateProcessA( 0, fn , 0, 0, TRUE, dwCreationFlags, 0, 0, &si, &pi ) ) {
            hProcess = pi.hProcess;
        }



#else


#endif

    return(ret);
}
*/

int hhProcess::start(char *fn)
{
    int ret = 0;

    close();

#ifdef _WIN32

//		hInputFile, hStdout;

    SECURITY_ATTRIBUTES saAttr;
//	BOOL fSuccess;

 // Set the bInheritHandle flag so pipe handles are inherited.

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

 // Get the handle to the current STDOUT.

//	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

 // Create a pipe for the child process's STDOUT.

    if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) return -1;

 // Ensure the read handle to the pipe for STDOUT is not inherited.

    SetHandleInformation( hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

 // Create a pipe for the child process's STDIN.

    if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))  return -2;

 // Ensure the write handle to the pipe for STDIN is not inherited.

    SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

 // Now create the child process.

//	TCHAR szCmdline[]=TEXT("child");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA siStartInfo;
    BOOL bFuncRetn = FALSE;

 // Set up members of the PROCESS_INFORMATION structure.

    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

 // Set up members of the STARTUPINFO structure.

    ZeroMemory( &siStartInfo, sizeof( STARTUPINFOA ) );
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hChildStdoutWr;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;


 // Create the child process.
    DWORD dwCreationFlags = CREATE_NO_WINDOW;
//	DWORD dwCreationFlags = 0;

    wr = hChildStdinWr;
    rd = hChildStdoutRd;

    bFuncRetn = CreateProcessA(NULL,
       fn,     // command line
       NULL,          // process security attributes
       NULL,          // primary thread security attributes
       TRUE,          // handles are inherited
       dwCreationFlags,             // creation flags
       NULL,          // use parent's environment
       NULL,          // use parent's current directory
       &siStartInfo,  // STARTUPINFO pointer
       &piProcInfo);  // receives PROCESS_INFORMATION

    if (bFuncRetn == 0)
       return -3;
    else
    {
        hProcess = piProcInfo.hProcess;
//		CloseHandle(piProcInfo.hProcess);
//		CloseHandle(piProcInfo.hThread);
       return bFuncRetn;
    }

#else

    pipe(to_par);
    pipe(to_chil);

    prc_id = fork();

    if( !prc_id) {
        ::close(0);
        dup2(to_chil[0],0);
        ::close(1);
        dup2(to_par[1],1);

        ::close(2);   //close stderr
//        ::dup2(to_par[1],2); //now put stderr in stdout

        ::close(to_par[0]);
        ::close(to_par[1]);
        ::close(to_chil[0]);
        ::close(to_chil[1]);

//        ::dup2(to_par[1],2); //now put stderr in stdout

        char b[1000];
        strcpy(b,fn);

        char **par = get_cmd_par(b);
        if( !par) _exit(0);

        char path[1000];
        strcpy(path,par[0]);

        char file_n[1000];
        char	*p = strrchr(path,'/');
        if( p) {
            strcpy(file_n,p + 1);
        }else {
            strcpy(file_n,path);
        }
        par[0] = file_n;

//		char *new_env[] = {NULL};

/*
        fprintf(stderr,"\npath: (%s)\n",path);fflush(stderr);
        for(int i=0; par[i]; ++i) {
            fprintf(stderr,"par %d: (%s)\n",i,par[i]);fflush(stderr);
        }
*/
        execvp(path,par);

    } else {
/*
        char b[1000];
        strcpy(b,fn);

        char **par = get_cmd_par(b);
        if( !par) goto end1;

        char path[1000];
        strcpy(path,par[0]);

        char file_n[1000];
        char	*p = strrchr(path,'/');
        if( p) {
            strcpy(file_n,p + 1);
        }else {
            strcpy(file_n,path);
        }
        par[0] = file_n;


        fprintf(stderr,"\npath: (%s)\n",path);fflush(stderr);
        for(int i=0; par[i]; ++i) {
            fprintf(stderr,"par %d: (%s)\n",i,par[i]);fflush(stderr);
        }

        free(par);
*/
    }

//	fprintf(strderr,"fork %d \n",prc_id);fflush(strderr);
end1:
#endif

    return(ret);
}



/*
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define BUFSIZE 4096

HANDLE hChildStdinRd, hChildStdinWr,
   hChildStdoutRd, hChildStdoutWr,
   hInputFile, hStdout;

BOOL CreateChildProcess(VOID);
VOID WriteToPipe(VOID);
VOID ReadFromPipe(VOID);
VOID ErrorExit(LPSTR);

int _tmain(int argc, TCHAR *argv[])
{
   SECURITY_ATTRIBUTES saAttr;
   BOOL fSuccess;

// Set the bInheritHandle flag so pipe handles are inherited.

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

// Get the handle to the current STDOUT.

   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

// Create a pipe for the child process's STDOUT.

   if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
      ErrorExit("Stdout pipe creation failed\n");

// Ensure the read handle to the pipe for STDOUT is not inherited.

   SetHandleInformation( hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

// Create a pipe for the child process's STDIN.

   if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
      ErrorExit("Stdin pipe creation failed\n");

// Ensure the write handle to the pipe for STDIN is not inherited.

   SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

// Now create the child process.

   fSuccess = CreateChildProcess();
   if (! fSuccess)
      ErrorExit("Create process failed with");

// Get a handle to the parent's input file.

   if (argc == 1)
      ErrorExit("Please specify an input file");

   printf( "\nContents of %s:\n\n", argv[1]);

   hInputFile = CreateFile(argv[1], GENERIC_READ, 0, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

   if (hInputFile == INVALID_HANDLE_VALUE)
      ErrorExit("CreateFile failed");

// Write to pipe that is the standard input for a child process.

   WriteToPipe();

// Read from pipe that is the standard output for child process.

   ReadFromPipe();

   return 0;
}

BOOL CreateChildProcess()
{
   TCHAR szCmdline[]=TEXT("child");
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bFuncRetn = FALSE;

// Set up members of the PROCESS_INFORMATION structure.

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

// Set up members of the STARTUPINFO structure.

   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = hChildStdoutWr;
   siStartInfo.hStdOutput = hChildStdoutWr;
   siStartInfo.hStdInput = hChildStdinRd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

// Create the child process.

   bFuncRetn = CreateProcess(NULL,
      szCmdline,     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      0,             // creation flags
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo);  // receives PROCESS_INFORMATION

   if (bFuncRetn == 0)
      ErrorExit("CreateProcess failed\n");
   else
   {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      return bFuncRetn;
   }
}

VOID WriteToPipe(VOID)
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];

// Read from a file and write its contents to a pipe.

   for (;;)
   {
      if (! ReadFile(hInputFile, chBuf, BUFSIZE, &dwRead, NULL) ||
         dwRead == 0) break;
      if (! WriteFile(hChildStdinWr, chBuf, dwRead,
         &dwWritten, NULL)) break;
   }

// Close the pipe handle so the child process stops reading.

   if (! CloseHandle(hChildStdinWr))
      ErrorExit("Close pipe failed\n");
}

VOID ReadFromPipe(VOID)
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];

// Close the write end of the pipe before reading from the
// read end of the pipe.

   if (!CloseHandle(hChildStdoutWr))
      ErrorExit("Closing handle failed");

// Read output from the child process, and write to parent's STDOUT.

   for (;;)
   {
      if( !ReadFile( hChildStdoutRd, chBuf, BUFSIZE, &dwRead,
         NULL) || dwRead == 0) break;
      if (! WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL))
         break;
   }
}

VOID ErrorExit (LPSTR lpszMessage)
{
   fprintf(stderr, "%s\n", lpszMessage);
   ExitProcess(0);
}
*/
