#ifdef _WIN32
#include <windows.h>
#endif

#include "hh_shmem.h"
#include <stdlib.h>

//char hh_shmem_err_out[10000];
#ifdef _WIN32
//static char err_b[10000];
#endif


__cdecl int hh_shmem_create(char *fn,int sz,HH_SHMEM *shp)
{

#ifndef _WIN32

    shp->hMapFile =  shmget(atoi(fn),sz, IPC_CREAT | 0666);
	if (shp->hMapFile == -1)
	{
	   return -1;
	}

	shp->pBuf = (char *)shmat(shp->hMapFile, 0, 0);

	if (shp->pBuf == (char *)-1)
	{
	   return -2;
	}

#else

//	hh_shmem_err_out[0] = 0;

/*
	int i;

	wchar_t wb[200];

//	hh_shmem_err_out[0] = 0;

	memset(wb,0,sizeof(wb));

	for( i = 0; fn[i]; ++i)
		wb[i] = fn[i];
*/
	shp->hMapFile = CreateFileMappingA(
				  INVALID_HANDLE_VALUE,    // use paging file
				  NULL,                    // default security
				  PAGE_READWRITE,          // read/write access
				  0,                       // max. object size
				  sz,                // buffer size
				  fn);                 // name of mapping object

	if (shp->hMapFile == NULL)
	{
//	   sprintf(err_b,"Could not create file mapping object (%d) %x.\n",
//			  (int)GetLastError(),(int)GetLastError()); strcat(hh_shmem_err_out,err_b);
	   return -1;
	}
	shp->pBuf = (char *) MapViewOfFile(shp->hMapFile,   // handle to map object
						 FILE_MAP_ALL_ACCESS, // read/write permission
						 0,
						 0,
						 sz);
	if (shp->pBuf == NULL)
	{
//	   sprintf(err_b,"Could not map view of file (%d).\n",
//			  (int)GetLastError()); strcat(hh_shmem_err_out,err_b);
	   return -2;
	}

#endif

	shp->sz = sz;

	return (0);
}

__cdecl int hh_shmem_open(char *fn,int sz,HH_SHMEM *shp)
{

#ifndef _WIN32

    shp->hMapFile =  shmget(atoi(fn),sz,  0666);
	if (shp->hMapFile == -1)
	{
	   return -1;
	}

	shp->pBuf = (char *)shmat(shp->hMapFile, 0, 0);

	if (shp->pBuf == (char *)-1)
	{
	   return -2;
	}

#else

//	hh_shmem_err_out[0] = 0;

/*
	int i;
	wchar_t wb[200];

	hh_shmem_err_out[0] = 0;

	memset(wb,0,sizeof(wb));

	for(i = 0; fn[i]; ++i)
		wb[i] = fn[i];
*/
	shp->hMapFile = OpenFileMappingA(
					FILE_MAP_ALL_ACCESS,   // read/write access
					FALSE,                 // do not inherit the name
					fn);               // name of mapping object
	if (shp->hMapFile == NULL)
	{
//	   sprintf(err_b,"Could not create file mapping object (%d).\n",
//			  (int)GetLastError()); strcat(hh_shmem_err_out,err_b);
	   return -1;
	}
	shp->pBuf = (char *) MapViewOfFile(shp->hMapFile,   // handle to map object
						 FILE_MAP_ALL_ACCESS, // read/write permission
						 0,
						 0,
						 sz);
	if (shp->pBuf == NULL)
	{
//	   sprintf(err_b,"Could not map view of file (%d).\n",
//			  (int)GetLastError()); strcat(hh_shmem_err_out,err_b);
	   return -2;
	}

#endif

	shp->sz = sz;

	return (0);
}

__cdecl void hh_shmem_close(HH_SHMEM *shp)
{
#ifndef _WIN32

    if (shp->sz > 0 && shp->pBuf)
	{
	   shmdt(shp->pBuf);

	}

#else
    if (shp->sz > 0 && shp->pBuf)
	{
		UnmapViewOfFile((void* )shp->pBuf);
		CloseHandle(shp->hMapFile);

	}
#endif

	shp->hMapFile = 0;
	shp->pBuf = 0;
	shp->sz = 0;
}

