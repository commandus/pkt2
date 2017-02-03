#include "pkt2_code_generator.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/stl_util.h>

#include "utilstring.h"

#include "pkt2.pb.h"

const std::string CLAUSE_RETURN_VALUE("RETURN_VALUE");
const std::string CLAUSE_INCLUDES("#include \"pkt2util.h\"");
const std::string quote_mysql("`");

// Regular expression to match words beginning with 'search'
bool findWord
(
		std::string &subject,
		const std::string& search
)
{
	std::regex e("\\b" + search + "\\b");
	std::smatch m;
	return std::regex_search(subject, m, e);
}

void replaceWords
(
		std::string &subject,
		const std::string& search,
		const std::string& replace
)
{
    // Regular expression to match words beginning with 'search'
	std::regex e("(\\b(" + search + "))([^(),. ]*)");
	subject = std::regex_replace(subject, e, replace);
}

void replacePacketFieldNames
(
		std::string &subject,
		pkt2::Packet *packet
)
{
	for (int i = 0; i < packet->fields_size(); i++)
	{
		std::string f = packet->fields(i).name();
		replaceWords(subject, f, "src->" + f);
	}
}

std::string getProtoName(enum pkt2::Proto proto) {
	switch (proto) {
	case pkt2::PROTO_NONE:
		return "none";
	case pkt2::PROTO_TCP:
		return "tcp";
	case pkt2::PROTO_UDP:
		return "udp";
	default:
		return "-";
	}
}

std::string getPacketInputTypeString(enum pkt2::InputType inputtype, int size) {
	switch (inputtype) {
	case pkt2::INPUT_NONE:
		return "";
	case pkt2::INPUT_DOUBLE:
		switch (size) {
			case 4:
				return "float";
			case 8:
				return "double";
			default:
				return "";
		}
		break;
	case pkt2::INPUT_INT:
		switch (size) {
			case 1:
				return "int8_t";
			case 2:
				return "int16_t";
			case 4:
				return "int32_t";
			case 8:
				return "int64_t";
			default:
				return "";
		}
		break;
	case pkt2::INPUT_UINT:
		switch (size) {
			case 1:
				return "uint8_t";
			case 2:
				return "uint16_t";
			case 4:
				return "uint32_t";
			case 8:
				return "uint64_t";
			default:
				return "";
		}
		break;
	case pkt2::INPUT_BYTES:
		return "unsigned char[" + toString(size) + "]";
	case pkt2::INPUT_CHAR:
		return "char[" + toString(size) + "]";
	case pkt2::INPUT_STRING:
		return "char[" + toString(size) + "]";
	default:
		return "-";
	}
}

const std::map<std::string, std::string> types_mysql = {
	{"primary", "int(20) NOT NULL PRIMARY KEY AUTO_INCREMENT"},
	{"id", "int(20)"},
	{"clause_set_charset", "SET CHARSET \'utf8\';"},

	{"int32", "int(11)"},
	{"int64", "int(20)"},
	{"uint32", "int(11)"},
	{"uint64", "int(20)"},
	{"double", "double"},
	{"float", "float"},
	{"bool", "tinyint(1)"},
	{"enum", "enum"},
	{"string", "text"},
	{"message", ""}
};

const std::string quote_postgresql("\"");

const std::map<std::string, std::string> types_postgresql = {
	{"primary", "SERIAL PRIMARY KEY"},
	{"id", "int(20)"},
	{"clause_set_charset", "SET NAMES \'UTF8\';"},

	{"int32", "integer"},
	{"int64", "bigint"},
	{"uint32", "numeric(11)"},
	{"uint64", "numeric(20)"},
	{"double", "double precision"},
	{"float", "real"},
	{"bool", "boolean"},
	{"enum", "enum"},
	{"string", "text"},
	{"message", ""}
};

Pkt2CodeGenerator::Pkt2CodeGenerator(const std::string& name)
{
	// sqltypes = types_mysql;
	// quote = quote_mysql;
	sqltypes = types_postgresql;
	quote = quote_postgresql;
}

Pkt2CodeGenerator::~Pkt2CodeGenerator()
{
}

