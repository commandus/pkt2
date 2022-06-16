/**
 * Database backend config helper classes
 */
#include <fstream>
#include <string.h>
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
#else
#endif
#include <cstdlib>
#include <vector>

#include "database-config.h"

#include "duk/duktape.h"

#include "utilstring.h"
#include "errorcodes.h"
#include "messageformat.h"
/**
 * @brief Javascript error handler
 * @param env JavascriptContext object
 * @param msg error message
 * 
 */
static void duk_fatal_handler_process_descriptors(
	void *env, 
	const char *msg
)
{
	fprintf(stderr, "Javascript error: %s\n", (msg ? msg : ""));
	fflush(stderr);
	abort();
}

// --------------------------- ConfigDatabases

ConfigDatabases::ConfigDatabases(const std::string &filename)
{
	std::string v = pkt2utilstring::file2string(filename);
	load(v);
}
	
int ConfigDatabases::load
(
	const std::string &value
) 
{
	duk_context *context = duk_create_heap(NULL, NULL, NULL, this, duk_fatal_handler_process_descriptors);
	if (int r = duk_peval_string(context, value.c_str()) != 0) {
		return r;
	}
	duk_pop(context);  // ignore result

	duk_push_global_object(context);

	// read settings
	duk_get_prop_string(context, -1, "databases");
	if (duk_is_array(context, -1)) 
	{
		duk_size_t n = duk_get_length(context, -1);
		for (duk_size_t i = 0; i < n; i++) 
		{
			ConfigDatabase cfg;
            cfg.id = 0;
            cfg.active = true;
            cfg.port = 0;

            if (duk_get_prop_index(context, -1, i))
			{
				if (duk_get_prop_string(context, -1, "id"))
					cfg.id = duk_get_int(context, -1);
				duk_pop(context);

				if (duk_get_prop_string(context, -1, "name"))
					cfg.name = duk_get_string(context, -1);
				duk_pop(context);
				if (duk_get_prop_string(context, -1, "type"))
					cfg.type = duk_get_string(context, -1);
				duk_pop(context);
                if (duk_get_prop_string(context, -1, "active"))
                    cfg.active = duk_get_boolean(context, -1);
                duk_pop(context);
				if (duk_get_prop_string(context, -1, "connection"))
					cfg.connectionString = duk_get_string(context, -1);
				duk_pop(context);
				if (duk_get_prop_string(context, -1, "login"))
					cfg.login = duk_get_string(context, -1);
				duk_pop(context);
				if (duk_get_prop_string(context, -1, "password"))
					cfg.password = duk_get_string(context, -1);
				duk_pop(context);
				if (duk_get_prop_string(context, -1, "db"))
					cfg.db = duk_get_string(context, -1);
				duk_pop(context);

				duk_get_prop_string(context, -1, "table_aliases");
				if (duk_is_array(context, -1)) 
				{
					duk_size_t sz = duk_get_length(context, -1);
					for (duk_size_t v = 0; v < sz; v++) 
					{
						if (duk_get_prop_index(context, -1, v)) 
						{
							if (duk_is_array(context, -1)) {
								duk_size_t sz2 = duk_get_length(context, -1);
								if (sz2 >= 2) {
									std::string n;
									std::string v;
									if (duk_get_prop_index(context, -1, 0)) {
										n = duk_get_string(context, -1);
										duk_pop(context);
									}
									if (duk_get_prop_index(context, -1, 1)) {
										v = duk_get_string(context, -1);
										duk_pop(context);
									}
									cfg.tableAliases[n] = v;
								}
							}
						}
						duk_pop(context);
					}
				}
				duk_pop(context);

				duk_get_prop_string(context, -1, "field_aliases");
				if (duk_is_array(context, -1)) 
				{
					duk_size_t sz = duk_get_length(context, -1);
					for (duk_size_t v = 0; v < sz; v++) 
					{
						if (duk_get_prop_index(context, -1, v)) 
						{
							if (duk_is_array(context, -1)) {
								duk_size_t sz2 = duk_get_length(context, -1);
								if (sz2 >= 2) {
									std::string n;
									std::string v;
									if (duk_get_prop_index(context, -1, 0)) {
										n = duk_get_string(context, -1);
										duk_pop(context);
									}
									if (duk_get_prop_index(context, -1, 1)) {
										v = duk_get_string(context, -1);
										duk_pop(context);
									}
									cfg.fieldAliases[n] = v;
								}
							}
						}
						duk_pop(context);
					}
				}
				duk_pop(context);

				duk_get_prop_string(context, -1, "properties");
				if (duk_is_array(context, -1)) 
				{
					duk_size_t sz = duk_get_length(context, -1);
					for (duk_size_t v = 0; v < sz; v++) 
					{
						if (duk_get_prop_index(context, -1, v)) 
						{
							if (duk_is_array(context, -1)) {
								duk_size_t sz2 = duk_get_length(context, -1);
								if (sz2 >= 2) {
									std::string n;
									std::string v;
									if (duk_get_prop_index(context, -1, 0)) {
										n = duk_get_string(context, -1);
										duk_pop(context);
									}
									if (duk_get_prop_index(context, -1, 1)) {
										v = duk_get_string(context, -1);
										duk_pop(context);
									}
									cfg.properties[n] = v;
								}
							}
						}
						duk_pop(context);
					}
				}
				duk_pop(context);

			}
			duk_pop(context);
			dbs.push_back(cfg);
		}
	}
	duk_pop(context);
	duk_destroy_heap(context);
	return 0;
}

