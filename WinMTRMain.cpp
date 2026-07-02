//*****************************************************************************
// FILE:            WinMTRMain.cpp
//
//
// DESCRIPTION:
//   Command line entry point for WinMTR. Runs a continuous MTR-style trace,
//   appends periodic snapshots to a CSV log and writes a final summary on
//   shutdown. Designed to run headless 24/7, e.g. as an nssm service.
//
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRNet.h"
#include "WinMTRReport.h"
#include <string>

//-----------------------------------------------------------------------------
// Shared state used by the console control handler.
//-----------------------------------------------------------------------------
static WinMTRNet		*g_net		= NULL;
static volatile bool	 g_running	= true;

//*****************************************************************************
// CtrlHandler
//
// nssm stops a service by sending Ctrl+C to the console. We catch that (and
// close/logoff/shutdown) to stop the trace cleanly and let main() write the
// final report before exiting.
//*****************************************************************************
static BOOL WINAPI CtrlHandler(DWORD ctrlType)
{
	switch(ctrlType) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			g_running = false;
			if(g_net) g_net->StopTrace();
			return TRUE;
		default:
			return FALSE;
	}
}

//*****************************************************************************
// TraceRunner
//
// DoTrace() blocks until the trace is stopped, so it runs on its own thread.
//*****************************************************************************
struct trace_ctx {
	WinMTRNet	*net;
	int			 addr;
};

static unsigned __stdcall TraceRunner(void *p)
{
	trace_ctx *c = (trace_ctx*)p;
	c->net->DoTrace(c->addr);
	return 0;
}

//*****************************************************************************
// PrintUsage
//*****************************************************************************
static void PrintUsage(const char *prog)
{
	printf(
		"%s v%s\n"
		"\n"
		"Usage: %s [options] <host>\n"
		"\n"
		"Options:\n"
		"  -i, --interval <sec>    Seconds between pings per hop (default %.1f)\n"
		"  -s, --size <bytes>      ICMP payload size (default %d)\n"
		"  -m, --max-hops <n>      Maximum number of hops (default 30)\n"
		"  -n, --numeric           Do not resolve hop IPs to hostnames\n"
		"  -l, --log <file>        Live log file, human-readable, one line per hop\n"
		"                          appended per interval (tailable with LogExpert)\n"
		"  -o, --output <file>     CSV log file, one snapshot appended per interval\n"
		"      --log-interval <s>  Seconds between log/CSV snapshots (default 60)\n"
		"      --text <file>       Write final ASCII summary to file on exit\n"
		"      --html <file>       Write final HTML summary to file on exit\n"
		"  -q, --quiet             Do not print the live table to the console\n"
		"  -h, --help              Show this help\n"
		"\n"
		"The trace runs until stopped (Ctrl+C or service stop). On stop the final\n"
		"summary files are written.\n"
		"\n"
		"If no output option (-l/-o/--text/--html) is given, default files are\n"
		"created in the current directory: <host>_<timestamp>.log (live) plus\n"
		"<host>_<timestamp>_results.txt/.html on stop.\n",
		WINMTR_NAME, WINMTR_VERSION, prog,
		(double)DEFAULT_INTERVAL, DEFAULT_PING_SIZE);
}

//*****************************************************************************
// ResolveHost
//
// Resolves a host name or dotted IP to a network-byte-order address, as
// expected by IcmpSendEcho. Requires WSAStartup to have run already.
//*****************************************************************************
static int ResolveHost(const char *name, int *out_addr)
{
	int isIP = 1;
	for(const char *t = name; *t; t++) {
		if(!isdigit((unsigned char)*t) && *t != '.') { isIP = 0; break; }
	}

	if(isIP) {
		*out_addr = (int)inet_addr(name);
		return (*out_addr != (int)INADDR_NONE) ? 1 : 0;
	}

	struct hostent *host = gethostbyname(name);
	if(host == NULL || host->h_addr == NULL) return 0;
	*out_addr = *(int*)host->h_addr;
	return 1;
}

//*****************************************************************************
// FileExists
//*****************************************************************************
static bool FileExists(const char *path)
{
	struct _stat st;
	return _stat(path, &st) == 0;
}

//*****************************************************************************
// DefaultBaseName
//
// Builds a file-system-safe base name from the target host and the current
// time, e.g. "google.de_20260702_083455". Used to auto-generate output file
// names when the user did not specify any output option.
//*****************************************************************************
static std::string DefaultBaseName(const char *host)
{
	std::string safe;
	for(const char *p = host; *p; p++) {
		char c = *p;
		if(isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_')
			safe += c;
		else
			safe += '_';
	}

	char ts[32];
	time_t now = time(NULL);
	struct tm lt;
	localtime_s(&lt, &now);
	strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &lt);

	return safe + "_" + ts;
}

