//*****************************************************************************
// FILE:            WinMTRReport.cpp
//
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRNet.h"
#include "WinMTRReport.h"

static void CurrentTimestamp(char *buf, size_t len)
{
	time_t now = time(NULL);
	struct tm lt;
	localtime_s(&lt, &now);
	strftime(buf, len, "%Y-%m-%d %H:%M:%S", &lt);
}

// Flush CRT buffers AND force the OS to update the on-disk file (size, mtime).
// Without _commit()/FlushFileBuffers the reported file size of a still-open
// file may stay at 0 in Explorer/dir until the handle is closed, even though
// the data is already readable.
static void FlushToDisk(FILE *fp)
{
	fflush(fp);
	_commit(_fileno(fp));
}

void ReportWriteCsvHeader(FILE *fp)
{
	if(fp == NULL) return;
	fprintf(fp, "timestamp,target,hop,host,loss_percent,sent,recv,best,avg,worst,last\n");
	FlushToDisk(fp);
}

void ReportAppendCsv(FILE *fp, WinMTRNet *net, const char *target)
{
	if(fp == NULL || net == NULL) return;

	char ts[32];
	CurrentTimestamp(ts, sizeof(ts));

	int nh = net->GetMax();
	char host[255];

	for(int i = 0; i < nh; i++) {
		net->GetName(i, host);
		if(strcmp(host, "") == 0) strcpy(host, "No response from host");

		fprintf(fp, "%s,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d\n",
			ts, target, i + 1, host,
			net->GetPercent(i), net->GetXmit(i), net->GetReturned(i),
			net->GetBest(i), net->GetAvg(i), net->GetWorst(i), net->GetLast(i));
	}
	FlushToDisk(fp);
}

void ReportAppendLog(FILE *fp, WinMTRNet *net, const char *target)
{
	if(fp == NULL || net == NULL) return;

	char ts[32];
	CurrentTimestamp(ts, sizeof(ts));

	int nh = net->GetMax();
	char host[255];

	for(int i = 0; i < nh; i++) {
		net->GetName(i, host);
		if(strcmp(host, "") == 0) strcpy(host, "No response from host");

		fprintf(fp,
			"%s [%s] hop=%-2d loss=%3d%% sent=%-4d recv=%-4d best=%-4dms avg=%-4dms worst=%-4dms last=%-4dms host=%s\n",
			ts, target, i + 1,
			net->GetPercent(i), net->GetXmit(i), net->GetReturned(i),
			net->GetBest(i), net->GetAvg(i), net->GetWorst(i), net->GetLast(i),
			host);
	}
	FlushToDisk(fp);
}

static void BuildTextReport(WinMTRNet *net, const char *target, char *f_buf, size_t /*cap*/)
{
	char buf[255], t_buf[1000];

	int nh = net->GetMax();

	strcpy(f_buf,  "|------------------------------------------------------------------------------------------|\r\n");
	sprintf(t_buf, "|                                      WinMTR statistics                                   |\r\n");
	strcat(f_buf, t_buf);
	sprintf(t_buf, "| Target: %-80s |\r\n", target);
	strcat(f_buf, t_buf);
	sprintf(t_buf, "|                       Host              -   %%  | Sent | Recv | Best | Avrg | Wrst | Last |\r\n" );
	strcat(f_buf, t_buf);
	sprintf(t_buf, "|------------------------------------------------|------|------|------|------|------|------|\r\n" );
	strcat(f_buf, t_buf);

	for(int i = 0; i < nh; i++) {
		net->GetName(i, buf);
		if(strcmp(buf, "") == 0) strcpy(buf, "No response from host");

		sprintf(t_buf, "|%40s - %4d | %4d | %4d | %4d | %4d | %4d | %4d |\r\n",
			buf, net->GetPercent(i),
			net->GetXmit(i), net->GetReturned(i), net->GetBest(i),
			net->GetAvg(i), net->GetWorst(i), net->GetLast(i));
		strcat(f_buf, t_buf);
	}

	sprintf(t_buf, "|________________________________________________|______|______|______|______|______|______|\r\n" );
	strcat(f_buf, t_buf);
}

bool ReportWriteText(const char *path, WinMTRNet *net, const char *target)
{
	if(net == NULL) return false;

	static char f_buf[255 * 100];
	BuildTextReport(net, target, f_buf, sizeof(f_buf));

	FILE *fp = fopen(path, "wt");
	if(fp == NULL) {
		fprintf(stderr, "Unable to open text report file: %s\n", path);
		return false;
	}
	fprintf(fp, "%s", f_buf);
	fclose(fp);
	return true;
}

bool ReportWriteHtml(const char *path, WinMTRNet *net, const char *target)
{
	if(net == NULL) return false;

	char buf[255], t_buf[1000];
	static char f_buf[255 * 100];

	int nh = net->GetMax();

	strcpy(f_buf, "<html><head><title>WinMTR Statistics</title></head><body bgcolor=\"white\">\r\n");
	sprintf(t_buf, "<center><h2>WinMTR statistics</h2></center>\r\n");
	strcat(f_buf, t_buf);
	sprintf(t_buf, "<center><p>Target: %s</p></center>\r\n", target);
	strcat(f_buf, t_buf);

	sprintf(t_buf, "<p align=\"center\"> <table border=\"1\" align=\"center\">\r\n" );
	strcat(f_buf, t_buf);

	sprintf(t_buf, "<tr><td>Host</td> <td>%%</td> <td>Sent</td> <td>Recv</td> <td>Best</td> <td>Avrg</td> <td>Wrst</td> <td>Last</td></tr>\r\n" );
	strcat(f_buf, t_buf);

	for(int i = 0; i < nh; i++) {
		net->GetName(i, buf);
		if(strcmp(buf, "") == 0) strcpy(buf, "No response from host");

		sprintf(t_buf, "<tr><td>%s</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td></tr>\r\n",
			buf, net->GetPercent(i),
			net->GetXmit(i), net->GetReturned(i), net->GetBest(i),
			net->GetAvg(i), net->GetWorst(i), net->GetLast(i));
		strcat(f_buf, t_buf);
	}

	sprintf(t_buf, "</table></body></html>\r\n");
	strcat(f_buf, t_buf);

	FILE *fp = fopen(path, "wt");
	if(fp == NULL) {
		fprintf(stderr, "Unable to open HTML report file: %s\n", path);
		return false;
	}
	fprintf(fp, "%s", f_buf);
	fclose(fp);
	return true;
}

void ReportPrintConsole(WinMTRNet *net, const char *target)
{
	if(net == NULL) return;

	static char f_buf[255 * 100];
	BuildTextReport(net, target, f_buf, sizeof(f_buf));
	fputs(f_buf, stdout);
	fflush(stdout);
}
