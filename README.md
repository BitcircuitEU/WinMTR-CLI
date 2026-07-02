# WinMTR-CLI

A command line / headless fork of [WinMTR](https://github.com/WinMTR/WinMTR-Official).

## About

WinMTR combines the functionality of `traceroute` and `ping` in a single network
diagnostic tool. The original WinMTR is a Microsoft Windows **GUI** application.

This fork strips the graphical user interface (MFC) and turns WinMTR into a plain
Win32 **console application** (`winmtrcli.exe`) that:

- runs a continuous MTR-style trace against a target host,
- appends periodic statistics snapshots to a **CSV log file**,
- writes a final **text / HTML summary** on shutdown, and
- is designed for unattended **24/7 operation as a Windows service** (via
  [nssm](https://nssm.cc/)).

The network core (ICMP probing per TTL, DNS resolution, statistics) is unchanged;
only the GUI layer was removed and the core decoupled from it.

## License & Redistribution

WinMTR is offered as Open Source Software under GPL v2.

- [Read more about the licensing conditions](http://www.gnu.org/licenses/gpl-2.0.html)
- Upstream project: https://github.com/WinMTR/WinMTR-Official

## Build

Open `WinMTR.sln` in Visual Studio (with the "Desktop development with C++"
workload) and build the `Release` configuration for `x64` (or `Win32`). MFC is
**not** required anymore.

Output: `Release_x64\winmtrcli.exe` (or `Release_x32\winmtrcli.exe`).

The project links `ws2_32.lib` and `iphlpapi.lib`; ICMP functionality is loaded
dynamically from `ICMP.DLL` at runtime.

## Usage

```
winmtrcli [options] <host>
```

### Options

| Option | Description |
|--------|-------------|
| `-i, --interval <sec>` | Seconds between pings per hop (default `1.0`) |
| `-s, --size <bytes>` | ICMP payload size (default `64`) |
| `-m, --max-hops <n>` | Maximum number of hops / TTL (default `30`) |
| `-n, --numeric` | Do not resolve hop IPs to hostnames |
| `-l, --log <file>` | Live log file (human-readable, one line per hop appended per interval) |
| `-o, --output <file>` | CSV log file; one snapshot is appended per interval |
| `--log-interval <s>` | Seconds between log/CSV snapshots (default `60`) |
| `--text <file>` | Write final ASCII summary to file on exit |
| `--html <file>` | Write final HTML summary to file on exit |
| `-q, --quiet` | Do not print the live table to the console |
| `-h, --help` | Show help |

The trace runs until it is stopped (Ctrl+C, or a service stop). On stop the final
summary files are written and a last snapshot is appended to the log/CSV.

### Default output (no options given)

If you run `winmtrcli.exe google.de` **without** any output option
(`-l`/`-o`/`--text`/`--html`), the tool automatically creates files in the current
working directory:

- `google.de_<YYYYMMDD_HHMMSS>.log` - live log, written continuously while running
- `google.de_<YYYYMMDD_HHMMSS>_results.txt` - ASCII summary, written on stop
- `google.de_<YYYYMMDD_HHMMSS>_results.html` - HTML summary, written on stop

Each run uses a fresh timestamped name, so nothing gets overwritten. Pass explicit
options to control names/locations (recommended for a service).

### Live log for LogExpert

The `--log` file is written continuously (append + flush after every snapshot),
so you can open it in [LogExpert](https://github.com/zarunbal/LogExpert) (or any
tailing viewer) and watch values update **while the service keeps running**. Each
line starts with a timestamp, which LogExpert can parse into a time column:

```
2026-07-02 08:21:22 [google.de] hop=1  loss=  0% sent=6    recv=6    best=3   ms avg=5   ms worst=6   ms last=6   ms host=fritz.box
2026-07-02 08:21:22 [google.de] hop=7  loss=  0% sent=6    recv=6    best=20  ms avg=20  ms worst=24  ms last=21  ms host=dns.google
```

### Examples

Trace `8.8.8.8`, print a live table, and write a live log every 30 seconds:

```
winmtrcli --log C:\logs\mtr.log --log-interval 30 8.8.8.8
```

Headless logging (no console table) with live log, CSV and a final HTML report:

```
winmtrcli -q --log C:\logs\mtr.log -o C:\logs\mtr.csv --html C:\logs\mtr.html github.com
```

### CSV format

One row per hop and per snapshot:

```
timestamp,target,hop,host,loss_percent,sent,recv,best,avg,worst,last
```

## Running 24/7 as a Windows service (nssm)

[nssm](https://nssm.cc/) wraps any executable as a Windows service. When the
service is stopped, nssm sends `Ctrl+C` to the console, which `winmtrcli` catches
to stop cleanly and write its final summary.

```powershell
# Install the service (adjust paths). Everything after the exe is passed as args.
nssm install WinMTRCLI "C:\Tools\winmtrcli.exe" "-q --log C:\logs\mtr.log -o C:\logs\mtr.csv --html C:\logs\mtr.html --log-interval 60 8.8.8.8"

# Optionally capture stdout/stderr to log files
nssm set WinMTRCLI AppStdout C:\logs\winmtr-stdout.log
nssm set WinMTRCLI AppStderr C:\logs\winmtr-stderr.log

# Start / stop
nssm start WinMTRCLI
nssm stop  WinMTRCLI

# Remove the service
nssm remove WinMTRCLI confirm
```

Notes:

- Ensure the target log directory (e.g. `C:\logs`) exists and is writable by the
  service account.
- The CSV file is opened in append mode, so restarting the service keeps history
  in the same file. Delete or rotate the file yourself if desired.

## Troubleshooting

**Nothing happens / no replies.**
Usually antivirus or firewall software blocks ICMP. Configure them to allow
`winmtrcli.exe` (raw ICMP via `ICMP.DLL`).

**Unable to resolve host.**
Check DNS / the spelling of the host name, or pass an IP address directly.

## Bug Reports

Please include the WinMTR-CLI version, your OS, and the exact command line used.
