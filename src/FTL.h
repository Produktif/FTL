/* Pi-hole: A black hole for Internet advertisements
*  (c) 2017 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  Global definitions
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */
#ifndef FTL_H
#define FTL_H

#define __USE_XOPEN
#define _GNU_SOURCE
#include <stdio.h>
// variable argument lists
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
// struct sockaddr_in
#include <netinet/in.h>
// char* inet_ntoa(struct in_addr in)
#include <arpa/inet.h>
// getnameinfo();
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
//#include <math.h>
#include <pwd.h>
// syslog
#include <syslog.h>
// tolower()
#include <ctype.h>
// Interfaces
#include <ifaddrs.h>
#include <net/if.h>

// Define MIN and MAX macros, use them only when x and y are of the same type
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
// MIN(x,y) is already defined in dnsmasq.h

#define SOCKETBUFFERLEN 1024

// How often do we garbage collect (to ensure we only have data fitting to the MAXLOGAGE defined above)? [seconds]
// Default: 3600 (once per hour)
#define GCinterval 3600

// Delay applied to the garbage collecting [seconds]
// Default: -60 (one minute before a full hour)
#define GCdelay (-60)

// How many client connection do we accept at once?
#define MAXCONNS 255

// Over how many queries do we iterate at most when trying to find a match?
#define MAXITER 1000

// How many hours do we want to store in FTL's memory? [hours]
#define MAXLOGAGE 24

// Interval for overTime data [seconds]
// Default: 600 (10 minute intervals)
#define OVERTIME_INTERVAL 600

// How many overTime slots do we need?
// (24+1) hours * number of intervals per hour
// We need to be able to hold 25 hours as we need some reserve
// due to that GC is only running once an hours so the shown data
// can be 24 hours + 59 minutes
#define OVERTIME_SLOTS ((MAXLOGAGE+1)*3600/OVERTIME_INTERVAL)

// Interval for resolving NEW client and upstream server host names [seconds]
// Default: 60 (once every minute)
#define RESOLVE_INTERVAL 60

// Interval for re-resolving ALL known host names [seconds]
// Default: 3600 (once every hour)
#define RERESOLVE_INTERVAL 3600

// Privacy mode constants
#define HIDDEN_DOMAIN "hidden"
#define HIDDEN_CLIENT "0.0.0.0"

// Used to check memory integrity in various structs
#define MAGICBYTE 0x57

// Some magic database constants
#define DB_FAILED -2
#define DB_NODATA -1

// Add a timeout for the pihole-FTL.db database connection [milliseconds]
// This prevents immediate failures when the database is busy for a short time.
// Default: 1000 (one second)
#define DATABASE_BUSY_TIMEOUT 1000

// After how much time does a valid API session expire? [seconds]
// Default: 300 (five minutes)
#define API_SESSION_EXPIRE 300

// How many authenticated API clients are allowed simultaneously? [.]
#define API_MAX_CLIENTS 16

// FTLDNS enums
//enum { DNSSEC_UNSPECIFIED, DNSSEC_SECURE, DNSSEC_INSECURE, DNSSEC_BOGUS, DNSSEC_ABANDONED, DNSSEC_UNKNOWN };
//enum { QUERY_UNKNOWN, QUERY_GRAVITY, QUERY_FORWARDED, QUERY_CACHE, QUERY_WILDCARD, QUERY_BLACKLIST, QUERY_EXTERNAL_BLOCKED_IP, QUERY_EXTERNAL_BLOCKED_NULL, QUERY_EXTERNAL_BLOCKED_NXRA };
//enum { TYPE_A = 0, TYPE_AAAA, TYPE_ANY, TYPE_SRV, TYPE_SOA, TYPE_PTR, TYPE_TXT, TYPE_MAX };

enum { QUERIES, UPSTREAMS, CLIENTS, DOMAINS, OVERTIME, WILDCARD, DNS_CACHE };
enum { DNSSEC_UNSPECIFIED, DNSSEC_SECURE, DNSSEC_INSECURE, DNSSEC_BOGUS, DNSSEC_ABANDONED };
enum { QUERY_UNKNOWN, QUERY_GRAVITY, QUERY_FORWARDED, QUERY_CACHE, QUERY_REGEX, QUERY_BLACKLIST, \
       QUERY_EXTERNAL_BLOCKED_IP, QUERY_EXTERNAL_BLOCKED_NULL, QUERY_EXTERNAL_BLOCKED_NXRA, \
       QUERY_GRAVITY_CNAME, QUERY_REGEX_CNAME, QUERY_BLACKLIST_CNAME, QUERY_STATUS_MAX };
enum { TYPE_A = 1, TYPE_AAAA, TYPE_ANY, TYPE_SRV, TYPE_SOA, TYPE_PTR, TYPE_TXT, TYPE_NAPTR, TYPE_MAX };

enum { REPLY_UNKNOWN, REPLY_NODATA, REPLY_NXDOMAIN, REPLY_CNAME, REPLY_IP, REPLY_DOMAIN, REPLY_RRNAME, REPLY_SERVFAIL, REPLY_REFUSED, REPLY_NOTIMP, REPLY_OTHER };
enum { PRIVACY_SHOW_ALL = 0, PRIVACY_HIDE_DOMAINS, PRIVACY_HIDE_DOMAINS_CLIENTS, PRIVACY_MAXIMUM, PRIVACY_NOSTATS };
enum { MODE_IP, MODE_NX, MODE_NULL, MODE_IP_NODATA_AAAA, MODE_NODATA };
enum { REGEX_BLACKLIST, REGEX_WHITELIST };

// Use out own memory handling functions that will detect possible errors
// and report accordingly in the log. This will make debugging FTL crashs
// caused by insufficient memory or by code bugs (not properly dealing
// with NULL pointers) much easier.
#define free(param) FTLfree(param, __FILE__,  __FUNCTION__,  __LINE__)
#define lib_strdup() strdup()
#undef strdup
#define strdup(param) FTLstrdup(param, __FILE__,  __FUNCTION__,  __LINE__)
#define calloc(p1,p2) FTLcalloc(p1,p2, __FILE__,  __FUNCTION__,  __LINE__)
#define realloc(p1,p2) FTLrealloc(p1,p2, __FILE__,  __FUNCTION__,  __LINE__)

extern pthread_t DBthread;
extern pthread_t GCthread;
extern pthread_t DNSclientthread;
extern pthread_t timerthread;

// Intentionally ignore result of function declared warn_unused_result
#define igr(x) {__typeof__(x) __attribute__((unused)) d=(x);} 

#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

#endif // FTL_H
