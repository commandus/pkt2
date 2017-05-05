pkt2
====

История изменений
-----------------
2017/04/25 pkt2dumppq
2017/01/18 Пример описания
2017/01/11 Черновик

Назначение
----------

Разбор приходящих TCP/IP пакетов.

Каждому пакету необходимо его описание (протокол).

Протокол содержит 

- описание входящих данных(входящего пакета), опционально- адреса источника данных и адреса назначения.
- описание извлекаемых(выходных) данных, опционально- формат для проедставления в виде строки.

Описания (протокол) записываются на языке Protobuf как опции языка:

- option(pkt2.output) входящие данные 
- (pkt2.variable) выходные данные 

в файле с расширением .proto в папке proto.

Одному пакету данных должно быть сопоставлено одно сообщение (message), записанное в файле .proto в колировке UTF-8.

Например, файл proto/example/example1.proto начинается с:

```
syntax = "proto3";
package example1;
import "pkt2.proto";
/// Temperature
message TemperaturePkt
...
```

Каждое сообщение (message) относится к пакету. Полное имя сообщения в примере: example1.TemperaturePkt.

## Поля(fields)
В сообщении(message) опция pkt2.packet содержит поля(fields) пакета. 

Каждое поле- это непрерывная область памяти. Для поля указывается смещение и размер в байтах.

В случае, если данные в пакете разбиты на несколько областей, или содержат битовые поля, можно задавать поля с перекрытием. 

Важно, чтобы по смещению и размеру полей можно было определить размер пакета. Если пакет содержит в конце неиспользуемые(пустые) данные, 
нужно задать поле, чтобы прошоамма моглда определить размер пакета.


```
	option(pkt2.packet) = {
    	id: 5001 
        name: "temperature"
        short_name: "Температура"
        full_name: "DEVICE TEMP"
        set: "device = message.device; unix_time = message.time; value = (message.degrees_c / 2) / 1.22);"
        source: {
            proto: PROTO_TCP
            address: "84.237.104.57"
            port: 50052 // 0- any port
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
            endian: ENDIAN_BIG_ENDIAN
        },
        {
            name: "value"
            type: INPUT_UINT
            size: 2
            offset: 5
            endian: ENDIAN_BIG_ENDIAN
        },
        {
            name: "tag"
            type: INPUT_UINT
            size: 1
            offset: 7
            tag: 255
        }
        ]
    };
```

Для полей, занимающих более 1 байта, можно указывать порядок байт в слове как в примере: endian: ENDIAN_BIG_ENDIAN. 

Когда программа будет читать из полей значения чисел, будет применяться восстановление порядка байт.

Чтобы различать пакеты по содержимому, поля с фиксированным значением нужно помечать, как в примере: tag:255. 
Для полей длиной более 1 байта нужно указать endian.

Например:
```
message IE_IOHeader
{
    // input packet description
    option(pkt2.packet) = { 
        name: "ioheader"
        short_name: "Iridium IO header"
        full_name: "Iridium message IO header"
        fields: [
        {
            name: "ie_id"
            type: INPUT_UINT
            size: 1
            offset: 0
            tag: 1			      ///< tag 1- I/O header
        },
        {
            name: "ie_size"
            type: INPUT_UINT
            size: 2
            offset: 1
            endian: ENDIAN_BIG_ENDIAN
            tag: 28               ///< tag, 28 bytes long
        },
        {
```
тег (1) и фиксированый размер тела (28 байт) могут считаться признаком того, что пакет с тегами 1 и 28 - заголовок IOHeader. 

Так как поле ie_size двухбайтное, то для него указывается, что порядок байт- сетевой: endian: ENDIAN_BIG_ENDIAN

## Выходные данные(variables)

Поля указывают только на области памяти, откуда могут быть извлечены данные.

Приложение указывает, какие данные ему нужны для дальнейшей обработки в опции pkt2.variable

В примере:
```
double degrees_c = 3 [(pkt2.variable) = {
        name: "degrees_c"
        short_name: "Температура"
        full_name: "Температура"
        measure_unit: "C"
        get: "1.22 * field.value"
        priority: 0                                 // required
        format: ["value.degrees_c.toFixed(2).replace('.', ',')"]
    }];
```

double degrees_c указывает на то, что мы лдолжнгы извлечь пеерменную с числом с плавающей запятой.

В опции pkt2.variable.get указывается, что мы должны извлечь число из поля value на языке Javascript.

field- это объект, содержащий поля:

- field.device
- field.unix_time
- field.value
- field.tag (всегда содержит число 255)

Формула:

```
1.22 * field.value
```

ссылается на поле value через объект field.

## Запись

Для записми в базу данных нужно описание выходгных данных снабдить индексами
Ключ:

