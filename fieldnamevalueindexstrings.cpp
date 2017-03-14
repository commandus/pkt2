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

static const std::string sql2table = "proto";
static const std::string sql2names[] = {"message", "time", "device", "field", "value"};


FieldNameValueString::FieldNameValueString(
	int idx,
	const std::string &fld,
	const std::string &val
)
		: index(idx), field(fld), value(val)
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
	const std::string &field,
	const std::string &value,
	int index
)
{
	if (index > 0)
	{
		if (index >= index2values.size())
			index2values.resize(index + 1);
		index2values[index] = values.size();
	}
	values.push_back(FieldNameValueString(index, field, value));
}

void FieldNameValueIndexStrings::add_string
(
	const std::string &field,
	const std::string &value,
	int index
)
{
	add(field, string_quote + replace(value, string_quote, string_quote + string_quote) + string_quote, index);
}

/**
 * After all message "parsed" get INSERT clause
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringInsert() {
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
		ss << values[i].value;
		if (i < sz - 1)
			ss << ", ";
	}
	ss << ");" << std::endl;
	return ss.str();
}

/**
 * After all message "parsed" get INSERT clause
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringInsert2()
{
	std::stringstream ssprefix;
	ssprefix << "INSERT INTO " << quote << sql2table << quote << "(" << sql2names[0] << ",";

	// index first
	for (int i = 1; i < index2values.size(); i++)
	{
		ssprefix << sql2names[i] << ",";
	}
	ssprefix << sql2names[3] << "," << sql2names[4] << ") VALUES (" << index2values[0] << ",";

	// values (index first)
	for (int i = 1; i < index2values.size(); i++)
	{
		ssprefix << values[index2values[i]].value << ",";
	}

	std::string prefix(ssprefix.str());
	prefix = prefix.substr(0, prefix.size() - 1); // remove last ","

	std::stringstream ss;

	// each non-index field
	for (int i = 0; i < values.size(); i++)
	{
		if (std::find(index2values.begin(), index2values.end(), i) != index2values.end())
			continue;
		ss << prefix << ",'" << values[i].field << "'"
			<< "," << values[i].value << ");" << std::endl;
	}

	return ss.str();
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
