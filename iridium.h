/*
 * iridium.h
 *
 */

#ifndef IRIDIUM_H_
#define IRIDIUM_H_

#include <inttypes.h>

#ifdef _MSC_VER
#define ALIGN   __declspec(align(1))
#define PACKED
#else
#define ALIGN
#define PACKED  __attribute__((aligned(1), packed))
#endif

typedef ALIGN struct {
        uint8_t id;     // Iridium version
        uint16_t size;
} PACKED IridiumHeader;

typedef ALIGN struct {
        uint8_t b[15];
} PACKED IMEI;

// IE_IO 0x01, 28
typedef ALIGN struct {
        uint32_t cdrref;        //< Iridium message identifier                                  4
        IMEI imei;              //< see IMEI                                                    15
        uint8_t status;         //< see IOStatus                                                1
        uint16_t sentno;        //< momsn modem sent out packet number                          2
        uint16_t recvno;        //< mtmsn modem received out packet number. Not used            2
        uint32_t recvtime;      //< stime Unix epoch time, seconds since 1970                   4
} PACKED IEIOHeader;

/* MO Location data IE_LOCATION 0x03 11 bytes long
 * Dir byte
 * Bits 0-3 reserved
 * Bits 4 5 format code
 * Bit 6 0- North, 1- South
 * Bit 7 0- East, 1- West
 */
typedef ALIGN struct {
        uint8_t direwi : 1;
        uint8_t dirnsi : 1;
        uint8_t dirformat : 2;
        uint8_t dirreserved : 4;
        uint8_t lat;            //< 1 Latitude
        uint16_t lat1000;       //< 2 1/1000 latitude
        uint8_t lon;            //< 4 Longitude
        uint16_t lon1000;       //< 5 1/1000 longitude
        uint32_t cepradius;     //< 7 CEP radius: 80% probability radius
} PACKED IELocation;

typedef ALIGN struct
{
	uint8_t latitude_g;	    // 0	    [3](0-90)	добавить бит N or S 0х80 (0х80 - данные корректны)
	uint8_t latitude_m;	    // 1        [4](0-60)	или добавить бит N or S 0х80, E or W (East or West) 0х40
	//unsigned long int latitude_s;       //3  (6 bytes (48 bit) число до 281 474 976 710 656) size=32
	uint16_t latitude_s;	// 2-3      [5-6] 2 байта
	//   unsigned long ?? size =   long int 32 -2147483648 to 2147483647    unsigned long int size=32 0 to 4 294 967 295
	uint16_t latitude_s1;	// 4-5      [7-8]7 reserved
	uint8_t latitude_ns;	// 6 N/S    [9]   0x4E или 0x53
	uint8_t longitude_g;	// 7(0-180) [10]
	uint8_t longitude_m;	// 8(0-60)  [11]
	//unsigned long int longitude_s;     //11  (6 bytes (48 bit) число до 281 474 976 710 656) unsigned long int size=32
	uint16_t longitude_s;	// 9-10      [12-13]
	uint16_t longitude_s1;	// 11-12     [14-15]
	uint8_t longitude_ew;	// 13 E or W    0x45 или 0x57
	uint8_t hdop;		    // 14 HDOP, m, 0-99
	uint8_t pdop;	        // 15 PDOP, m, 0-99
} PACKED gps_coord_t;

// 5 bytes time
// 2 байта год/месяц/день
//	7 бит год           0b xxxx xxx0
//	4 бита месяц        0b x xxx0 0000
//	5 бит день          0b 000x xxxx
typedef ALIGN struct {
	uint16_t day: 5;
	uint16_t month: 4;
	uint16_t year: 7;

	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} PACKED time5;

typedef ALIGN struct {
	uint8_t packettype;		    //< 0) 1 байт 0х08 - координаты с NMEA приемников LAT, LON,
	uint8_t gpsolddata:1;		//< 1)GPS old data, GPS not read
	uint8_t gpsencoded:1;		//< GPS data encoded
	uint8_t gpsfrommemory:1;	//< data got from the memory
	uint8_t gpsnoformat:1;		//< memory is not formatted
	uint8_t gpsnosats:1;		//< No visible GPS salellites
	uint8_t gpsbadhdop:1;		//< GPS HDOP = 99 or > 50
	uint8_t gpstime:1;		    //< 1 бит 1-0  GPS.valid=0x01 или 0;     GPS.time_valid=0b0000 0010 или 0;
	uint8_t gpsnavdata:1;		//< 1 бит 1-0  GPS.valid=0x01 или 0;     GPS.time_valid=0b0000 0010 или 0;
	uint8_t sats;		        //< 2) 1 байт  кол. видимых спутников
	gps_coord_t coord;		    //< 3) 16 байт (или 10 байт??? ) hex ?
	uint8_t bat:6;			    //< 19) байт бортовое напряжение fixed point (32=3.2 вольта) (от 2.0 до 4.0)
	uint8_t alarmlow:1;	        //< bit 6 < 2V, bit 7 > 4V
	uint8_t alarmhigh:1;		//< bit 6 < 2V, bit 7 > 4V
	int8_t temperature;	        //< 20) 1 байт температура
	uint8_t r2;			        //< 21) 1 байт номер пакета?? - он же индекс в таблице кодировки
	uint8_t failurepower:1;		//< 22) device status power loss
	uint8_t failureeep:1;		//< EEPROM failure
	uint8_t failureclock:1;		//< clock not responding
	uint8_t failurecable:1;		//< MAC not responding
	uint8_t failureint0:1;		//< clock int failure
	uint8_t failurewatchdog:1;	//< software failure
	uint8_t failurenoise:1;		//< blocks in memory found
	uint8_t failureworking:1;	//< device was
	uint16_t key;			    // 23) 2 байт     volatile unsigned int packet_key;  младшие 16 бит
	// uint8_t		 res[3];
	time5 time;                 // 25 5 байт
	// uint32_t crc;
} PACKED packet8_t;

#endif /* IRIDIUM_H_ */
