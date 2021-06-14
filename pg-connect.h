#ifndef PG_CONNECT_H
#define PG_CONNECT_H    1

#include <string>
#include <libpq-fe.h>

struct PGConfig {
    std::string dbconn;
	std::string dboptionsfile;
	std::string dbname;
	std::string dbuser;
	std::string dbpassword;
	std::string dbhost;
	std::string dbport;
	std::string dbsocket;
	std::string dbcharset;
	int dbclientflags;
};

/**
 * Establish configured database connection
 */
PGconn *dbconnect(PGConfig *config);

#endif