- uint32 Идентификатор входного пакета
- uint32 Помеченное как ключ (метка времени)
- uint64 Идентификатор(ы) устройства


## Преобразование в строку(форматирование)

В предыдущем примере format опции pkt2.variable:
```
value.degrees_c.toFixed(2).replace('.', ',')
```
использует объект value, содержащий зачения переменных заданного типа (в примере это double).

Точно также, как в случае с get, format требьует указаия вложенным сообщений. Например:
```
    // output
    uint32 time5 = 1 [(pkt2.variable) = {
        short_name: "Время"
        full_name: "Время, Unix epoch"
        measure_unit: "s"
        get: "new Date(field.packet8.time5.day_month_year & ((1 << 7) - 1) + 2000, (field.packet8.time5.day_month_year >> 7) & ((1 << 4) - 1) - 1, (field.packet8.time5.day_month_year >> (7 + 4)) & ((1 << 5) - 1), field.packet8.time5.hour, field.packet8.time5.minute, field.packet8.time5.second, 00).getTime() / 1000" 
        priority: 0
        format: ["var d = new Date(value.packet8.time5.time5 * 1000); ('0' + d.getDate()).slice(-2) + '.' + ('0' + (d.getMonth() + 1)).slice(-2) + '.' + d.getFullYear() + ' ' + ('0' + d.getHours()).slice(-2) + ':' + ('0' + d.getMinutes()).slice(-2)"]
    }];
```

format ссылается на вложенное сообщение time5 вложенного сообщения packet8.

## Вложенные сообщения

Пакет можно расписать в одной файле, или разбить его на логические части.

Например, пакет Iridium можно разбить на несколько уровней- уровень входящего пакета, и входящих его вложенных сообщений.

Поля во всех, втом числе вложенных сообщениях, имеют начальное смещение 0. Реальное смещение вычисляется по размеру вложенным сообщений при работе программы(без выравнивания, если нужно выравнивание, добавьте в конец пустые байты).

В следующем примере сообщение packet8- вложенное:

```
	bool battery_low = 13 [(pkt2.variable) = {
        name: "battery_low"
        short_name: "низкое бортовое напряжение"
        get: "field.packet8.battery & 0x40"
    }];
```
В таком случае надо указать вложенность:
```
field.packet8.battery
```
вместо
```
field.battery
```

Это нужно потому, что уникальность имен ограничена сообщением, и тогда возможно совпадение имен.

## Файлы .proto

Когда программы начинают выполнение, загружаются все файлы .proto из папки (включая вложенные папки) пакетов. Сообщения об ошибка записывабтся в журнал.

Каждое сообщение содержит атрибуты целого, вещественного тиов, реже типов перечислений и строк. Сообщения могут быть вложенными. 

В языке Protobuf есть понятие опций сообщений и атрибутов. 

В этом приложении опции сообщения используются для записи структуры входящих пакетов, а опции атрибутов- описание того, как значение атрибута получается из входного пакета.

В опции сообщения pkt2.packet записываются смещения и размеры полей входного пакета и присваивается имя.

В опции атрибута pkt2.variable записывается код функции преобразования. 

Два атрибута должны помечаться в опциях как индексные поля времени и номера устройства для всех сообщений, которые планируется записывать в базу данных. 
Вложенные сообщения не должны иметь индексы, перекрывающие индексы сообщений, куда они вложены.  

Если файлы .proto изменены, а программы уже запущены, то процессам надо послать сигнал 1:

```
kill -1 <номер процесса>
```
чтобы программы перезапустились с чтением обновленных .proto файлов.

## Шины

Входящие пакеты принимаются одно или несколькими процессами программы tcpreceiver.

