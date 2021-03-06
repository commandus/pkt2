#ifndef PKT2_CODE_GENERATOR_H
#define PKT2_CODE_GENERATOR_H	1

#include <string>
#include <map>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>

typedef std::vector<const google::protobuf::Descriptor*> messagetypes;

class Pkt2CodeGenerator : public google::protobuf::compiler::CodeGenerator {
private:
	static void listOne2Many(const google::protobuf::FileDescriptor* file, std::map<const google::protobuf::Descriptor*, messagetypes*> *repeatedmessages);
	std::map<std::string, std::string> sqltypes;
	std::string quote;
	bool generateSQL(
			const google::protobuf::FileDescriptor* file,
			const std::string& parameter,
			google::protobuf::compiler::GeneratorContext* context,
			std::string* error) const;
	bool generateOptions(
			const google::protobuf::FileDescriptor* file,
			const std::string& parameter,
			google::protobuf::compiler::GeneratorContext* context,
			std::string* error) const;
public:
		Pkt2CodeGenerator(const std::string& name);
		virtual ~Pkt2CodeGenerator();
		virtual bool Generate(
				const google::protobuf::FileDescriptor* file,
				const std::string& parameter,
				google::protobuf::compiler::GeneratorContext* context,
				std::string* error) const;
};

#endif