//*****************************************************************************
// main
//*****************************************************************************
int main(int argc, char **argv)
{
	const char	*host		= NULL;
	const char	*csvPath	= NULL;
	const char	*logPath	= NULL;
	const char	*textPath	= NULL;
	const char	*htmlPath	= NULL;
	int			 logInterval= 60;
	bool		 quiet		= false;
	MTRConfig	 cfg;

	for(int i = 1; i < argc; i++) {
		const char *a = argv[i];

		if(!strcmp(a, "-h") || !strcmp(a, "--help")) {
			PrintUsage(argv[0]);
			return 0;
		} else if(!strcmp(a, "-n") || !strcmp(a, "--numeric")) {
			cfg.useDNS = false;
		} else if(!strcmp(a, "-q") || !strcmp(a, "--quiet")) {
			quiet = true;
		} else if(!strcmp(a, "-i") || !strcmp(a, "--interval")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			cfg.interval = atof(argv[i]);
		} else if(!strcmp(a, "-s") || !strcmp(a, "--size")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			cfg.pingsize = atoi(argv[i]);
		} else if(!strcmp(a, "-m") || !strcmp(a, "--max-hops")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			cfg.maxHops = atoi(argv[i]);
		} else if(!strcmp(a, "-o") || !strcmp(a, "--output")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			csvPath = argv[i];
		} else if(!strcmp(a, "-l") || !strcmp(a, "--log")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			logPath = argv[i];
		} else if(!strcmp(a, "--log-interval")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			logInterval = atoi(argv[i]);
		} else if(!strcmp(a, "--text")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			textPath = argv[i];
		} else if(!strcmp(a, "--html")) {
			if(++i >= argc) { fprintf(stderr, "Missing value for %s\n", a); return 1; }
			htmlPath = argv[i];
		} else if(a[0] == '-') {
			fprintf(stderr, "Unknown option: %s\n", a);
			return 1;
		} else {
			host = a;
		}
	}

	if(host == NULL) {
		fprintf(stderr, "No host specified.\n\n");
		PrintUsage(argv[0]);
		return 1;
	}

	if(logInterval < 1) logInterval = 1;

	// If the user did not configure any output, auto-generate default files in
	// the current working directory: a live .log plus result files on stop.
	std::string defLog, defText, defHtml;
	if(!csvPath && !logPath && !textPath && !htmlPath) {
		std::string base = DefaultBaseName(host);
		defLog  = base + ".log";
		defText = base + "_results.txt";
		defHtml = base + "_results.html";
		logPath  = defLog.c_str();
		textPath = defText.c_str();
		htmlPath = defHtml.c_str();
	}

	// WinMTRNet ctor performs WSAStartup and loads ICMP.DLL.
	WinMTRNet net(cfg);
	if(!net.initialized) {
		fprintf(stderr, "Failed to initialize network core.\n");
		return 2;
	}
	g_net = &net;

	int traddr = 0;
	if(!ResolveHost(host, &traddr)) {
		fprintf(stderr, "Unable to resolve host: %s\n", host);
		return 3;
	}

	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// Open the CSV log in append mode; write a header only for a new file.
	FILE *csv = NULL;
	if(csvPath) {
		bool isNew = !FileExists(csvPath);
		csv = fopen(csvPath, "a");
		if(csv == NULL) {
			fprintf(stderr, "Unable to open output file: %s\n", csvPath);
			return 4;
		}
		if(isNew) ReportWriteCsvHeader(csv);
	}

	// Open the human-readable live log in append mode (tailable with LogExpert).
	FILE *logf = NULL;
	if(logPath) {
		logf = fopen(logPath, "a");
		if(logf == NULL) {
			fprintf(stderr, "Unable to open log file: %s\n", logPath);
			return 4;
		}
	}

	printf("%s v%s - tracing %s (interval %.1fs, size %d, DNS %s)\n",
		WINMTR_NAME, WINMTR_VERSION, host, cfg.interval, cfg.pingsize,
		cfg.useDNS ? "on" : "off");
	if(logPath)  printf("Live log every %ds     -> %s\n", logInterval, logPath);
	if(csvPath)  printf("CSV snapshots every %ds -> %s\n", logInterval, csvPath);
	if(textPath) printf("Final text summary      -> %s\n", textPath);
	if(htmlPath) printf("Final HTML summary      -> %s\n", htmlPath);
	printf("Press Ctrl+C to stop.\n\n");

	// Start the (blocking) trace on its own thread.
	trace_ctx ctx;
	ctx.net  = &net;
	ctx.addr = traddr;
	unsigned tid = 0;
	HANDLE hTrace = (HANDLE)_beginthreadex(NULL, 0, TraceRunner, &ctx, 0, &tid);

	// Reporter loop: wake up in small slices so we react quickly to a stop.
	int elapsed = 0;
	while(g_running) {
		Sleep(500);
		elapsed += 500;
		if(elapsed >= logInterval * 1000) {
			elapsed = 0;
			if(csv)  ReportAppendCsv(csv, &net, host);
			if(logf) ReportAppendLog(logf, &net, host);
			if(!quiet) ReportPrintConsole(&net, host);
		}
	}

	// Stop and wait for the trace threads to unwind (bounded by ICMP timeout).
	net.StopTrace();
	if(hTrace) {
		WaitForSingleObject(hTrace, 15000);
		CloseHandle(hTrace);
	}

	// Final snapshot + summaries.
	if(csv) {
		ReportAppendCsv(csv, &net, host);
		fclose(csv);
	}
	if(logf) {
		ReportAppendLog(logf, &net, host);
		fclose(logf);
	}
	if(textPath) ReportWriteText(textPath, &net, host);
	if(htmlPath) ReportWriteHtml(htmlPath, &net, host);

	printf("\nStopped.\n");
	if(!quiet) ReportPrintConsole(&net, host);

	g_net = NULL;
	return 0;
}