tcpreceiver слушает порт, и все всходящие в порт пакеты пересылаются в шину пакетов(на диаграмме
внизу- ipc:///tmp/packet.pkt2.

Адрес шины ipc:///tmp/packet.pkt2 применим для межпроцессного взаимодействия в пределах одного компьютера через разделяюмую память.

Для организации шины между несколькими  нужно использовать адрес tcp: или udp: (обратитесь к документации http://nanomsg.org/)

К шине пакетов подключается один или несколько процессов программы pkt2receiver.

pkt2receiver осуществляет поиск подходящего проткола по имеющимся у него .proto файлам:
- по длине пакета
- по нахождению тегов(маркеров пакета) в сообщенгия и вложеных сообщениях

Когда протокол найден, формируется сообщение из значений пакета и передается далее в шину сообщений.

Так как процесс сопоставления пакетов сообщениям может оказаться затратным, можно запустить несколько процессов pkt2receiver с разделением по размеру пакета.

К шине сообщений подключаются программы- обработчики. Программы- обработчикам можно указать олин из способов:

- обрабатывать все сообщения
- фильтровать по имени сообщения

Программы обработчики использую .proto файл в основном для того, чтобь получить значение format для форматирования результата в виде строки.

В фильтре по имении сообщения можно задать один или несколько сообщений по полноми имени сообщения (иимя пакета и имя сообщения).

## Дампер

Программа pqdumppq читает пакеты из шины пакетов и записывает их в базу данных напрямую.

## Диаграмма обработки

```
               Шина пакетов                                          Шина сообщений
               ipc:///tmp/packet.pkt2                                ipc:///tmp/message.pkt2
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
 |  TCP  |--->| очередь |--->| вх. пакет  |--->| Поля  |    | Перемен.|  | | Запись |
 +-------+    +---------+    +------------+    +-------+    +---------+  | +--------+
 +-------+        |                   |        | int x |    | int sum |  | | sum    |    +-------------+
 |  UDP  |--------+                   |        | int y |--->| = x + y |--->| x1     |--->| Обработчик 1|
 +-------+                            |        | int z |    |         |  | | z      | |  +-------------+
                                      |        +-------+    +---------+  | +--------+ |  |             |
                                      |               Описание|          +--+         |  +-------------+
                                      |               исх.пак.|             |         |  +-------------+
Трансмиттер               Передатчик  |     Композер          |             |         +->| Обработчик 2|
 +-------+    +----+    +-----------+ |  +--------------+    +-------+    +---------+    +-------------+
 |       |<---|    |<---| исх.пакет |<---| "Переменные" |    |Входн. |<---| Запись  |<---|             |
 +-------+    +----+    +-----------+ |  +--------------+    +-------+    +---------+    +-------------+
                        | uint16 a  | |  | float a = x+y|    | x     |                          
                        | uint32 b  |--->| uint16 b = x |    | y     |                          
                        | uint32 c  | |  | uint32 c = z |    | z     |                  
                        +-----------+ |  +--------------+    +-------+                  
                                      |                                                  
                                      |  +---------+                                     +----+
			                          +->|  Дампер |------------------------------------>| БД |
			                             +---------+                                     +----+
Программы
[tcpemitter-iridium]
[tcpemitter-example1]
[tcpemitter]
              tcpreceiver                       pkt2receiver               [pkt2gateway]     handlerpq             
              mqtt-receiver                                                [message2gateway] handler-google-sheets  
                                                                           [example1message] handlerline
                                                                                             handlerlmdb
```                                                                                             
В квадратных скобках ([]) тестирующие программы

Назначение программ:

- tcpreceiver			слушает TCP порт, передает полученные пакеты в шину пакетов
- mqtt-receiver			подписывается на указанные топики, передает полученные пакеты из брокера msqt в шину пакетов
- pkt2receiver 			чтение пакетов из шины пакетов, нахождение протокола, отправка сообщения в шину сообщений
- handler-google-sheets	помещение сообщений в электронную таблицу Google Sheets
- handlerline			помещение сообщений в stdout lля последующей обработки скриптами
- handlerlmdb			помещение сообщений в базу данных LMDB
- handlerpq				помещение сообщений в базу данных PostgreSQL
- handler-google-sheets	помещение сообщений в электронную таблицу Google Sheets

### Вспомогательные программы

#### Помещение готовых сообщений в шину сообщений

message2gateway			читает сериализованные (бинарные) сообщения из сокета или stdin и перемещает их в шину сообщений без адресов откуда и куда пришли

messageemitter			читает сообщения в формате JSON из сокета или stdin и перемещает их в шину сообщений без адресов откуда и куда пришли

#### Генераторы случайных данных

tcpemitter-example1		отправялет в TCP/IP порт пакеты (на вход tcpreceiver) со случайными значениями для примера example/example1.proto

tcpemitter-iridiun		отправялет в TCP/IP порт пакеты (на вход tcpreceiver) со случайными значениями примера iridim/animals.proto

tcpemitter				читает пакеты из файла (hex) или stdin и перемещает их в шину пакетов

#### Разное 

protoc-gen-pkt2			плагин компилятора protoc для создания SQL скриптов

example1message			отправляет в stdout сериализованные (бинарные) сообщения для примера example/example1.proto (можно подать на вход прогшраммы tcpemitter)

example1message1		отправляет в stdout одно сообщение без сериализованных полногшо имени сообщения, его длины, адресов. Не используется.

## Порядок запуска и останова

Сначала запускаются публикаторы, поэтому нужно запускать ресиверы, затем message2gateway, и потом обработчики (handler*)(по схеме слева направо).

Останов делается в обратном порядке (по схеме справо налево) от обработчиков к ресиверам.

Если по какой то причине остановить программу в левой части схемы или  message2gateway, все программы правее должны быть перезапущены.

Обработчики (самые правые), как конечные потребители, можно перезапускать  

## Описание выходных данных


### Значения по умолчанию

tcpreceiver TCP по умолчанию порт 50052

Имена каналов(очередей) по умолчанию:

- ipc:///tmp/packet.pkt2
- ipc:///tmp/message.pkt2

                                                                           
### Тесты


#### example1message1

Сериализует в stdout одно случайное сообщение (сообщение TemperaturePkt, файл описания example/example1.proto)

```
./example1message1 > 1
codex -protofile proto/example/example1.proto -message_name TemperaturePkt 1
```

#### pkt2gateway

Отправляет подготовленные заранее тестовые сообщения в очередь обработчиков ipc:///tmp/message.pkt2 

#### tcpemitter-example1

Отправляет TCP пакеты как в примере 1.
 
По умолчанию шлет в порт 50052.

Один экземпляр tcpreceiver должен быть запущен. По умолчанию он слушает порт 50052.

#### tcpemitter-iridium

Отправляет TCP пакеты Iridium пакет 8
 
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

## Поля

В field

field.name

В format можно указать значение: 

value

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

## Запуск программ и контроль

Опция --maxfd позволяет увеличивать максимальное количество одновременно открытых дескрипторов- сокетов.

Значение по умолчанию для Linux- 1024. 

```
./tcpreceiver --maxfd 100000
```
 
### Запуск демонов

Опция -d демонизирует процессы. Создаются файлы /var/run/<имя программы>.pid

#### Файлы с номерами процессов демонов 

PID файлы создаются только при наличии прав на запись в каталоге /var/run/: 

/var/run/tcpreceiver.pid

то есть демон нужно запускать от имени root.

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

### handler-goole-sheets

#### Удаление (смена) пароля сертификата с приватным ключом сервиса Google Sheets

```
tools/p12-remove-password cert/pkt2-sheets.p12 
Enter Import Password:[notasecret]
MAC verified OK
Enter Export Password:
Verifying - Enter Export Password:
```

## Ошибки

### Ошибка открытия сокета

Socket connect error localhost:50052. Cannot assign requested address 

Переполнение стека TCP/IP из-за того, что сервис не успевает обрабатывать данные из сокета.

### Ошибки доступа IPC
 
Operation not permitted [1] (...bipc.c:309)

E0324 13:26:53.405812 16817 tcpreceivernano.cpp:114] Can not connect to the IPC url ipc:///tmp/packet.pkt2

```
sudo chown <user>:<group> /tmp/packet.pkt2
```
## Тесты

Запуск генератора пакета example/example1 и слушателя TCP 
```
./tcpreceiver -vv & ./tcpemitter-example1 -vv && fg
```


## Баги и особенности реализации

### nanomsg

[Issue 182](https://github.com/nanomsg/nanomsg/issues/182)

При отключении потока публикатора (PUB) нужно пересоединить сокеты подписчиков (SUB)

Предположение: если поставить sleep(0) в публикаторе, то вроде бы работает. 

### Eclipse

Работает странно, лучше исптользоать KDevelop

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

### Репозиторий и сборка

Репозиторий /media/dept/Конструкторский отдел/Repo/pkt2

Сборка:

```
tar xvfz pkt2-0.1.tar.gz
cd pkt2-0.1
./configure
make
```

#### DEBUG

```
./configure --enable-debug
```

### Запуск с зависимостями

```
ssh nova.ysn.ru
export LD_LIBRARY_PATH=/home/andrei/pkt2/lib
...
```

-rwxr-xr-x 1 andrei andrei    97332 Apr 23 19:40 libargtable2.so.0

-rwxr-xr-x 1 andrei andrei   740633 Apr 23 19:41 libglog.so.0

-rwxr-xr-x 1 andrei andrei   266483 Apr 23 19:42 liblmdb.so

-rwxr-xr-x 1 andrei andrei   389454 Apr 23 19:43 libnanomsg.so.5.0.0

-rwxr-xr-x 1 andrei andrei   306544 Apr 23 19:44 libnetsnmpagent.so.20

-rwxr-xr-x 1 andrei andrei   153544 Apr 23 19:53 libnetsnmphelpers.so.20

-rwxr-xr-x 1 andrei andrei  1687840 Apr 23 19:55 libnetsnmpmibs.so.20

-rwxr-xr-x 1 andrei andrei   676416 Apr 23 19:45 libnetsnmp.so.20

-rwxr-xr-x 1 andrei andrei  1485896 Apr 23 19:46 libperl.so

-rwxr-xr-x 1 andrei andrei 22389022 Apr 23 19:37 libprotobuf.so.11

-rwxr-xr-x 1 andrei andrei   394799 Apr 23 19:48 libunwind.so.8

