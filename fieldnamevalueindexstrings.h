/*
 * fieldnamevalueindexstrings.h
 */
#ifndef FIELDNAMEVALUEINDEXSTRINGS_H_
#define FIELDNAMEVALUEINDEXSTRINGS_H_
#if defined(_WIN32) || defined(_WIN64)
#else
#include <unistd.h>
#endif
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "output-message.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "pkt2optionscache.h"
#include "messageformat.h"

#include "messagedecomposer.h"

class FieldNameValueString
{
public:
	int index;
	google::protobuf::FieldDescriptor::CppType field_type;
	std::string field;
	std::string value;
	bool sql_string;

	FieldNameValueString();

	FieldNameValueString
	(
		const FieldNameValueString &value 
	);

	FieldNameValueString
	(
		int idx,
		const google::protobuf::FieldDescriptor::CppType field_type,
		bool sql_string,
		const std::string &fld,
		const std::string &val
	);
};

/**
 * @brief Accumulate field names and values as string
 */
class FieldNameValueIndexStrings {
private:
	Pkt2OptionsCache *options;
	const std::string &table;
	std::vector<uint64_t> index2values;
	const std::string string_quote;
	std::string quote;
public:
	std::vector<FieldNameValueString> values;

	FieldNameValueIndexStrings();

	FieldNameValueIndexStrings
	(
		const FieldNameValueIndexStrings &value
	);

	FieldNameValueIndexStrings operator= (const FieldNameValueIndexStrings&value) 
	{
		FieldNameValueIndexStrings r(value);
		return r;
	};
	
	FieldNameValueIndexStrings
	(
		Pkt2OptionsCache *pkt2_options,
		const std::string &message_name
	);

	FieldNameValueIndexStrings
	(
		Pkt2OptionsCache *pkt2_options,
		const std::string &message_name,
		const std::string &astring_quote,
		const std::string &aquote
	);

	void add
	(
		const google::protobuf::FieldDescriptor *field,
		const std::string &value,
		int index,
		bool allow_quotes
	);

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	void toStringInsert
	(
		std::vector<std::string> *stmts,
		const std::map<std::string, std::string> *tableAliases,
		const std::map<std::string, std::string> *fieldAliases,
		int sqlDialect
	);

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	void toStringInsert2
	(
		std::vector<std::string> *stmts,
		const std::map<std::string, std::string> *tableAliases,
		const std::map<std::string, std::string> *fieldAliases,
		int sqlDialect
	);

	/**
	 * CSV line
	 * @return String
	 */
	std::string toStringCSV(
		const std::map<std::string, std::string> *tableAliases,
		const std::map<std::string, std::string> *fieldAliases
	);

	/**
	 * Tab delimited line
	 * @return String
	 */
	std::string toStringTab(
		const std::map<std::string, std::string> *tableAliases,
		const std::map<std::string, std::string> *fieldAliases
	);

	/**
	 * JSON
	 * @return String
	 */
	std::string toStringJSON(
		const std::map<std::string, std::string> *tableAliases,
		const std::map<std::string, std::string> *fieldAliases
	);
	
	/**
	* @brief return value
	* @return String
	*/
	std::string findByLastName
	(
		const std::string &name_suffix
	);

	/**
	 * put CREATE TABLE clause inyo output parameter
	 * @param output return cluase
	 * @param tableName table name
	 * @param sqldialect SQL dialect 0..2
	 * @param fieldAliases message field aliases
	 * @return String
	 */
	void toCreateSQLTableFields
	(
		std::ostream *output,
		const std::string &tableName,
		int sqldialect,
		const std::map<std::string, std::string> *fieldAliases
	);

};

std::string findAlias(
	const std::map<std::string, std::string> *aliases,
	const std::string &name
);

#endif /* FIELDNAMEVALUEINDEXSTRINGS_H_ */
