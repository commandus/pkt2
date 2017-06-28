/**
  *	pkt2 configuration file.
  * Sections:
  * - listeners: 
  * 	- tcpreceiver			слушает TCP порт, передает полученные пакеты в шину пакетов
  * 	- mqtt-receiver			подписывается на указанные топики, передает полученные пакеты из брокера msqt в шину пакетов
  * - pkt2receiver				чтение пакетов из шины пакетов, нахождение протокола, отправка сообщения в шину сообщений
  * - handlers
  * 	- handlerline			помещение сообщений в stdout lля последующей обработки скриптами
  * 	- handlerlmdb			помещение сообщений в базу данных LMDB
  * 	- handlerpq				помещение сообщений в базу данных PostgreSQL
  * 	- handler-google-sheets	помещение сообщений в электронную таблицу Google Sheets
  */

/* 
 * Global settings
 */

/**
 * Proto file root directory . Default "proto"
 */
proto_path = "proto";

/**
 * Force max file (socket) descriptors per process. 0- use system default (1024)
 */
max_file_descriptors = 0;

/**
 * Set buffer size for packet. Default 4096 bytes.
 */
max_buffer_size = 4096;

/**
 * Set bus for incoming packets. Default "ipc:///tmp/packet.pkt2"
 */
bus_in = "ipc:///tmp/packet.pkt2";

/**
 * Set bus for outgoing messages. Default "ipc:///tmp/message.pkt2"
 */
bus_out = "ipc:///tmp/message.pkt2";

/**
 * Set logging verbosity: 0- none, 1- error/warn, 2- info
 */
verbosity = 2;

//------------------------- Listeners -------------------------------

//------------------------- Handlers -------------------------------
/**
 * Handler, write to file
 * messages: filter by message names to write. Empty: any message
 * mode: 0- JSON, 1-CSV, 2- tab, 3- SQL, 4- SQL(2)
 * file: file to write
 */
write_file = 
[
	{
		"messages":
		[
			"iridium.IE_Packet"
		],
		"mode": 0,
		"file": "1.txt"
	}
];
