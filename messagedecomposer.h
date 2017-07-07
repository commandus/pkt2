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
	JavascriptContext *context;

	/**
	 * Add "message.field" javascript object to the context
	 * @param message message to decompose
	 * @param field field descriptor
	 */
	static void addJavascriptField
	(
		duk_context *context,
		const google::protobuf::Message *message,
		const google::protobuf::FieldDescriptor *field
	);

	/**
	 * Add "message.field" javascript default object to the context
	 * @param message message to decompose
	 * @param field field descriptor
	 */
	static void addJavascriptFieldDefault
	(
		duk_context *context,
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
	bool own_context;
public:
	Pkt2OptionsCache *options_cache;

	/**
	 * @brief message decompser
	 * @param context can be NULL
	 */
	MessageDecomposer
	(
		void *env, 
		JavascriptContext *context,
		Pkt2OptionsCache *options, 
		const google::protobuf::Message *message, 
		ondecompose_callback callback
	);
	
	/**
	* @brief message decompser
	*/
	MessageDecomposer
	(
		void *environment, 
		const std::string &messagetypename,
		Pkt2OptionsCache *options, 
		const google::protobuf::Message *message, 
		ondecompose_callback callback
	);

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
	
	/**
	  * @brief set "output" fields in the packet
	  * @param field Protobuf field descriptor
	  * @param value value to set in human readable text representation
	  */
	static void setJsValue
	(
		duk_context *context,
		const google::protobuf::FieldDescriptor *field,
		const std::string& value
	);

	/**
	* @brief get "input" fields in the packet
	* @param field_name field name
	* @return value in human readable text representation
	*/
	static std::string getJsField
	(
		duk_context *context,
		const std::string &field_name
	);

	/**
	* @brief set "input" fields in the packet
	* @param packet where set bits
	* @param message_descriptor Protobuf message descriptor
	* @param field Protobuf field descriptor
	* @param value value to set in human readable text representation
	*/
	static void setFieldsByVariable
	(
		duk_context *context,
		Pkt2OptionsCache *options_cache,
		Pkt2PacketVariable *packetVariable,
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor *field,
		const std::string& value
	);

	static std::string reflectJsFieldsToMessagePacket
	(
		const std::string& packet,
		duk_context *context,
		Pkt2OptionsCache *options_cache,
		const Pkt2PacketVariable *packetVariable
	);

	static std::string reflectJsFieldsToMessagePacketField
	(
		const std::string& packet,
		duk_context *context,
		Pkt2OptionsCache *options_cache,
		const pkt2::Field &field
	);

	/**
	 * @brief Add "message" javascript object to the context
	 * @param message message to decompose
	 */
	static void addJavascriptMessage
	(
		duk_context *context,
		const google::protobuf::Message *message
	);


};

#endif /* MESSAGEDECOMPOSER_H_ */
