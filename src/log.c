/* Pi-hole: A black hole for Internet advertisements
*  (c) 2017 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  Logging routines
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */

#include "FTL.h"
#include "version.h"
#include "memory.h"
#include "daemon.h"
#include "config.h"
#include "log.h"
// global variable username
#include "main.h"
// global variable daemonmode
#include "args.h"
// global counters variable
#include "shmem.h"

static pthread_mutex_t lock;
static FILE *logfile = NULL;

static void close_FTL_log(void)
{
	if(logfile != NULL)
		fclose(logfile);
}

void open_FTL_log(const bool test)
{
	if(test)
	{
		// Obtain log file location
		getLogFilePath();
	}

	// Open the log file in append/create mode
	logfile = fopen(FTLfiles.log, "a+");
	if((logfile == NULL) && test){
		syslog(LOG_ERR, "Opening of FTL\'s log file failed!");
		printf("FATAL: Opening of FTL log (%s) failed!\n",FTLfiles.log);
		printf("       Make sure it exists and is writeable by user %s\n", username);
		// Return failure
		exit(EXIT_FAILURE);
	}

	if(test)
	{
		if (pthread_mutex_init(&lock, NULL) != 0)
		{
			printf("FATAL: Log mutex init failed\n");
			// Return failure
			exit(EXIT_FAILURE);
		}
		close_FTL_log();
	}
}

void get_timestr(char *timestring, const time_t timein)
{
	struct tm tm;
	localtime_r(&timein, &tm);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	const int millisec = tv.tv_usec/1000;

	sprintf(timestring,"%d-%02d-%02d %02d:%02d:%02d.%03i", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, millisec);
}

void __attribute__ ((format (gnu_printf, 1, 2))) logg(const char *format, ...)
{
	char timestring[84] = "";
	va_list args;

	pthread_mutex_lock(&lock);

	get_timestr(timestring, time(NULL));

	// Get and log PID of current process to avoid ambiguities when more than one
	// pihole-FTL instance is logging into the same file
	const long pid = (long)getpid();

	// Print to stdout before writing to file
	if(!daemonmode)
	{
		printf("[%s %ld] ", timestring, pid);
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\n");
	}

	// Open log file
	open_FTL_log(false);

	// Write to log file
	if(logfile != NULL)
	{
		fprintf(logfile, "[%s %ld] ", timestring, pid);
		va_start(args, format);
		vfprintf(logfile, format, args);
		va_end(args);
		fputc('\n',logfile);
	}
	else if(!daemonmode)
	{
		printf("!!! WARNING: Writing to FTL\'s log file failed!\n");
		syslog(LOG_ERR, "Writing to FTL\'s log file failed!");
	}

	// Close log file
	close_FTL_log();

	pthread_mutex_unlock(&lock);
}

void format_memory_size(char *prefix, const unsigned long long int bytes, double *formated)
{
	int i;
	*formated = bytes;
	// Determine exponent for human-readable display
	for(i=0; i < 7; i++)
	{
		if(*formated <= 1e3)
			break;
		*formated /= 1e3;
	}
	const char* prefixes[8] = { "", "K", "M", "G", "T", "P", "E", "?" };
	// Chose matching SI prefix
	strcpy(prefix, prefixes[i]);
}

void log_counter_info(void)
{
	logg(" -> Total DNS queries: %i", counters->queries);
	logg(" -> Cached DNS queries: %i", counters->cached);
	logg(" -> Forwarded DNS queries: %i", counters->forwarded);
	logg(" -> Blocked DNS queries: %i", counters->blocked);
	logg(" -> Unknown DNS queries: %i", counters->unknown);
	logg(" -> Unique domains: %i", counters->domains);
	logg(" -> Unique clients: %i", counters->clients);
	logg(" -> Known forward destinations: %i", counters->upstreams);
}

void log_FTL_version(const bool crashreport)
{
	logg("FTL branch: %s", GIT_BRANCH);
	logg("FTL version: %s", get_FTL_version());
	logg("FTL commit: %s", GIT_HASH);
	logg("FTL date: %s", GIT_DATE);
	if(crashreport)
		logg("FTL user: started as %s, ended as %s", username, getUserName());
	else
		logg("FTL user: %s", username);
	logg("Compiled for %s using %s", FTL_ARCH, FTL_CC);
}

static char *FTLversion = NULL;
char __attribute__ ((malloc)) *get_FTL_version(void)
{
	// Obtain FTL version if not already determined
	if(FTLversion == NULL)
	{
		if(strlen(GIT_TAG) > 1)
		{
			FTLversion = strdup(GIT_VERSION);
		}
		else
		{
			FTLversion = calloc(13, sizeof(char));
			// Build version by appending 7 characters of the hash to "vDev-"
			snprintf(FTLversion, 13, "vDev-%.7s", GIT_HASH);
		}
	}

	return FTLversion;
}
