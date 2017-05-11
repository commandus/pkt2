/*
 * fieldnamevalueindexstrings.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: andrei
 */

#include "fieldnamevalueindexstrings.h"
#include <algorithm>
#include <glog/logging.h>
#include <google/protobuf/message.h>
#include "utilstring.h"

static const std::string sql2tableNumeric = "num";
static const std::string sql2tableString = "str";
static const std::string sql2names[] = {"message", "time", "device", "field", "value"};


FieldNameValueString::FieldNameValueString
(
	int idx,
	const google::protobuf::FieldDescriptor::CppType fieldtype,
	bool sqlstring,
	const std::string &fld,
	const std::string &val
)
		: index(idx), field_type(fieldtype), sql_string(sqlstring), field(fld), value(val)
{

}

FieldNameValueIndexStrings::FieldNameValueIndexStrings
(
	Pkt2OptionsCache *pkt2_options,
	const std::string &message_name
)
	: options(pkt2_options), table(message_name), string_quote ("'"), quote("\"")
{
	index2values.reserve(3);
	index2values.push_back(options->getMessageId(message_name));
}

FieldNameValueIndexStrings::FieldNameValueIndexStrings
(
		Pkt2OptionsCache *pkt2_options,
		const std::string &message_name,
		const std::string &astring_quote,
		const std::string &aquote
)
	: options(pkt2_options), table(message_name), string_quote (astring_quote), quote(aquote)
{
	index2values.reserve(3);
	index2values.push_back(options->getMessageId(message_name));
}

void FieldNameValueIndexStrings::add
(
	const google::protobuf::FieldDescriptor *field,
	const std::string &value,
	int index,
	bool allow_quotes
)
{
	if (index > 0)
	{
		if (index >= index2values.size())
			index2values.resize(index + 1);
		index2values[index] = values.size();
	}

	if (allow_quotes)
	{
		pkt2::Variable variable = field->options().GetExtension(pkt2::variable);
		values.push_back(FieldNameValueString(index, field->cpp_type(), variable.sql_string(), field->full_name(), value));
	}
	else
		values.push_back(FieldNameValueString(index, field->cpp_type(), false, field->full_name(), value));
}

/**
 * After all message "parsed" get INSERT clause
 * @return String
 */
void FieldNameValueIndexStrings::toStringInsert
(
	std::vector<std::string> *stmts
) {
	std::stringstream ss;
	int sz = values.size();
	ss << "INSERT INTO " << quote << replace(table, ".", "_") << quote << "(";
	for (int i = 0; i < sz; i++)
	{
		ss << quote << values[i].field << quote;
		if (i < sz - 1)
			ss << ", ";
	}
	ss << ") VALUES (";
	sz = values.size();
	for (int i = 0; i < sz; i++)
	{
		if (values[i].field_type < google::protobuf::FieldDescriptor::CPPTYPE_STRING)
			ss << values[i].value;
		else
			ss << string_quote << values[i].value << string_quote;
		if (i < sz - 1)
			ss << ", ";
	}
	ss << ");" << std::endl;
	stmts->push_back(ss.str());
}

// 		values.push_back(FieldNameValueString(index, field_type, field, string_quote + replace(value, string_quote, string_quote + string_quote) + string_quote));

/**
 * After all message "parsed" get INSERT clause
 * @return String
 */
void FieldNameValueIndexStrings::toStringInsert2
(
		std::vector<std::string> *stmts
)
{
	std::stringstream ssprefix;
	// index first
	for (int i = 1; i < index2values.size(); i++)
	{
		ssprefix << sql2names[i] << ",";
	}
	ssprefix << sql2names[3] << "," << sql2names[4] << ") VALUES (" << index2values[0] << ",";

	// values (index first)
	for (int i = 1; i < index2values.size(); i++)
	{
		
		if ((values[index2values[i]].field_type < google::protobuf::FieldDescriptor::CPPTYPE_STRING) 
			&& (!values[index2values[i]].sql_string))
			ssprefix << values[index2values[i]].value << ",";
		else
			ssprefix << string_quote << values[index2values[i]].value << string_quote << ",";
	}

	std::string prefix(ssprefix.str());
	prefix = prefix.substr(0, prefix.size() - 1); // remove last ","

	std::stringstream ss;
	// each non-index field
	for (int i = 0; i < values.size(); i++)
	{
		ss.clear();
		if (std::find(index2values.begin(), index2values.end(), i) != index2values.end())
			continue;

		if ((values[i].field_type < google::protobuf::FieldDescriptor::CPPTYPE_STRING) && (!values[i].sql_string))
		{
			ss << "INSERT INTO " << quote << sql2tableNumeric << quote << "(" << sql2names[0] << ","
				<< prefix << ",'" << values[i].field << "'"
				<< "," << values[i].value << ");" << std::endl;
		}
		else
		{
			ss << "INSERT INTO " << quote << sql2tableString << quote << "(" << sql2names[0] << ","
				<< prefix << ",'" << values[i].field << "'"
				<< "," << string_quote << values[i].value << string_quote << ");" << std::endl;
		}
		
		stmts->push_back(ss.str());
	}
}

/**
 * CSV line
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringCSV() {
	std::stringstream ss;
	int sz = values.size();
	ss << quote << table << quote << ",";
	sz = values.size();
	for (int i = 0; i < sz; i++)
	{
		ss << values[i].value;
		if (i < sz - 1)
			ss << ", ";
	}
	ss << std::endl;
	return ss.str();
};

/**
 * Tab delimited line
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringTab() {
	std::stringstream ss;
	int sz = values.size();
	ss << quote << table << quote << "\t";
	for (int i = 0; i < sz; i++)
	{
		ss << values[i].value;
		if (i < sz - 1)
			ss << "\t";
	}
	ss << std::endl;
	return ss.str();
}

/**
 * JSON
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringJSON() {
	std::stringstream ss;
	int sz = values.size();
	ss << "\"" << table << "\":{";
	for (int i = 0; i < sz; i++)
	{
		ss << "\""<< values[i].field << "\": \"" << values[i].value << "\"";
		if (i < sz - 1)
			ss << ",";
	}
	ss << "}" << std::endl;
	return ss.str();
}
