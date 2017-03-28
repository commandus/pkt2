/*
 * packet2message.cpp
 *
 *  Created on: Mar 24, 2017
 *      Author: andrei
 */

#include <string.h>
#include <algorithm>

#include <glog/logging.h>

#include "duk/duktape.h"

#include "packet2message.h"
#include "pkt2packetvariable.h"
#include "platform.h"
#include "errorcodes.h"
#include "utilstring.h"

Packet2Message::Packet2Message(
		const std::string &protopath,
		int verbose
)
	: verbosity(verbose)
{
	declarations = new ProtobufDeclarations(protopath, verbosity);
	options_cache = new Pkt2OptionsCache(declarations);
}

Packet2Message::~Packet2Message() {
	delete options_cache;
	delete declarations;
}

/**
 * Get field value from the packet
 * @param packet
 * @param field
 * @return
 */
std::string extractField
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	int sz = packet.size();
	if (sz < field.offset() + field.size())
	{
		LOG(ERROR) << ERR_PACKET_TOO_SMALL << field.name() << " " << field.offset() << " " << field.size() << " " << sz;
		return "";
	}
	std::string r = packet.substr(field.offset(), field.size());
	if (ENDIAN_NEED_SWAP(field.endian()))
	{
		char *p = &r[0];
		std::reverse(p, p + field.size());
	}
	return r;
}

/**
 * Get field value from the packet as 32 bit integer
 * @param packet
 * @param field
 * @return
 */
uint32_t extractFieldUInt
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 0:
		return 0;
	case 1:
		return *((uint8_t*) &r[0]);
	case 2:
		return *((uint16_t*) &r[0]);
	case 4:
		return *((uint32_t*) &r[0]);
	default:
	{
		uint64_t v = 0;
		memmove(&v, &r[0], (sz < sizeof(uint64_t) ? sz : sizeof(uint64_t)));
		return v;
	}
	}
	return 0;
}

/**
 * Get field value from the packet as double
 * @param packet
 * @param field
 * @return
 */
double extractFieldDouble
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 4:
		return *((float*) &r[0]);
	case 8:
		return *((double*) &r[0]);
	}
	return 0;
}


void pushField
(
		duk_context *ctx,
		const std::string &packet,
		const pkt2::Field &field
)
{
	switch (field.type()) {
		case pkt2::INPUT_NONE:
			duk_push_false(ctx);
			break;
		case pkt2::INPUT_CHAR:
		case pkt2::INPUT_STRING:
			duk_push_string(ctx, packet.substr(field.offset(), field.size()).c_str());
			break;
		case pkt2::INPUT_DOUBLE:
			duk_push_number(ctx, extractFieldDouble(packet, field));
			break;
		case pkt2::INPUT_BYTES:
		{
			duk_idx_t arr_idx = duk_push_array(ctx);
			int c = 0;
			for (int i = field.offset(); i < field.size(); i++)
			{
				duk_push_int(ctx, packet[i]);
				duk_put_prop_index(ctx, arr_idx, c);
				c++;
			}
			break;
		}
		default:
			duk_push_uint(ctx, extractFieldUInt(packet, field));
			break;
	}

}

static duk_ret_t native_print
(
		duk_context *ctx
)
{
        duk_push_string(ctx, " ");
        duk_insert(ctx, 0);
        duk_join(ctx, duk_get_top(ctx) - 1);
        printf("%s\n", duk_safe_to_string(ctx, -1));
        return 0;
}

duk_context *makeJavascriptContext
(
		const Pkt2PacketVariable &pv,
		const std::string &packet
)
{
	duk_context *ctx = duk_create_heap_default();

	duk_push_c_function(ctx, native_print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");

	// field--
	duk_push_global_object(ctx);
	duk_push_object(ctx);

	for (int i = 0; i < pv.packet.fields_size(); i++)
	{
		const pkt2::Field &f = pv.packet.fields(i);
		pushField(ctx, packet, f);
		duk_put_prop_string(ctx, -2, f.name().c_str());
	}
	duk_put_global_string(ctx, "field");

	/*
	for (int i = 0; i < pv.fieldname_variables.size(); i++)
	{
		const FieldNameVariable &v = pv.fieldname_variables[i];

		duk_push_int(ctx, extractFieldUInt(packet, v));
		duk_put_prop_string(ctx, -2, v.field_name.c_str());
	}
	*/

	// --field
	return ctx;
}

/**
 * Get variable from the packet as string
 * @param packet
 * @param variable
 * @return
 */
std::string extractVariable
(
	const Pkt2PacketVariable &pv,
	const FieldNameVariable &variable,
	const std::string &packet
)
{
	// js(pv, variable, packet);
	return variable.var.get();
}

/**
 *
 * @param packet
 * @param force_message packet.message or "" (no force)
 * @return
 */google::protobuf::Message *Packet2Message::parse
(
	const std::string &packet,
	const std::string &force_message
)
{
	bool found;
	const Pkt2PacketVariable &pv = (!force_message.empty()
			? options_cache->getPacketVariable(force_message, &found)
					: options_cache->getPacketVariable("example1.TemperaturePkt", &found));

	if (!found)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << force_message;
		return NULL;
	}

	duk_context *jsCtx = makeJavascriptContext(pv, packet);
	if (verbosity >= 2)
	{
		// packet input fields
		for (int i = 0; i < pv.packet.fields_size(); i++)
		{
			pkt2::Field f = pv.packet.fields(i);
			LOG(INFO) << f.name() << ": " << hexString(extractField(packet, f));
		}
		// packet output variables
		for (int i = 0; i < pv.fieldname_variables.size(); i++)
		{
			FieldNameVariable v = pv.fieldname_variables[i];
			LOG(INFO) << v.field_name << "(" << v.var.full_name() << ") = " << extractVariable(pv, v, packet) << v.var.measure_unit();
		}

	}

	duk_eval_string(jsCtx, "print(field.device);print(field.unix_time);print(field.value);");

	duk_pop(jsCtx);
	duk_destroy_heap(jsCtx);

	return NULL;	
}
