#include "rpi_sock.h"

int send_piece( int sock, char * buffer_in, size_t buflen_in)
{
	size_t					bytesWritten=0;
	int						writeResult;
	int						retval=0,done=0;
	int32_t					i32,*pi32;
	size_t 					buflen;


	if( !buffer_in ) {
	  return -1;
	}
	
    if( !buflen_in ) buflen_in = strlen(buffer_in);// + 1;

	char *buffer = (char *)malloc(buflen_in + sizeof(int32_t) + 10);
	
	if( !buffer ) {
	  return -2;
	}
	
	i32 = buflen_in;
	pi32 = (int32_t*)buffer;
	*pi32 = i32;
	memcpy(buffer+sizeof(int32_t),buffer_in,buflen_in);
	buflen = buflen_in + sizeof(int32_t);
	
	do {
		writeResult=send(sock,buffer+bytesWritten,buflen-bytesWritten,0);
		if(writeResult == -1)
			{
			if(errno==EINTR)
				writeResult=0;
			else
				{
				retval= -3;
				done=1;
				}
			}
		else
			{
			bytesWritten+=writeResult;
			if(writeResult==0)
				done=1;
			}
	} while(done==0);

	free(buffer);
	
	return retval;
}

int	recv_piece(int sc,char **b)
{
	int		ret,tot,sz;
	int32_t	len = 0;
	char	*p,*blk;

	*b = NULL;
	
	
	tot = 0;
	p = (char*)&len;
	sz = sizeof(int32_t);
	
	do {
		ret = recv(sc,p,sz - tot,0);
		if(ret == 0 ) return(-1);//closed
		
		if(ret == -1 ) {
		  if (errno!=EINTR){
			return(-2);
		  } else {
			continue;
		  }
		}
		
		p += ret;
		tot += ret;

	} while(tot < sz);
	
//syslog(LOG_LOCAL0|LOG_INFO,"recv_piece len = %d ",len);

	blk = (char *) malloc(len + 10);
	if(!blk) {
		return(-3);
	}

	tot = 0;
	p = (char*)blk;
	sz = len;

	if(len > 0)
	do {
		ret = recv (sc,p,sz - tot,0);
		
		if(ret == 0 ) { //closed
		  free(blk);
		  *b = NULL;
		  return(-4);
		}
		if(ret == -1 ) {
		  if (errno!=EINTR){
			free(blk);
			*b = NULL;
			return(-5);
		  } else {
			continue;
		  }
		}

		p += ret;
		tot += ret;

	} while(tot < sz);

	blk[len] = 0;
	*b = blk;
	
	return(len);
}

//************************************************************************/
int	hh_get_sock_addr (char *ser_url,struct sockaddr_in *srv_addr)
{
	  int port;
	  char	*p;
	  struct hostent *server;
	  
	  struct sockaddr_in serv_addr;
	  char	bser[200];
	
	  if(!ser_url) return(-1);
	  
	  strcpy(bser,ser_url);
	  
	  p = strchr(bser,':');
	  if(!p) return(-1);
	  
	  *p = 0;
	  ++p;
	  port = atoi(p);
	  
	  server = gethostbyname(bser);
	  if (server == NULL) 
	  {
		return (-1);
	  }
    
	  memset((char *) &serv_addr, 0, sizeof(serv_addr));
	  serv_addr.sin_family = AF_INET;
	  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	  serv_addr.sin_port = htons(port);
	  
	  memcpy(srv_addr,&serv_addr,sizeof(struct sockaddr_in));
  
	  return(0);
}

int	hh_connect (char *ser_url,struct sockaddr_in *srv_addr)
{
	int sock, port;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char	*p;
	char	bser[200];

	if(srv_addr) {
	  memcpy(&serv_addr,srv_addr,sizeof(struct sockaddr_in));
	}else {
	  if(!ser_url) return(-1);

	  strcpy(bser,ser_url);

	  p = strchr(bser,':');
	  if(!p) return(-1);

	  *p = 0;
	  ++p;
	  port = atoi(p);

	  server = gethostbyname(bser);
	  if (server == NULL)
	  {
		return (-1);
	  }

	  memset((char *) &serv_addr, 0, sizeof(serv_addr));
	  serv_addr.sin_family = AF_INET;
	  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	  serv_addr.sin_port = htons(port);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
	  return (-1);
	}

    int optval=1;
    size_t optlen=sizeof(int);
    ::setsockopt(sock, SOL_TCP, TCP_NODELAY, (const char *)&optval,optlen);

//    int flags = fcntl(sock,F_GETFL,0);
//    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

#ifdef _WIN32
	if (connect(sock, (const sockaddr * )&serv_addr, sizeof(serv_addr)) < 0)
#else
	if (connect(sock, (__CONST_SOCKADDR_ARG )&serv_addr, sizeof(serv_addr)) < 0)
#endif

	{
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	  return (-2);
	}

//	if(srv_addr) {
//	  memcpy(srv_addr,&serv_addr,sizeof(struct sockaddr_in));
//	}

    return(sock);
}

