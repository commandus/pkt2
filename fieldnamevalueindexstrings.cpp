/*
 * fieldnamevalueindexstrings.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: andrei
 */

#include "fieldnamevalueindexstrings.h"
#include <algorithm>

#include "utilstring.h"

#define PROPERTY_INT_TIME	"time"

static const std::string sql2tableNumeric = "num";
static const std::string sql2tableString = "str";
// SQL mode 2 key fields
static const std::string sql2names[] = {"message", "time", "device", "field", "value"};

FieldNameValueString::FieldNameValueString
(
	const FieldNameValueString &value
)
	: index(value.index), field_type(value.field_type), field(value.field), value(value.value)
{
	sql_string = value.sql_string;
}	

FieldNameValueString::FieldNameValueString
(
	int idx,
	const google::protobuf::FieldDescriptor::CppType fieldtype,
	bool sqlstring,
	const std::string &fld,
	const std::string &val
)
		: index(idx), field_type(fieldtype), field(fld), value(val), sql_string(sqlstring) 
{

}

static const std::string empty_string("");

FieldNameValueIndexStrings::FieldNameValueIndexStrings()
	: table(empty_string), string_quote ("'"), quote("\"")
{
	index2values.reserve(3);
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
	const FieldNameValueIndexStrings &value
)
	: options(value.options), table(value.table), index2values(value.index2values), 
	string_quote(value.string_quote), quote(value.quote), values(value.values)
{
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

std::string findAlias(
	const std::map<std::string, std::string> *aliases,
	const std::string &name
) {
	std::map<std::string, std::string>::const_iterator it;
	if (aliases && (it = aliases->find(name)) != aliases->end())
		return it->second;
	else 
		return name;
}

/**
 * After all message "parsed" get INSERT clause
 * @return String
 */
void FieldNameValueIndexStrings::toStringInsert
(
	std::vector<std::string> *stmts,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties,
	int sqlDialect
) {
	std::stringstream ss;
	std::string tableName = findAlias(tableAliases, table);
	// if alias set to empty string, skip table
	if (tableName.empty())
		return;

	int sz = values.size();

	std::string sqlquote;
	if (sqlDialect == SQL_MYSQL)
		sqlquote = "`";
	else
		sqlquote = "\"";
	ss << "INSERT INTO " << sqlquote << pkt2utilstring::replace(tableName, ".", "_") << sqlquote << "(";
	int fieldCount = 0;
	for (int i = 0; i < sz; i++)
	{
		std::string fieldName = findAlias(fieldAliases, values[i].field);
		// if alias set to empty string, skip table
		if (fieldName.empty())
			continue;
		if (fieldCount)
			ss << ", ";
		ss << sqlquote << fieldName << sqlquote;
		fieldCount++;
	}

	// add property name if exists
	if (properties) {
		for (std::map<std::string, std::string>::const_iterator it(properties->begin()); it != properties->end(); it++)
		{
			if (fieldCount)
				ss << ", ";
			ss << sqlquote << it->first << sqlquote;
			fieldCount++;
		}
	}

	if (fieldCount == 0)
		return;

	ss << ") VALUES (";
	sz = values.size();
	fieldCount = 0;
	for (int i = 0; i < sz; i++)
	{
		if (findAlias(fieldAliases, values[i].field).empty())
			continue;
		if (fieldCount)
			ss << ", ";
		if ((values[i].field_type >= google::protobuf::FieldDescriptor::CPPTYPE_STRING) || (!pkt2utilstring::isNumber(values[i].value)))
			ss << string_quote << values[i].value << string_quote;
		else
			ss << values[i].value;
		fieldCount++;
	}

	// add property value if exists
	if (properties) {
		for (std::map<std::string, std::string>::const_iterator it(properties->begin()); it != properties->end(); it++)
		{
			if (fieldCount)
				ss << ", ";
			/*	
			if ((it->first == PROPERTY_INT_TIME) && pkt2utilstring::isNumber(it->second))
				ss << it->second;
			else
			*/
			ss << string_quote << it->second << string_quote;
			fieldCount++;
		}
	}

	ss << ");" << std::endl;
	stmts->push_back(ss.str());
}

std::string getSqlDialectTypeName(
	google::protobuf::FieldDescriptor::CppType fieldType,
	int sqldialect
)
{
	if (sqldialect == SQL_SQLITE) {
		switch (fieldType) {
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
				return "integer";
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:				
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				return "double";
			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
				return "integer";
		default:
			// >= google::protobuf::FieldDescriptor::CPPTYPE_STRING)
			return "TEXT";
		}
	}

	if (sqldialect == SQL_MYSQL) {
		switch (fieldType) {
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				return "int";
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				return "bigint";
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
				return "int unsigned";
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
				return "bigint unsigned";
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				return "float";
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				return "double";
			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
				return "int";
		default:
			// >= google::protobuf::FieldDescriptor::CPPTYPE_STRING)
			return "TEXT";
		}
	}

	if (sqldialect == SQL_POSTGRESQL) {
		switch (fieldType) {
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				return "integer";
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				return "bigint";
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
				return "bigint";
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
				return "numeric(20)";
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				return "float";
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				return "double precision";
			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
				return "integer";
		default:
			// >= google::protobuf::FieldDescriptor::CPPTYPE_STRING)
			return "TEXT";
		}
	}

	if (sqldialect == SQL_FIREBIRD) {
		switch (fieldType) {
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				return "integer";
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				return "bigint";
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
				return "bigint";
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				return "float";
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				return "double precision";
			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
				return "integer";
		default:
			// >= google::protobuf::FieldDescriptor::CPPTYPE_STRING)
			return "BLOB sub_type 1";
		}
	}

	return "";
}

/**
 * put CREATE TABLE clause inyo output parameter
 * @param output return cluase
 * @param tableName table name
 * @param sqldialect SQL dialect 0..2
 * @param fieldAliases message field aliases
 * @return String
 */
void FieldNameValueIndexStrings::toCreateSQLTableFields
(
	std::ostream *output,
	const std::string &tableName,
	int sqldialect,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
) {
	std::string quote;
	if (sqldialect == SQL_MYSQL)
		quote = "`";	// MySQL exceptions for spaces and reserved words
	else
		quote = "\"";

	*output << "CREATE TABLE " << quote << pkt2utilstring::replace(tableName, ".", "_") << quote << "(";

	int sz = values.size();

	int fieldCount = 0;
	for (int i = 0; i < sz; i++)
	{
		std::string fieldName = findAlias(fieldAliases, values[i].field);
		// if alias set to empty string, skip table
		if (fieldName.empty())
			continue;
		if (fieldCount)
			*output << ", ";
		*output << quote << fieldName << quote << " ";
		*output << getSqlDialectTypeName(values[i].field_type, sqldialect);
		fieldCount++;
	}
	if (fieldCount == 0)
		return;

	if (properties) {
		for (std::map<std::string, std::string>::const_iterator it(properties->begin()); it != properties->end(); it++)
		{
			if (it->second.empty())
				continue;
			if (fieldCount)
				*output << ", ";
			*output << quote << it->second << quote << " ";
			// force string type
			google::protobuf::FieldDescriptor::CppType cpptype;
			if (it->first == PROPERTY_INT_TIME)
				cpptype = google::protobuf::FieldDescriptor::CPPTYPE_UINT32;
			else
				cpptype = google::protobuf::FieldDescriptor::CPPTYPE_STRING;
			*output << getSqlDialectTypeName(cpptype, sqldialect);
			fieldCount++;
		}
	}

	*output << ");";
}

// 		values.push_back(FieldNameValueString(index, field_type, field, string_quote + replace(value, string_quote, string_quote + string_quote) + string_quote));

/**
 * After all message "parsed" get INSERT clause
 * @return String
 */
void FieldNameValueIndexStrings::toStringInsert2
(
	std::vector<std::string> *stmts,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties,
	int sqlDialect
)
{
	std::stringstream ssprefix;

	std::string sqlquote;
	if (sqlDialect == SQL_MYSQL)
		sqlquote = "`";
	else
		sqlquote = "\"";

	// index first
	for (int i = 1; i < index2values.size(); i++)
	{
		ssprefix << sql2names[i] << ",";
	}
	ssprefix << sql2names[3] << "," << sql2names[4] << ") VALUES (" << index2values[0] << ",";

	// values (index first)
	for (int i = 1; i < index2values.size(); i++)
	{
		if ((values[index2values[i]].field_type >= google::protobuf::FieldDescriptor::CPPTYPE_STRING) 
			|| (!pkt2utilstring::isNumber(values[index2values[i]].value))
			|| values[index2values[i]].sql_string
			)
			ssprefix << string_quote << values[index2values[i]].value << string_quote << ",";
		else
			ssprefix << values[index2values[i]].value << ",";
	}

	std::string prefix(ssprefix.str());
	prefix = prefix.substr(0, prefix.size() - 1); // remove last ","
	
	// each non-index field
	for (int i = 0; i < values.size(); i++)
	{
		std::string fieldName = findAlias(fieldAliases, values[i].field);
		// if alias set to empty string, skip table
		if (fieldName.empty())
			continue;

		std::stringstream ss;
		if (std::find(index2values.begin(), index2values.end(), i) != index2values.end())
			continue;

		if ((values[i].field_type >= google::protobuf::FieldDescriptor::CPPTYPE_STRING) 
			|| (!pkt2utilstring::isNumber(values[i].value))
			|| values[i].sql_string)
		{
			ss << "INSERT INTO " << sqlquote << sql2tableString << sqlquote << "(" << sql2names[0] << ","
				<< prefix << ",'" << fieldName << "'"
				<< "," << string_quote << values[i].value << string_quote << ");" << std::endl;
		}
		else
		{
			ss << "INSERT INTO " << sqlquote << sql2tableNumeric << sqlquote << "(" << sql2names[0] << ","
				<< prefix << ",'" << fieldName << "'"
				<< "," << values[i].value << ");" << std::endl;
		}
		
		stmts->push_back(ss.str());
	}

	// add properties if exists
	if (properties) {
		// TODO
	}
}

/**
 * CSV line
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringCSV(
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
) {
	std::stringstream ss;
	int sz = values.size();
	std::string tableName = findAlias(tableAliases, table);
	// if alias set to empty string, skip table
	if (tableName.empty())
		return "";

	ss << quote << tableName << quote << ", ";
	sz = values.size();
	for (int i = 0; i < sz; i++)
	{
		ss << values[i].value;
		if (i < sz - 1)
			ss << ", ";
	}

	if (properties) 
	{
		bool addColon = sz > 0;
		for (std::map<std::string, std::string>::const_iterator it(properties->begin()); it != properties->end(); it++)
		{
			if (addColon)
				ss << ", ";
			else
				addColon = true;
			ss << it->second;
		}
	}

	ss << std::endl;
	return ss.str();
};

/**
 * Tab delimited line
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringTab(
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
) {
	std::stringstream ss;
	int sz = values.size();
	std::string tableName = findAlias(tableAliases, table);
	// if alias set to empty string, skip table
	if (tableName.empty())
		return "";
	ss << quote << tableName << quote << "\t";
	for (int i = 0; i < sz; i++)
	{
		ss << values[i].value;
		if (i < sz - 1)
			ss << "\t";
	}

	if (properties) 
	{
		bool addColon = sz > 0;
		for (std::map<std::string, std::string>::const_iterator it(properties->begin()); it != properties->end(); it++)
		{
			if (addColon)
				ss << "\t";
			else
				addColon = true;
			ss << it->second;
		}
	}

	ss << std::endl;
	return ss.str();
}

/**
 * JSON
 * @return String
 */
std::string FieldNameValueIndexStrings::toStringJSON(
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
) {
	std::string tableName = findAlias(tableAliases, table);
	// if alias set to empty string, skip table
	if (tableName.empty())
		return "";

	std::stringstream ss;
	int sz = values.size();

	ss << "{\"" << tableName << "\":{";
	int fieldCount = 0;
	for (int i = 0; i < sz; i++)
	{
		std::string fieldName = findAlias(fieldAliases, values[i].field);
		if (fieldName.empty())
			continue;
		if (fieldCount)
			ss << ", ";
		ss << "\"" << fieldName << "\": ";
		if ((values[i].field_type == google::protobuf::FieldDescriptor::CPPTYPE_STRING) 
			|| (!pkt2utilstring::isNumber(values[i].value)))
			ss << "\"" << values[i].value << "\"";
		else
			ss << values[i].value;
		fieldCount++;
	}
	if (properties)
	{
		bool addColon = fieldCount > 0;
		for (std::map<std::string, std::string>::const_iterator it(properties->begin()); it != properties->end(); it++)
		{
			if (addColon)
				ss << ", ";
			addColon = true;
			ss << "\"" << it->first << "\": ";
			/*
			if ((it->first == PROPERTY_INT_TIME) && pkt2utilstring::isNumber(it->second))
				ss << it->second;
			else
			*/
			ss << "\"" << it->second << "\"";
		}
	}

	ss << "}}" << std::endl;
	return ss.str();
}

/**
 * @brief return value
 * @return String
 */
std::string FieldNameValueIndexStrings::findByLastName
(
	const std::string &name_suffix
) 
{
	int sz = values.size();
	int len = name_suffix.length();
	for (int i = 0; i < sz; i++)
	{
		int l = values[i].value.length();
		if (l < len)
			continue;
		if (values[i].field.substr(l - len, l) == name_suffix )
			return values[i].value;
	}
	return "";
}
