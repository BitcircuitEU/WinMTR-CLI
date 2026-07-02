//*****************************************************************************
// FILE:            WinMTRReport.h
//
//
// DESCRIPTION:
//   Output helpers for the WinMTR command line build. Turns the statistics
//   held in WinMTRNet into CSV log lines, an ASCII summary table, an HTML
//   report and a console dump.
//
//*****************************************************************************

#ifndef WINMTRREPORT_H_
#define WINMTRREPORT_H_

#include <stdio.h>

class WinMTRNet;

// Writes the CSV column header (call once for a freshly created file).
void ReportWriteCsvHeader(FILE *fp);

// Appends one CSV row per hop describing the current statistics, prefixed
// with a timestamp and the traced target. Flushes so nothing is lost on crash.
void ReportAppendCsv(FILE *fp, WinMTRNet *net, const char *target);

// Appends a human-readable, line-based snapshot (one timestamped line per hop)
// to a running log file. Flushes immediately so the file can be tailed live
// (e.g. with LogExpert) while the trace keeps running.
void ReportAppendLog(FILE *fp, WinMTRNet *net, const char *target);

// Writes the classic WinMTR ASCII statistics table to the given file.
// Returns true on success.
bool ReportWriteText(const char *path, WinMTRNet *net, const char *target);

// Writes an HTML statistics report to the given file. Returns true on success.
bool ReportWriteHtml(const char *path, WinMTRNet *net, const char *target);

// Prints the current statistics table to stdout.
void ReportPrintConsole(WinMTRNet *net, const char *target);

#endif // ifndef WINMTRREPORT_H_