int	hh_closesocket (int sc)
{

#ifdef _WIN32
		return(closesocket(sc));
#else
		return(close(sc));
#endif

}

/*
int	hh_sld_client(struct sockaddr_in *srv_addr,char *in,int sz_in,char **out)
{
	int	sc;
	int	ret = 0;
  
	sc = rpi_connect (0,srv_addr);
	if(sc >= 0) {
	  
	  if( (ret = send_piece(sc,in,sz_in)) < 0 ) {
//		ret = -1;
		goto R_END;
	  }
		
	  if( (ret = recv_piece(sc,out)) < 0 ) {
		ret *= 10;
		goto R_END;
	  }
	  
R_END:

#ifdef _WIN32
		closesocket(sc);
#else
		close(sc);
#endif
	  return(ret);
	} else {
	  ret = 100 * ret;
	}
	
	return(ret);
}
*/

int BindPassiveSocket(const int portNum, int *const boundSocket)
{
	/* Note by A.B.: simplified this function code a bit */

	struct sockaddr_in			  sin;
	int							  newsock,optval;
	size_t						  optlen;


	/* clear the socket address structure */

	memset(&sin.sin_zero, 0, 8);

	/* set up the fields. Note htonX macros are important for
		portability */

	sin.sin_port=htons(portNum);
	sin.sin_family=AF_INET; /* Usage: AF_XXX here, PF_XXX in socket() */
	sin.sin_addr.s_addr=htonl(INADDR_ANY);

	if((newsock=socket(PF_INET, SOCK_STREAM, 0))<0)
		return -1;

	/* The SO_REUSEADDR socket option allows the kernel to re-bind
		local addresses without delay (for our purposes, it allows re-binding
		while the previous socket is in TIME_WAIT status, which lasts for
		two times the Maximum Segment Lifetime - anything from
		30 seconds to two minutes). It should be used with care as in
		general you don't want two processes sharing the same port. There are
		also dangers if a client tries to re-connect to the same port a
		previous server used within the 2*MSL window that TIME_WAIT provides.
		It's handy for us so the server can be restarted without having to
		wait for termination of the TIME_WAIT period. */

	optval=1;
	optlen=sizeof(int);
	setsockopt(newsock,SOL_SOCKET,SO_REUSEADDR,(const char *)&optval,optlen);
    optval=1;
    setsockopt(newsock, SOL_TCP, TCP_NODELAY, (const char *)&optval,optlen);

	/* bind to the requested port */

	if(bind(newsock,(struct sockaddr*)&sin,sizeof(struct sockaddr_in))<0)
		return -2;

	/* put the socket into passive mode so it is lisetning for connections */

	if(listen(newsock,SOMAXCONN)<0)
		return -3;

	*boundSocket=newsock;

	return 0;
}

int send_buf( int sock, char * buffer, size_t buflen)
{
    size_t					bytesWritten=0;
    int						writeResult;
    int						retval=0,done=0;
//    int32_t					i32,*pi32;


    if( !buffer ) {
      return -1;
    }

    if( !buflen) buflen = strlen(buffer);

    do {
//fprintf(stderr,"= send_buf 0 %d %d %016x %016X\n",buflen,buflen-bytesWritten,buffer+bytesWritten,buffer);fflush(stderr);
/*
        int error = 0;
        socklen_t len = sizeof (error);
        int retval_sc = getsockopt (sock, SOL_SOCKET, SO_ERROR, &error, &len);
        if(retval_sc != 0) {
            fprintf(stderr,"=!!!== retval_sc %d\n",retval_sc);fflush(stderr);
            retval= -3;
            break;
        }

        char c;
        ssize_t x = recv(sock, &c, 1, MSG_PEEK);
        fprintf(stderr,"=!!!== x %d\n",x);fflush(stderr);
*/

        writeResult=send(sock,buffer+bytesWritten,buflen-bytesWritten,0);
//fprintf(stderr,"= send_buf 0.1\n");fflush(stderr);

//fprintf(stderr,"= send_buf 1 r: %d\n",writeResult);fflush(stderr);
        if(writeResult == -1)
            {
            if(errno==EINTR)
                writeResult=0;
            else
                {
                retval= -3;
                done=1;
                }
            }
        else
            {
            bytesWritten += writeResult;
            if(writeResult == 0 || (buflen-bytesWritten) <= 0)
                done=1;
            }
    } while(done==0);

//fprintf(stderr,"= send_buf 1 retval: %d\n",retval);fflush(stderr);
    return retval;
}
