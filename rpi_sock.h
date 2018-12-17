#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

//#pragma warning( disable:4996 )

#ifdef _WIN32
#include "windows.h"
#include "winsock.h"

typedef int int32_t;
typedef int socklen_t;

#else
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif


int send_piece( int sock, char * buffer_in, size_t buflen_in);
int	recv_piece(int sc, char **b);
int	hh_connect (char *ser_url, struct sockaddr_in *srv_addr);
int	hh_get_sock_addr (char *ser_url, struct sockaddr_in *srv_addr);
int BindPassiveSocket(const int portNum, int *const boundSocket);
int	hh_closesocket (int sc);
int send_buf( int sock, char * buffer, size_t buflen);
