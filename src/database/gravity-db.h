/* Pi-hole: A black hole for Internet advertisements
*  (c) 2019 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  gravity database prototypes
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */
#ifndef GRAVITY_H
#define GRAVITY_H

// struct clientsData
#include "datastructure.h"

// Table indices
enum { GRAVITY_TABLE, EXACT_BLACKLIST_TABLE, EXACT_WHITELIST_TABLE, REGEX_BLACKLIST_TABLE, REGEX_WHITELIST_TABLE, UNKNOWN_TABLE };
enum { GRAVITY_DOMAINLIST_EXACT_WHITELIST = 0,
       GRAVITY_DOMAINLIST_EXACT_BLACKLIST = 1,
       GRAVITY_DOMAINLIST_REGEX_WHITELIST = 2,
       GRAVITY_DOMAINLIST_REGEX_BLACKLIST = 3 };

typedef struct domainrecord {
	const char *domain;
	time_t date_added;
	time_t date_modified;
	const char *comment;
	bool enabled;
} domainrecord;

bool gravityDB_open(void);
bool gravityDB_prepare_client_statements(const int clientID, clientsData* client);
void gravityDB_close(void);
bool gravityDB_getTable(unsigned char list);
const char* gravityDB_getDomain(int *rowid);
void gravityDB_finalizeTable(void);
int gravityDB_count(unsigned char list);
bool in_auditlist(const char *domain);

bool gravityDB_addToTable(const int type, const char* domain);
bool gravityDB_delFromTable(const int type, const char* domain);
bool gravityDB_readTable(const int type);
bool gravityDB_readTableGetDomain(domainrecord *domain);
void gravityDB_readTableFinalize(void);

bool in_gravity(const char *domain, const int clientID, clientsData* client);
bool in_whitelist(const char *domain, const int clientID, clientsData* client);
bool in_blacklist(const char *domain, const int clientID, clientsData* client);

bool gravityDB_get_regex_client_groups(clientsData* client, const int numregex, const int *regexid,
                                       const unsigned char type, const char* table, const int clientID);

#endif //GRAVITY_H