void Pkt2CodeGenerator::listOne2Many(const google::protobuf::FileDescriptor* file, std::map<const google::protobuf::Descriptor*, messagetypes*> *repeatedmessages)
{
	// create for all messages list of used repeated messages
	for (int i = 0; i < file->message_type_count(); i++)
	{
		messagetypes *mt = new messagetypes();
		repeatedmessages->insert(std::pair<const google::protobuf::Descriptor*, messagetypes*>(file->message_type(i), mt));
	}

	// search for repeated only
	for (int i = 0; i < file->message_type_count(); i++)
	{
		const google::protobuf::Descriptor *m = file->message_type(i);
		for (int f = 0; f < m->field_count(); f++)
		{
			const google::protobuf::FieldDescriptor *fd = m->field(f);
			if ((!fd->is_repeated()) || (fd->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE))
				continue;

			const google::protobuf::Descriptor *mt = fd->message_type();
			messagetypes *ls = repeatedmessages->find(mt)->second;
			ls->push_back(m);
		}
	}
}

std::string getSuffix(const google::protobuf::FieldDescriptor *fd)
{
	std::stringstream ss;
	if (fd->is_required())
		ss << " NOT NULL ";
	if (fd->has_default_value())
	{
		ss << " DEFAULT ";
		switch (fd->cpp_type())
		{
		case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
			ss << fd->default_value_int32();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
			ss << fd->default_value_int64();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
			ss << fd->default_value_uint32();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
			ss << fd->default_value_uint64();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
			ss << fd->default_value_double();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
			ss << fd->default_value_float();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
			ss << fd->default_value_bool();
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
		{
			ss << "'" << fd->default_value_enum()->number() << "'";
		}
		break;
		case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
			ss << "'" << fd->default_value_string() << "'";
			break;
		case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
			ss << "0";
			break;
		default:
			ss << "0";
		}
	}
	return ss.str();
}

// sort by offset
std::vector<int> sortPacketFieldsIndex(
		const google::protobuf::RepeatedPtrField<pkt2::Field > &value)
{
	std::vector<std::pair<int, int>> idxOfs;
	for (int i = 0; i < value.size(); i++)
	{
		idxOfs.push_back(std::pair<int, int>(i, value[i].offset()));
	}

	std::sort(idxOfs.begin(), idxOfs.end(), [] (std::pair<int, int> const& a, std::pair<int, int> const& b) { return a.second < b.second; });

	std::vector<int> idx;
	for (int i = 0; i < value.size(); i++)
	{
		idx.push_back(idxOfs[i].first);
	}
	return idx;
}

void messageOptionsToInputPacketStructDeclaration(
		std::ostream &strm,
		const google::protobuf::Descriptor *m
)
{
	const google::protobuf::MessageOptions options = m->options();
	pkt2::Packet packet =  options.GetExtension(pkt2::packet);
	pkt2::Address src = packet.source();
	strm << "/*" << std::endl;
	strm << " * Input packet: " <<  std::endl
			<< " * Name:  " << packet.name() << std::endl
			<< " * Full:  " << packet.full_name() <<  std::endl
			<< " * Short: " << packet.short_name() <<  std::endl
			<< " * Source: "
			<< getProtoName(src.proto()) << ":" << src.address() <<  ":" << src.port() <<  " "
			<< std::endl;

	strm << "Packet fields:" << std::endl;
	const std::vector<int> idxs = sortPacketFieldsIndex(packet.fields());

	strm << spaces(' ', 41) << "Offset   Bytes Type                 Name" << std::endl;
	int ofs = 0;
	for (int f = 0; f < packet.fields_size(); f++)
	{
		pkt2::Field field = packet.fields(idxs[f]);
		int sz = field.size();
		strm
			<< spaces(' ', field.offset()) << spaces('|', sz) << spaces(' ', 40 - field.offset() - sz)
			<< std::right
			<< std::setw(7) << field.offset() << " "
			<< std::setw(7) << field.size() << " "
			<< std::setw(20) << std::left << getPacketInputTypeString(field.type(), field.size()) << " "
			<< field.name() << " "
			<< std::endl;
		ofs += sz;
	}
	strm << "*/" << std::endl << std::endl;

	strm << CLAUSE_INCLUDES << std::endl << std::endl;

	strm << "typedef struct " << packet.name() << " {" << std::endl;
	for (int f = 0; f < packet.fields_size(); f++)
	{
		pkt2::Field field = packet.fields(idxs[f]);
		int sz = field.size();
		strm
			<< "    " << std::left << std::setw(18)
			<< getPacketInputTypeString(field.type(), field.size()) << " "
			<< field.name() << ";"
			<< std::endl;
		ofs += sz;
	}
	strm << "} " << packet.name() << ";" << std::endl << std::endl;

	strm << std::endl;

}

