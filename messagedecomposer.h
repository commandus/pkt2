/*
 * messagedecomposer.h
 *
 * @brief Decompose google::protobuf::Message
 *
 */

#ifndef MESSAGEDECOMPOSER_H_
#define MESSAGEDECOMPOSER_H_

#include <google/protobuf/message.h>
#include "javascript-context.h"
#include "pkt2optionscache.h"

/**
  * Parse Protobuf message
  * @brief "Parse" message using callback
  */
class MessageDecomposer {
	typedef void (*ondecompose_callback)(
		MessageDecomposer *decomposer,
		void* env,
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor *field_descriptor,
		void* value,
		int size,
		int index
	);
private:
	void* env;
	JavascriptContext context;

	/**
	 * @brief Add "message" javascript object to the context
	 * @param message message to decompose
	 */
	void addJavascriptMessage
	(
		const google::protobuf::Message *message
	);
	/**
	 * Add "message.field" javascript object to the context
	 * @param message message to decompose
	 * @param field field descriptor
	 */
	void addJavascriptField
	(
		const google::protobuf::Message *message,
		const google::protobuf::FieldDescriptor *field
	);
	ondecompose_callback ondecompose;
	int decomposeField
	(
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::Message *message, 
		const google::protobuf::FieldDescriptor *field
	);
	
	int decomposeMessage
	(
		const google::protobuf::Message *message
	);

protected:
	/// Decompose with no index
	int decompose
	(
		const google::protobuf::Message *message
	);

	/**
	  * @brief format
	  */
	std::string format
	(
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor *field,
		const std::string& value,
		int format_number
	);
public:
	Pkt2OptionsCache *options_cache;

	MessageDecomposer(void *env, Pkt2OptionsCache *options, const google::protobuf::Message *message, ondecompose_callback callback);
	virtual ~MessageDecomposer();

	/**
	  * @brief return human readable value of message buffer as string
	  */
	std::string toString
	(
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor *field,
		const void* value,
		int size,
		int format_number
	);
};

#endif /* MESSAGEDECOMPOSER_H_ */
