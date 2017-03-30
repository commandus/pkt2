/*
 * messagecomposer.h
 *
 * @brief Compose google::protobuf::Message
 *
 */

#ifndef MESSAGECOMPOSER_H_
#define MESSAGECOMPOSER_H_

#include <google/protobuf/message.h>
#include <google/protobuf/reflection.h>

#include "pkt2optionscache.h"

/**
  * Compose Protobuf message
  * @brief "Parse" message using callback
  */
class MessageComposer {
	typedef bool (*oncompose_field)(
		void* env,
		const google::protobuf::Descriptor *message_descriptor,
		const google::protobuf::FieldDescriptor::CppType field_type,
		const std::string &message_name,
		const std::string &field_name,
		bool repeated,
		int index,
		std::string &retval
	);

	typedef bool (*on_next_message)(
		void* env,
		google::protobuf::Message* message,
		int index
	);

private:
	void* env;
	oncompose_field onComposeField;
	on_next_message onNextRepeatMessage;
	Pkt2OptionsCache *optionCache;
	int composeField
	(
		const google::protobuf::Descriptor *message_descriptor,
		google::protobuf::Message *message,
		const google::protobuf::FieldDescriptor *field
	);
protected:
	/// compose
	bool compose
	(
		google::protobuf::Message *message,
		bool repeated,
		int index
	);
public:
	MessageComposer();
	MessageComposer
	(
			void *env,
			Pkt2OptionsCache *options,
			google::protobuf::Message *message,
			oncompose_field callback_field,
			on_next_message callback_nextmessage
	);
	virtual ~MessageComposer();
	void setCallbacks
	(
			oncompose_field oncomposefield,
			on_next_message onnextmessage
	);

};

#endif /* MESSAGECOMPOSER_H_ */