// write input packet structure reader recursively
void fieldOptionsToInputPacketReader
(
		std::ostream &strm,
		const google::protobuf::Descriptor *md,
		const google::protobuf::FieldDescriptor *value,
		std::string prefix
)
{
	const google::protobuf::MessageOptions message_options = md->options();
	pkt2::Packet packet = message_options.GetExtension(pkt2::packet);
	const google::protobuf::FieldOptions options = value->options();
	pkt2::Variable variable =  options.GetExtension(pkt2::variable);

	// struct
	if (value->message_type())
	{
		const google::protobuf::Descriptor *v = value->message_type();
		prefix = prefix + "->" + "mutable_" + v->name();
		for (int f = 0; f < v->field_count(); f++)
		{
			const google::protobuf::FieldDescriptor *fd = v->field(f);
			fieldOptionsToInputPacketReader(strm, md, fd, prefix);
		}
	}
	else
	{
		std::string f = variable.get();

		if (findWord(f, CLAUSE_RETURN_VALUE))
		{
			replaceWords(f, CLAUSE_RETURN_VALUE, prefix + "->set_" + variable.name() + "(");
			strm << "    "
				<< f << std::endl;
		}
		else
		{
			replacePacketFieldNames(f, &packet);
			strm << prefix + "->set_" << variable.name() << "("
				<< f << ");" << std::endl;
		}
	}
}

void messageOptionsToInputPacketReader
(
		std::ostream &strm,
		const google::protobuf::Descriptor *value
)
{
	const google::protobuf::MessageOptions options = value->options();
	pkt2::Packet packet =  options.GetExtension(pkt2::packet);

	strm << "bool struct_" << packet.name() << "_to_" << value->name()
			<< "("
			<< value->name() << " *dest"
			<< ", "
			<< "struct " << packet.name() << " *src"
			<< ") {"
			<< std::endl;
	for (int f = 0; f < value->field_count(); f++)
	{
		const google::protobuf::FieldDescriptor *fd = value->field(f);
		fieldOptionsToInputPacketReader(strm, value, fd, "    dest");
	}
	strm << "}" << std::endl;
}


