//*****************************************************************************
// FILE:            WinMTRGlobal.h
//
//
// DESCRIPTION:
//   Global definitions shared by the WinMTR command line / service build.
//   This header is intentionally free of any MFC dependency so the core can
//   be compiled as a plain Win32 console application.
//
//*****************************************************************************

#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifndef  _WIN64
#define  _USE_32BIT_TIME_T
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tchar.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <process.h>
#include <stdio.h>
#include <io.h> 
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>

#define WINMTR_VERSION	"0.0.1"
#define WINMTR_LICENSE	"GPL - GNU Public License"
#define WINMTR_NAME		"WinMTR CLI"

#define DEFAULT_PING_SIZE	64
#define DEFAULT_INTERVAL	1.0
#define DEFAULT_MAX_LRU		128
#define DEFAULT_DNS			TRUE

#define SAVED_PINGS 100
#define MaxHost 256
//#define MaxSequence 65536
#define MaxSequence 32767
//#define MaxSequence 5

#define MAXPACKET 4096
#define MINPACKET 64

#define MaxTransit 4

 
#define ICMP_ECHO		8
#define ICMP_ECHOREPLY		0

#define ICMP_TSTAMP		13
#define ICMP_TSTAMPREPLY	14

#define ICMP_TIME_EXCEEDED	11

#define ICMP_HOST_UNREACHABLE 3

#define MAX_UNKNOWN_HOSTS 10

#define IP_HEADER_LENGTH   20


#define MTR_NR_COLS 9

const char MTR_COLS[ MTR_NR_COLS ][10] = {
		"Hostname",
		"Nr",
		"Loss %",
		"Sent",
		"Recv",
		"Best",
		"Avrg",
		"Worst",
		"Last"
};

const int MTR_COL_LENGTH[ MTR_NR_COLS ] = {
		190, 30, 50, 40, 40, 50, 50, 50, 50
};

int gettimeofday(struct timeval* tv, struct timezone *tz);

#endif // ifndef GLOBAL_H_
