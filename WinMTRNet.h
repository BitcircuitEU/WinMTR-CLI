//*****************************************************************************
// FILE:            WinMTRNet.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************

#ifndef WINMTRNET_H_
#define WINMTRNET_H_


// Configuration for a trace run. Replaces the former tight coupling to the
// MFC dialog: the network core no longer reads settings from the GUI.
struct MTRConfig {
	int		pingsize;	// ICMP payload size in bytes
	double	interval;	// seconds between pings per hop
	bool	useDNS;		// resolve hop IPs to hostnames
	int		maxHops;	// maximum number of hops (TTL) to probe

	MTRConfig()
		: pingsize(DEFAULT_PING_SIZE)
		, interval(DEFAULT_INTERVAL)
		, useDNS(DEFAULT_DNS ? true : false)
		, maxHops(30)
	{}
};

typedef ip_option_information IPINFO, *PIPINFO, FAR *LPIPINFO;

#ifdef _WIN64
typedef icmp_echo_reply32 ICMPECHO, *PICMPECHO, FAR *LPICMPECHO;
#else
typedef icmp_echo_reply ICMPECHO, *PICMPECHO, FAR *LPICMPECHO;
#endif

#define ECHO_REPLY_TIMEOUT 5000

struct s_nethost {
  __int32 addr;		// IP as a decimal, big endian
  int xmit;			// number of PING packets sent
  int returned;		// number of ICMP echo replies received
  unsigned long total;	// total time
  int last;				// last time
  int best;				// best time
  int worst;			// worst time
  char name[255];
};

//*****************************************************************************
// CLASS:  WinMTRNet
//
//
//*****************************************************************************

class WinMTRNet {
	typedef HANDLE (WINAPI *LPFNICMPCREATEFILE)(VOID);
	typedef BOOL  (WINAPI *LPFNICMPCLOSEHANDLE)(HANDLE);
	typedef DWORD (WINAPI *LPFNICMPSENDECHO)(HANDLE, u_long, LPVOID, WORD, LPVOID, LPVOID, DWORD, DWORD);

public:

	WinMTRNet(const MTRConfig &cfg);
	~WinMTRNet();
	void	DoTrace(int address);
	void	ResetHops();
	void	StopTrace();

	int		GetAddr(int at);
	int		GetName(int at, char *n);
	int		GetBest(int at);
	int		GetWorst(int at);
	int		GetAvg(int at);
	int		GetPercent(int at);
	int		GetLast(int at);
	int		GetReturned(int at);
	int		GetXmit(int at);
	int		GetMax();

	void	SetAddr(int at, __int32 addr);
	void	SetName(int at, char *n);
	void	SetBest(int at, int current);
	void	SetWorst(int at, int current);
	void	SetLast(int at, int last);
	void	AddReturned(int at);
	void	AddXmit(int at);

	MTRConfig			config;
	__int32				last_remote_addr;
	bool				tracing;
	bool				initialized;
    HANDLE				hICMP;
	LPFNICMPCREATEFILE	lpfnIcmpCreateFile;
	LPFNICMPCLOSEHANDLE lpfnIcmpCloseHandle;
	LPFNICMPSENDECHO	lpfnIcmpSendEcho;
private:
	HINSTANCE			hICMP_DLL;

    struct s_nethost	host[MaxHost];
	HANDLE				ghMutex; 
};

#endif	// ifndef WINMTRNET_H_
