/*
 * messagedecomposer.h
 *
 * @brief Decompose google::protobuf::Message
 */

#ifndef MESSAGEDECOMPOSER_H_
#define MESSAGEDECOMPOSER_H_

#include <google/protobuf/message.h>


class MessageDecomposer {
	typedef void (*ondecompose_callback)(
		google::protobuf::FieldDescriptor::CppType fieldType,
		void* retval,
		int size
	);
private:
	ondecompose_callback ondecompose;
	int decompose(const google::protobuf::Message *message);
	int decomposeField(const google::protobuf::Message *message, const google::protobuf::FieldDescriptor *field);
public:
	MessageDecomposer();
	MessageDecomposer(const google::protobuf::Message *message);

	virtual ~MessageDecomposer();
};

#endif /* MESSAGEDECOMPOSER_H_ */
