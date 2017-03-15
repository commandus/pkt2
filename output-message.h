#include <string>
#include <inttypes.h>

#ifndef OUTPUT_MESSAGE_H
#define OUTPUT_MESSAGE_H

/**
 * Key 16 bytes long
 * uint32 Идентификатор входного пакета
 * uint32 Помеченное как ключ (метка времени)
 * uint64 Идентификатор(ы) устройства
 *
 */

typedef struct OutputMessageKey {
	uint32_t packet_type_id;
	uint32_t time_stamp;
	uint64_t device_id;
} OutputMessageKey;

#endif