int ConfigDatabases::getDialect(int index) const
{
    const ConfigDatabase &db = dbs.at(index);
    if (db.type == "postgresql")
        return SQL_POSTGRESQL;
    else
    if (db.type == "mysql")
        return SQL_MYSQL;
    else
    if (db.type == "firebird")
        return SQL_FIREBIRD;
    else
    if (db.type == "sqlite3")
        return SQL_SQLITE;
    else
        return SQL_SQLITE;
}

std::string ConfigDatabases::toString(int index) const
{
    const ConfigDatabase &db = dbs.at(index);
    std::stringstream ss;
    ss << "{"
       << "\"id\": " << db.id << ", "
       << "\"name\": \"" << db.name << "\", "
       << "\"type\": \"" << db.type << "\", "
       << "\"active\": " << (db.active ? "true" : "false" ) << ", "
       << "\"connection\": \"" << db.connectionString << "\", "
       << "\"login\": \"" << db.login << "\", "
       << "\"password\": \"" << db.password << "\", "
       << "\"db\": \"" << db.db << "\"";

    if (db.tableAliases.size()) {
        ss << ", \"table_aliases\": [";
        bool isNext = false;
        for (std::map<std::string, std::string>::const_iterator it(db.tableAliases.begin()); it != db.tableAliases.end(); it++) {
            if (isNext) {
                ss << ", ";
            } else {
                isNext = true;
            }
            ss << "[\"" << it->first << "\", \"" << it->second << "\"]";
        }
        ss << "]";
    }

    if (db.fieldAliases.size()) {
        ss << ", \"field_aliases\": [";
        bool isNext = false;
        for (std::map<std::string, std::string>::const_iterator it(db.fieldAliases.begin()); it != db.fieldAliases.end(); it++) {
            if (isNext) {
                ss << ", ";
            } else {
                isNext = true;
            }
            ss << "[\"" << it->first << "\", \"" << it->second << "\"]";
        }
        ss << "]";
    }

    if (db.properties.size()) {
        ss << ", \"properties\": [";
        bool isNext = false;
        for (std::map<std::string, std::string>::const_iterator it(db.properties.begin()); it != db.properties.end(); it++) {
            if (isNext) {
                ss << ", ";
            } else {
                isNext = true;
            }
            ss << "[\"" << it->first << "\", \"" << it->second << "\"]";
        }
        ss << "]";
    }

    ss << "}";
    return ss.str();
}

std::string ConfigDatabases::toString() const
{
	std::stringstream ss;
	ss << "[";
	bool isNext = false;
	for (int i = 0; i < dbs.size(); i++) {
		if (isNext) {
			ss << ", ";
		} else {
			isNext = true;
		}
		ss << toString(i) << std::endl;
	}
	ss << "]";
	return ss.str();
}

const ConfigDatabase* ConfigDatabases::findByName(
	const std::string &name
) const
{
	for (std::vector<ConfigDatabase>::const_iterator it(dbs.begin()); it != dbs.end(); it++ ) {
		if (it->name == name) {
			return &(*it);
		}
	}
	return NULL;
}

void ConfigDatabases::setProperties(
    std::map<std::string, std::string> &retval,
    const std::map<std::string, std::string> &values,
    int index
) const
{
    const ConfigDatabase &db = dbs.at(index);
    // copy only values lsted in aliases, and replace key to the alias name
    for (std::map<std::string, std::string>::const_iterator it(db.properties.begin()); it != db.properties.end(); it++) {
        std::map<std::string, std::string>::const_iterator f = values.find(it->first);
        if (f != values.end()) {
            if (!it->second.empty()) {
                retval[it->second] = f->second;
            }
        }
    }
}

