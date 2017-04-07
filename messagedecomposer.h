/*
 * messagedecomposer.h
 *
 * @brief Decompose google::protobuf::Message
 *
 */

#ifndef MESSAGEDECOMPOSER_H_
#define MESSAGEDECOMPOSER_H_

#include <google/protobuf/message.h>

#include "pkt2optionscache.h"

/**
  * Parse Protobuf message
  * @brief "Parse" message using callback
  */
class MessageDecomposer {
	typedef void (*ondecompose_callback)(
		void* env,
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor::CppType field_type,
		const std::string &field_name,
		void* value,
		int size,
		int index
	);
private:
	void* env;
	ondecompose_callback ondecompose;
	Pkt2OptionsCache *options_cache;
	int decomposeField
	(
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::Message *message, 
		const google::protobuf::FieldDescriptor *field
	);
protected:
	/// Decompose with no index
	int decompose
	(
		const google::protobuf::Message *message
	);
public:
	MessageDecomposer();
	MessageDecomposer(void *env, Pkt2OptionsCache *options, const google::protobuf::Message *message, ondecompose_callback callback);
	virtual ~MessageDecomposer();
	void setCallback(ondecompose_callback ondecompose);

	/**
	  * @brief return human readable value of message buffer as string
	  */
	static std::string toString
	(
		google::protobuf::FieldDescriptor::CppType field_type,
		void* value,
		int size
	);
};

#endif /* MESSAGEDECOMPOSER_H_ */