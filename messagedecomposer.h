/*
 * messagedecomposer.h
 *
 * @brief Decompose google::protobuf::Message
 *
 */

#ifndef MESSAGEDECOMPOSER_H_
#define MESSAGEDECOMPOSER_H_

#include <google/protobuf/message.h>

/**
  * @brief "Parse" message using callback
  */
class MessageDecomposer {
	typedef void (*ondecompose_callback)(
		void* env,
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor::CppType field_type,
		const std::string &field_name,
		void* value,
		int size
	);
private:
	void* env;
	ondecompose_callback ondecompose;
	int decomposeField
	(
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::Message *message, 
		const google::protobuf::FieldDescriptor *field
	);
public:
	MessageDecomposer();
	MessageDecomposer(void *env, const google::protobuf::Message *message, ondecompose_callback callback);
	virtual ~MessageDecomposer();
	void setCallback(ondecompose_callback ondecompose);
	int decompose(const google::protobuf::Message *message);
	/**
	  * @brief return human readable value as string 
	  */
	static std::string toString(
		google::protobuf::FieldDescriptor::CppType field_type,
		void* value,
		int size
	);
};

#endif /* MESSAGEDECOMPOSER_H_ */
