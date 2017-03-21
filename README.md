pkt2
====

История изменений
-----------------

2017/01/18 Пример описания
2017/01/11 Черновик

Назначение
----------


Разбор приходящих пакетов.

Сервер соержит список  поддерживаемых протоколов.

Протокол содержит 

- описание входящего пакета данных, опционально- адреса источника данных и адреса, на который пришел пакет.
- описание извлекаемых в БД данных (выходных данных)
- описание исходящего пакета данных (ответа), опционально- адрес куда отправить пакет.

Выходные данные записываются для последующей обработки в базы данных обработчиками на языке Protobuf в виде сообщений (message).

Каждое сообщение содержит атрибуты целого, вещественного тиов, реже типов перечислений и строк. Сообщения могут быть вложенными. 

В языке Protobuf есть понятие опций сообщений и атрибутов. 

В этом приложении опции сообщения используются для записи структуры входящих пакетов, а опции атрибутов- описание того, как значение атрибута получается из входного пакета.

В опции сообщения pkt2.packet записываются смещения и размеры полей входного пакета и присваивается имя.

В опции атрибута pkt2.variable записывается код функции преобразования. 

Два атрибута должны помечаться в опциях как индексные поля времени и номера устройства для всех сообщений, которые планируется записывать в базу данных. 
Вложенные сообщения не должны иметь индексы, перекрывающие индексы сообщений, куда они вложены.  



```
               ipc:///tmp/packet.pkt2                                      ipc:///tmp/message.pkt2
                                              Файлы
                                            (плагины)
                                          поддерживаемых
                                            протоколов
                                          +------------+
                                          | Протоколы  |
                                          +------------+
                                          | Протокол 1 |
                                          | ...        |
                                          | Протокол N |-----------------+
                                          +------------+  Описание       |
                                                ^        входящего       |
                                   Определение  |         пакета         |
                                    протокола   |            |           |
                                                |            |           |
  Ресиверы     Сообщение        Приемник          Парсер    Процессор-   | Сообщение           Шлюз
                очередь                                     сериализатор |  очередь
 +-------+    +---------+    +------------+    +-------+    +---------+  | +--------+     
 |  TCP  |--->| очередь |--->| вх. пакет  |--->| Блоки |    | Выходн. |  | | Запись |    
 +-------+    +---------+    +------------+    +-------+    +---------+  | +--------+    
 +-------+        |                            | int x |    | int sum |  | | sum    |    +-------------+
 |  UDP  |--------+                            | int y |--->| = x + y |--->| x1     |--->| Обработчик 1|
 +-------+                                     | int z |    |         |  | | z      | |  +-------------+
                                               +-------+    +---------+  | +--------+ |  |             |
                                                      Описание|          +--+         |  +-------------+
                                                      исх.пак.|             |         |  +-------------+
Трансмиттер               Передатчик        Композер          |             |         +->| Обработчик 2|
 +-------+    +----+    +-----------+    +--------------+    +-------+    +---------+    +-------------+
 |       |<---|    |<---| исх.пакет |<---| "Переменные" |    |Входн. |<---| Запись  |<---|             |
 +-------+    +----+    +-----------+    +--------------+    +-------+    +---------+    +-------------+
                        | uint16 a  |    | float a = x+y|    | x     |                          
                        | uint32 b  |--->| uint16 b = x |    | y     |                          
                        | uint32 c  |    | uint32 c = z |    | z     |                  
                        +-----------+    +--------------+    +-------+                  
                                                                                        

Программы

tcpreceiver                  pkt2receiver                                  pkt2gateway       handlerpq
udpreceiver                                                                message2gateway   
                                                                           example1message   handlerline
tcpemitter                                                                                   handlerlmdb
tcpemitter-example1    
    

## Описание выходных данных


### Порты

tcpreceiver TCP 50052
                                                                           
### Тесты


#### example1message1

Сериализует в stdout одно случайное сообщение (сообщение TemperaturePkt, файл описания example/example1.proto)

```
./example1message1 > 1
codex -protofile proto/example/example1.proto -message_name TemperaturePkt 1
```

Имена каналов(очередей) по умолчанию
           ipc:///tmp/packet.pkt2                                           ipc:///tmp/message.pkt2

```

#### tcpemitter-example1

Отправляет TCP пакеты как в примере 1.
 
По умолчанию шлет в порт 50052.

Один экземпляр tcpreceiver должен быть запущен. По умолчанию он слушает порт 50052.


## Запись

Ключ:

- uint32 Идентификатор входного пакета
- uint32 Помеченное как ключ (метка времени)
- uint64 Идентификатор(ы) устройства

Значение:

