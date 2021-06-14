#include "pg-connect.h"

/**
 * Establish configured database connection
 */
PGconn *dbconnect(PGConfig *config)
{
	if (!config->dbconn.empty())
		return PQconnectdb(config->dbconn.c_str());
	else
		return PQsetdbLogin(config->dbhost.c_str(), config->dbport.c_str(), config->dboptionsfile.c_str(),
			NULL, config->dbname.c_str(), config->dbuser.c_str(), config->dbpassword.c_str());
}
