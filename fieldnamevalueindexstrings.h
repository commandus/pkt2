/*
 * fieldnamevalueindexstrings.h
 */


#ifndef FIELDNAMEVALUEINDEXSTRINGS_H_
#define FIELDNAMEVALUEINDEXSTRINGS_H_

#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>

#include "output-message.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "pkt2optionscache.h"

#include "messagedecomposer.h"

class FieldNameValueString
{
public:
	int index;
	google::protobuf::FieldDescriptor::CppType field_type;
	std::string field;
	std::string value;
	bool sql_string;
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
		int index
	);

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	void toStringInsert
	(
		std::vector<std::string> *stmts
	);

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	void toStringInsert2
	(
		std::vector<std::string> *stmts
	);

	/**
	 * CSV line
	 * @return String
	 */
	std::string toStringCSV();

	/**
	 * Tab delimited line
	 * @return String
	 */
	std::string toStringTab();

	/**
	 * JSON
	 * @return String
	 */
	std::string toStringJSON();

};

#endif /* FIELDNAMEVALUEINDEXSTRINGS_H_ */