- Сообщение(сериализованное


## Определение протокола

Описание протокола делается с на языке описания сериализатора сообщений protobuf: proto3 (https://developers.google.com/protocol-buffers/)
с использованием расширения (опций).

Опция pkt2.packet содержит описание структуры пакета данных:

- полей
- их смещений и размеров
- адреса, откуда приходят пакеты

и располагается внутри message (сериалитзуемого сообщения)

Опция pkt2.variable содержит 

- описание того, как она извлекается из пакета данных
- формат строки для перевода в читаемый текст

и располагается внутри поля message. Тип поля может быть выбрано из поддерживаемых protobuf типов.

## Пример описания

```
syntax = "proto3";

package example1;

import "pkt2.proto";    // описание расширения (опций pkt2.packet, pkt2.variable)

/// Temperature
message TemperaturePkt
{
    // input packet description
    option(pkt2.packet) = { 
        name: "temperature"
        short_name: "Температура"
        full_name: "DEVICE TEMP"
        source: {
            proto: PROTO_TCP
            address: "84.237.104.57"
            port: 0 // any port
        }
        fields: [
        {
            name: "device"
            type: INPUT_UINT
            size: 1
            offset: 0
        },
        {
            name: "unix_time"
            type: INPUT_UINT
            size: 4
            offset: 1
            endian: BIG_ENDIAN
        },
        {
            name: "value"
            type: INPUT_UINT
            size: 2
            offset: 5
            endian: BIG_ENDIAN
        }]
    };

    // output 
    uint32 degrees_c = 1 [(pkt2.variable) = {
        name: "degrees_c"
        type: OUTPUT_DOUBLE
        short_name: "Температура"
        full_name: "Температура, C"
        measure_unit: "C"
        formula: "1.22 * ((value & 0x0f) << 1)"
        priority: 0                                 // required
        format: "%8.2f"
    }];

    uint32 degrees_f = 2 [(pkt2.variable) = {
        name: "degrees_f"
        type: OUTPUT_DOUBLE
        short_name: "Температура"
        full_name: "Температура, F"
        measure_unit: "F"
        formula: "degrees_f * 1.8 + 32"
        priority: 1                                 // optional
        format: "%8.2f"
    }];
}
```

## Последовательность, в какой работает pkt2 


Пакет имеет название, описание и Источник (его адрес)

- name            имя для файлов, имен переменных (лат.)
- short_name      отображаемое имя
- full_name       описание
- source.proto = tcp | udp
- source.addr = IPv4
- source.port = 0..

По адресу отправителя отпределяется по источник данных. 

Если адрес source.addr не указан или равен 0, "0.0.0.0", то это пакет, пришедший с любого адреса.

Если порт не указан или равен 0, то это пакет, пришедший с любого порта.

Если один источник данных порождает не один тип пакета, а несколько, то поиск делается перебором зарегистрированных протоколов с подходящими источниками 
до тех пор, пока процедура parse() не вернет признак валидности пакета.

Процедура parse() возвращает признак валидности пакета. Если пакет вадиден, он записывается в базу данных.

Если ни один протокол не сообщает о валидности пакета, пакет записывается в таблицу необработанных пакетов.

Если ни один протокол не найден по Источнику, пакет записывается в таблицу необработанных пакетов.

### Входящий пакет (PDU)

Входящий пакет описывается в опции option(pkt2.packet) сообщения (message).

Элемент структуры имеет имя, на которое может ссылаться элемент извлекаемых (выходных) данных

- name               имя переменной (лат). Это имя может использоваться для получения значений в  
- type               тип переменной в пакете (например, UINT8) (не больше size)
- endian             дополнительные признаки для преобразоваия типа: BIG_ENDIAN,..
- offset             расположение относительно родительского элемента
- size               длина в байтах

Если нужно маскировать, сдвигать биты- это указывется в формуле выходных данных.

###  Выходные данные

Выходные данные описваются как поля mtssage в опции option(pkt2.variable)

- name               имя переменной (лат). Это имя может использоваться для получения значений в  
- type               тип переменной в единицах измерения (например, FLOAT)
- short_name         отображаемое короткое имя
- full_name          описание
- measure_unit       название единицы измерения (если не задан values)
- formula            формула приведения значения к единице измерения) (если не задан values). Применяются name в вычислениях.
- values             строки для флагов или перечислений по порядку. Если задан, measure_unit и formula не действуют.
- priority           уровень детализации отображения. 0 (высший)- отображать всегда (по умолчанию), 1- не отображать (не записывать в БД)
- format             формат преобразования в строку (для определенных типов, например, 8.2f для указания точности)

### Тип элемента структуры пакета

Указывается тип- целое (со знаком или без), вещественное число или последоавтельность символов. 
Вместе со значениями size и endian получается тип.

- NONE           массив байт
- DOUBLE         плавающая запятая
- INT            знаковое целое
- UINT           беззнаковое целое
- CHAR           8-битный символ, байт
- STRING         NULL-terminated строка символов

#### Тип переменной (src_type, dst_type)

Тип выходных переменных из числа поддерживаемых protobuf типов.
При преобразовании в строку используется значение format.
Не для всех типов возможна конверсия.


- DOUBLE
- FLOAT
- INT64
- UINT64
- INT32
- FIXED64
- FIXED32
- BOOL
- STRING
- GROUP
- MESSAGE // Length-delimited aggregate.
- BYTES
- UINT32
- ENUM
- SFIXED32
- SFIXED64
- SINT32
- SINT64

Программы
---------

Программы

tcpemitter tcpreceiver pkt2receiver pkt2gateway handlerpq handlerline tcptransmitter message2gateway

### Примеры

- example1message - тестирующая программа для message2gateway. Генерирует сообщения для записи.


показаны на схеме, для передачи данных друг другу используется передача через именованные разделяемые области памяти
 библиотекой nanomsg (http://nanomsg.org/), эмулирующих межпроцессное взаимодействие с очередями сообщений как с сокетами.
 
### Плагин компилятора protoc protoc-gen-pkt2

Плагин компилятора protoc (компилятор скачать можно тут https://github.com/google/protobuf/releases)

Пример использования protoc-gen-pkt2 в скрипте tests/p1.sh:

```
protoc --proto_path=proto --cpp_out=. proto/pkt2.proto

protoc --proto_path=proto --cpp_out=. proto/example/example1.proto

protoc --proto_path=proto --cpp_out=. proto/iridium/packet8.proto

protoc --plugin=protoc-gen-pkt2="../protoc-gen-pkt2" --proto_path=../proto --pkt2_out=pkt2 ../proto/example1.proto

protoc --plugin=protoc-gen-pkt2="protoc-gen-pkt2" --proto_path=proto --pkt2_out=pkt2 proto/example1.proto

```

#### Опции

- pkt2_out каталог, где будут сохранены сгенерированные файлы 
- plugin имя плагина и путь к его исполнимому файлу

## Опции proto2

- packet
- output
- variable

```
extend google.protobuf.MessageOptions {
    pkt2.Packet packet = 50501;
}

extend google.protobuf.MessageOptions {
    pkt2.Output output = 50502;
}

extend google.protobuf.FieldOptions {
    pkt2.Variable variable = 50503;
}

```

## Программы

### tcpemitter

```
tcpemitter -i localhost -l 50052 << messages.txt
```

Каждая строка должна иметь тип сообщения и значения в формате JSON, разделенный знаком двоеточия ":"

```
Packet.MessageType:{"json-object-in-one-line"}
```

### handlerlmdb

Значения

values 
------
Record# PK



## Баги

```
protoc --proto_path=proto --cpp_out=. proto/pkt2.proto
```

Удалить в pkt2.pb.h

#include "descriptor.pb.h"

pkt2.pb.cpp

пару строк:

::google::protobuf::protobuf_InitDefaults_descriptor_2eproto();

::google::protobuf::protobuf_AddDesc_descriptor_2eproto();


## SNMP
```
cp mib/* ~/.snmp/mibs

snmptranslate -On -m +EAS-IKFIA-MIB -IR pkt2
.1.3.6.1.4.1.46956.1.2
snmptranslate -On -m +EAS-IKFIA-MIB -IR memoryPeak
.1.3.6.1.4.1.46956.1.2.1.1.1.6

smilint -l3  -s -p ./mib/EAS-IKFIA-MIB 

```

#### Cannot find module (SNMPv2-MIB)

```
sudo apt-get install snmp-mibs-downloader snmptrapd
sudo download-mibs
sudo sed -i "s/^\(mibs :\)./#\1/" /etc/snmp/snmp.conf
```

```
mkdir -vp ~/.snmp/mibs
sudo mkdir -p /root/.snmp/mib
cp mib/* ~/.snmp/mibs
sudo cp mib/* /root/.snmp/mib

snmptranslate -On -m +ONEWAYTICKET-COMMANDUS-MIB -IR onewayticketservice
.1.3.6.1.4.1.46821.1.1

smilint -l3  -s -p ./mib/*

snmpget -v2c -c private 127.0.0.1 ONEWAYTICKET-COMMANDUS-MIB::ticketssold.0
ONEWAYTICKET-COMMANDUS-MIB::ticketssold.0 = INTEGER: 0

snmpget -v2c -c private 127.0.0.1 ONEWAYTICKET-COMMANDUS-MIB::memorycurrent.0
ONEWAYTICKET-COMMANDUS-MIB::memorycurrent.0 = INTEGER: 29784
```

#### ERROR: You don't have the SNMP perl module installed.

#### Warning: no access control information configured.

```
Warning: no access control information configured.
  (Config search path: /etc/snmp:/usr/share/snmp:/usr/lib/x86_64-linux-gnu/snmp:/home/andrei/.snmp)
  It's unlikely this agent can serve any useful purpose in this state.
  Run "snmpconf -g basic_setup" to help you configure the onewayticketsvc.conf file for this agent.
```  
```
snmpconf -g basic_setup
```

#### SNMP Error opening specified endpoint "udp:161"

161 привелигирпованный порт, запустить от рута.

```
sudo ...
```

## Отладка

```
./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0'
```

## protoc

```
./example1message1 > 1

protoc -I proto --decode example1.TemperaturePkt proto/example/example1.proto < 1
device: 876648949
time: 1487218294
degrees_c: 22.111469307966281

protoc -I proto --decode_raw  < 1
1: 876648949
2: 1487218294
3: 0x40361c8940a83912

```

### handlerline

Записывает в поток stdout сообщения в текстовом виде. Для отладки.

### handlerpq

Для записи значений в базу данных Postgresql 

Два режима записи:

- 3 SQL "нативный"
- 4 SQL(2) c использованием view.

Предварительно для режима SQL нужно создать таблицы, для которых будуту поступать данные.

Запустите с опцией -vv и остановите (Ctrl+C) программу.  
```
./handlerpq -p proto --host localhost --user onewayticket --database onewayticket --password 123456 -vv
Press Ctrl+C
cat handlerpq.INFO
```

В файле журнала handlerpq.INFO будут записи следующего вида:

```
SQL CREATE TABLE statements
===========================
CREATE TABLE "example1_TemperaturePkt"(INTEGER device, INTEGER time, FLOAT degrees_c, id bigint);
CREATE TABLE "iridium_GPS_Coordinates"(FLOAT latitude, FLOAT longitude, INTEGER hdop, INTEGER pdop, id bigint);
CREATE TABLE "iridium_IE_IOHeader"(INTEGER cdrref, VARCHAR(32) imei, INTEGER status, INTEGER recvno, INTEGER sentno, INTEGER recvtime, id bigint);
CREATE TABLE "iridium_IE_Location"(FLOAT iridium_latitude, FLOAT iridium_longitude, INTEGER cepradius, id bigint);
CREATE TABLE "iridium_IE_Packet"(INTEGER iridium_version, INTEGER size, id bigint);
CREATE TABLE "iridium_Packet8"(INTEGER coordinates, INTEGER measure_time, INTEGER gpsolddata, INTEGER gpsencoded, INTEGER gpsfrommemory, INTEGER gpsnoformat, INTEGER gpsnosats, INTEGER gpsbadhdop, INTEGER gpstime, INTEGER gpsnavdata, INTEGER satellite_visible_count, FLOAT battery_voltage, INTEGER battery_low, INTEGER battery_high, INTEGER temperature_c, INTEGER reserved_2, INTEGER failurepower, INTEGER failureeep, INTEGER failureclock, INTEGER failurecable, INTEGER failureint0, INTEGER software_failure, INTEGER failurewatchdog, INTEGER failurenoise, INTEGER failureworking, INTEGER key, id bigint);
CREATE TABLE "iridium_Time5"(INTEGER date_time, id bigint);
``` 

Предварительно для режима SQL(2) нужно создать как минимум две таблицы:
```
CREATE TABLE num (message VARCHAR(255), time INTEGER, device INTEGER, field VARCHAR(255), value NUMERIC(10, 2));
CREATE TABLE str (message VARCHAR(255), time INTEGER, device INTEGER, field VARCHAR(255), value VARCHAR(255));
```

## Описание работы 

### Генерация кода

Входной пакет 

Message *parse(MessageTypeNAddress *descriptor, void *input_packet)

 

## Баги и особенности реализации

### nanomsg

[Issue 182](https://github.com/nanomsg/nanomsg/issues/182)

При отключении потока публикатора (PUB) нужно пересоедить сокеты подписчиков (SUB)

### Eclipse

Подсветка ошибок (включить c++ 11)

http://stackoverflow.com/questions/39134872/how-do-you-enable-c11-syntax-in-eclipse-neon

- Right click on your project and click Properties
- Navigate to C/C++ General and Preprocessor Include Paths, Macros etc.
- Select the Providers tab, click on compiler settings row for the compiler you use.
- Add -std=c++11 to Command to get compiler specs.

При застревании индексатора кода C/C++ Indexer:
```
rm ~/workspace/.metadata/.plugins/org.eclipse.cdt.core/*
```

#### Пропала иконка в Unity launcher

http://askubuntu.com/questions/80013/how-to-pin-eclipse-to-the-unity-launcher
