#ifndef HH_SHMEM_H
#define HH_SHMEM_H

#define HH_SHMEM_MAX_SIZE 20000000

#ifdef _WIN32
//#include <windows.h>
#include <stdio.h>
#include <conio.h>

typedef void *HANDLE;

#else

//#include <QtGui>

#include <sys/ipc.h>
#include <sys/shm.h>

#define HANDLE int
#define LPTSTR char *

#endif

typedef struct {
	HANDLE	hMapFile;
	char *	pBuf;
	int		sz;

}HH_SHMEM;

typedef struct {
	char	set;
	char	done;

	int		sz;

	int		pix_fmt;

	int		w;
	int		h;
	int		linesize;

	float	pos;

	char	add[2];
	char	add2[20];

}HH_SHMEM_MASTER_BLOK;

#define __cdecl

#ifdef __cplusplus
extern "C"  __cdecl int hh_shmem_create(char *fn,int sz,HH_SHMEM *shp);
extern "C"  __cdecl int hh_shmem_open(char *fn,int sz,HH_SHMEM *shp);
extern "C"  __cdecl void hh_shmem_close(HH_SHMEM *shp);
#else
__cdecl int hh_shmem_create(char *fn,int sz,HH_SHMEM *shp);
__cdecl int hh_shmem_open(char *fn,int sz,HH_SHMEM *shp);
__cdecl void hh_shmem_close(HH_SHMEM *shp);
#endif

#endif // HH_SHMEM_H
