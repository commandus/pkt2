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
		const int field_number,
		bool repeated,
		int index,	
		std::string &retval
	);

	typedef bool (*on_message_begin)(
		void* env,
        const google::protobuf::FieldDescriptor *field,
		google::protobuf::Message* message,
        bool repeated,
		int index
	);

   	typedef bool (*on_message_end)(
		void* env,
		google::protobuf::Message* message,
        bool repeated,
		int index
	);

private:
	void* env;
	oncompose_field onComposeField;
    on_message_begin onMessageBegin;
    on_message_end onMessageEnd;

	const Pkt2OptionsCache *optionCache;
    
    int composeField
	(
		google::protobuf::Message *message,
		const google::protobuf::FieldDescriptor *field
	);
protected:
	/// compose
	bool compose
	(
        const google::protobuf::FieldDescriptor *field,
		google::protobuf::Message *message,
		bool repeated,
		int index
	);
public:
	MessageComposer();
	MessageComposer
	(
			void *env,
			const Pkt2OptionsCache *options,
			google::protobuf::Message *message,
			oncompose_field callback_field,
            on_message_begin callback_message_begin,
            on_message_end callback_message_end
	);
	virtual ~MessageComposer();
	void setCallbacks
	(
		oncompose_field oncomposefield,
        on_message_begin onmessagebegin,
        on_message_end onmessageend
	);
    
};

#endif /* MESSAGECOMPOSER_H_ */
