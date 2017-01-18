pkt2
====

История изменений
-----------------

2017/01/11 Черновик

Назначение
----------

Разбор приходящих пакетов.

Сервер соержит список  поддерживаемых протоколов.

Протокол содержит 

- описание входящего пакета данных, опционально- адреса источника данных и адреса, на который пришел пакет.
- описание извлекаемых в БД данных
- описание исходящего пакета данных (ответа), опционально- адрес куда отправить пакет.

```
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
Трансмимттер              Передатчик        Композер          |             |         +->| Обработчик 2|
 +-------+    +----+    +-----------+    +--------------+    +-------+    +---------+    +-------------+
 |       |<---|    |<---| исх.пакет |<---| "Переменные" |    |Входн. |<---| Запись  |<---|             |
 +-------+    +----+    +-----------+    +--------------+    +-------+    +---------+    +-------------+
                        | uint16 a  |    | float a = x+y|    | x     |                          
                        | uint32 b  |--->| uint16 b = x |    | y     |                          
                        | uint32 c  |    | uint32 c = z |    | z     |                  
                        +-----------+    +--------------+    +-------+                  
                                                                                        

Пррограммы

tcpreceiver                  pkt2receiver                                  pkt2gateway       handlerpq
udpreceiver

Имена каналов(очередей) по умолчанию
           ipc:///tmp/input.pkt2                                           ipc:///tmp/output.pkt2

```

## Определение протокола

Протокол отпределяется по Источнику

Описание структуры пакетов

Пакет
  Название пакета
    Извлекаемые данные
    Источник
    Входящий пакет
        элемент структуры 1
        элемент структуры 2
        ...
        элемент структуры N

### Название пакета

name            имя для файлов, имен переменных (лат.)
short_name      отображаемое имя
full_name       описание

### Источник

src_proto = tcp | udp
src_addr = IPv4
src_port = 0..
dst_addr = IPv4
dst_port = 0..

Если адрес не указан или равен 0, "0.0.0.0", то пакет с любым адресом.

Если порт не указан или равен 0, то пакет с любым портом.

Если ни один протокол не найден по Источнику, пакет записывается в таблицу необработанных пакеты.

Если Источник дает более одного протокола, поиск делается перебором зарегистрированных протоколов с подходящими источниками.

Если Источник дает более чем одлин протокол, вызывается процедура предварительной обработки пекета каждого проткола, до тех пор, пока процедура не вернет признак валидности пакета.

Процедура предварительной обработки пекета возвращает признак валидности. Если пакет вадиден, он записывается в базу данных.

Если ни один протокол не сообщает о валидности пакета, пакет записывается в таблицу необработанных пакеты.

### Выходный адрес

Тоже самое

### Входящий пакет (PDU)

Элемент структуры имеет имя, на которое может ссылаться элемент извлекаемых (выходных) данных

- name               имя переменной (лат). Это имя может использоваться для получения значений в  
- type               тип переменной в пакете (например, UINT8) (не больше size)
- endian             дополнительные признаки для преобразоваия типа: BIG_ENDIAN,..
- offset             расположение относительно родительского элемента
- size               длина в байтах

Если нужно маскировать, сдвигать биты- это указывется в формуле выходгых данных.

###  Выходные данные

- name               имя переменной (лат). Это имя может использоваться для получения значений в  
- type               тип переменной в единицах измерения (например, FLOAT)
- short_name         отображаемое короткое имя
- full_name          описание
- measure_unit       название единицы измерения (если не задан values)
- formula            формула приведения значения к единице измерения) (если не задан values). Применяются name в вычислениях.
- values             строки для флагов или перечислений по порядку. Если задан, measure_unit и formula не действуют.
- priority           уровень детализации отображения. 0 (высший)- отображать всегда (по умолчанию), 1- не отображать (не записывать в БД)
- format             формат преобразования в строку (для определенных типов, например, 8.2f для указания точности)

#### Элемент values

cn                 отображаемое имя флага или перечисления, например, мотор
on                 например, "вкл."
off                например, "выкл."

parent             родительский элемент, относительно которого задается положение в пакете (NULL для пакета без вложений)

### Тип элемента

- NONE           массив байт
- DOUBLE         плавающая запятая
- INT            знаковое целое
- UINT           беззнаковое целое
- CHAR           8-битный символ, байт
- STRING         NULL-terminated строка символов

#### Тип переменной (src_type, dst_type)

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

tcpemitter tcpreceiver pkt2receiver pkt2gateway handlerpq tcptransmitter