bool Pkt2CodeGenerator::generateSQL(
	const google::protobuf::FileDescriptor* file,
	const std::string& parameter,
	google::protobuf::compiler::GeneratorContext* context,
	std::string* error
) const
{
	google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(file->name() + ".sql"));
	google::protobuf::io::Printer printer(output.get(), '$');

	std::map<const google::protobuf::Descriptor*, messagetypes*> repeatedmessages;
	listOne2Many(file, &repeatedmessages);

	std::stringstream ss;
	ss << sqltypes.at("clause_set_charset") << std::endl << std::endl;
	
	ss << "/*" << std::endl;
	for (int i = 0; i < file->message_type_count(); i++)
	{
		const google::protobuf::Descriptor *m = file->message_type(i);
		ss << "DROP TABLE IF EXISTS " << quote << m->name() << quote << ";" << std::endl;
	}
	ss << "*/" << std::endl << std::endl;
	printer.PrintRaw(ss.str());

	// Each message
	for (int i = 0; i < file->message_type_count(); i++) 
	{
		const google::protobuf::Descriptor *m = file->message_type(i);

		std::stringstream ss;
		ss << "CREATE TABLE IF NOT EXISTS " << quote << m->name() << quote << "(" << std::endl <<
				" " << quote << "id" << quote <<
				" " << sqltypes.at("primary") << "," << std::endl;


		// foreign keys for repeated
		messagetypes *fk = repeatedmessages.at(file->message_type(i));
		for (messagetypes::iterator it(fk->begin()); it != fk->end(); ++it)
		{
			const google::protobuf::Descriptor *d = *it;
			std::string lcn(d->name());
			std::transform(lcn.begin(), lcn.end(), lcn.begin(), ::tolower);
			std::string lcmn(file->message_type(i)->name());
			std::transform(lcmn.begin(), lcmn.end(), lcmn.begin(), ::tolower);

			ss << " " << quote << lcn << "id" << quote << " " << sqltypes.at("id") << " NOT NULL," << std::endl;
			ss << "  KEY " << quote << "key_" << lcmn << "_" << lcn << "id" << quote << " ("
					<< quote << lcn << "id" << quote << ")," << std::endl;
		}

		for (int f = 0; f < m->field_count(); f++)
		{
			const google::protobuf::FieldDescriptor *fd = m->field(f);

			google::protobuf::FieldDescriptor::CppType ct = fd->cpp_type();
			switch (ct)
			{
				case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
					if (!fd->is_repeated())
					{
						ss << " " << quote << fd->lowercase_name() << "id" << quote << " " << sqltypes.at("id") << getSuffix(fd) << ", " << std::endl;
						std::string lcmn(m->name());
						std::transform(lcmn.begin(), lcmn.end(), lcmn.begin(), ::tolower);
						ss << " KEY " << quote << "key_" << lcmn << "_" << fd->lowercase_name() << "id" << quote
								<< " (" << quote << fd->lowercase_name() << "id" << quote << "), " << std::endl;
						ss << "  CONSTRAINT " << quote << "fk_" << lcmn << "_" << fd->lowercase_name() << "id" << quote << " FOREIGN KEY("
								<< quote << fd->lowercase_name() << "id" << quote << ") REFERENCES " << quote
								<< fd->message_type()->name() << quote << "(" << quote << "id" << quote << ") ON DELETE CASCADE," << std::endl;
					}
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
					{
						ss << "  " << quote << fd->lowercase_name() << quote << " ";
						ss << sqltypes.at(fd->cpp_type_name()) << "(";
						const google::protobuf::EnumDescriptor *ed = fd->enum_type();
						for (int ei = 0; ei < ed->value_count(); ei++)
						{
							const google::protobuf::EnumValueDescriptor *ev = ed->value(ei);
							ss << "'" << ev->number() << "'";
							if (ei < ed->value_count() - 1)
								ss << ", ";
						}
						ss << ")" << getSuffix(fd) << ", " << std::endl;
					}
					break;
				default:
					ss << " " << quote << fd->lowercase_name() << quote << " " << sqltypes.at(fd->cpp_type_name()) << getSuffix(fd) << ", " << std::endl;
			}
		}
		ss << " " << quote << "tag" << quote << " int(11)" << std::endl;
		ss << ");" << std::endl << std::endl;
		printer.PrintRaw(ss.str());
	}

	if (printer.failed())
	{
		*error = "Pkt2CodeGenerator detected write error.";
		return false;
	}
	return true;
}

bool Pkt2CodeGenerator::generateOptions(
	const google::protobuf::FileDescriptor* file,
	const std::string& parameter,
	google::protobuf::compiler::GeneratorContext* context,
	std::string* error
) const
{
	google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(file->name() + ".options"));
	google::protobuf::io::Printer printer(output.get(), '$');

	std::map<const google::protobuf::Descriptor*, messagetypes*> repeatedmessages;
	listOne2Many(file, &repeatedmessages);

	std::stringstream ss;
	ss << "/*"	<< std::endl << " * This is an automatically generated file, do not edit." << std::endl
			<< " * " << file->name() << ".options, source file: " << file->name() << std::endl
			<< " * " << timeToString(0) << std::endl
			<< " * " << file->dependency_count() << " dependencies: " << std::endl;

	for (int i = 0; i < file->dependency_count(); i++)
	{
		ss << " *     "	<< file->dependency(i)->name() << std::endl;
	}


	ss << " * " << file->message_type_count() << " messages: " << std::endl;
	for (int i = 0; i < file->message_type_count(); i++)
	{
		const google::protobuf::Descriptor *m = file->message_type(i);
		ss << " *     " << m->name() << std::endl;
	}
	ss << " */" << std::endl << std::endl;

	// Each message
	for (int i = 0; i < file->message_type_count(); i++)
	{
		const google::protobuf::Descriptor *m = file->message_type(i);

		messageOptionsToInputPacketStructDeclaration(ss, m);

		messageOptionsToInputPacketReader(ss, m);
	}

	printer.PrintRaw(ss.str());
	if (printer.failed()) 
	{
		*error = "Pkt2CodeGenerator generateOptions detected write error.";
		return false;
	}
	return true;
}


bool Pkt2CodeGenerator::Generate(
		const google::protobuf::FileDescriptor* file,
		const std::string& parameter,
		google::protobuf::compiler::GeneratorContext* context,
		std::string* error)  const
{
	bool r = generateSQL(file, parameter, context, error);
	r &= generateOptions(file, parameter, context, error);

	return r;
}

