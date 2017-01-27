SET NAMES 'UTF8';

/*
DROP TABLE IF EXISTS "TemperaturePkt";
*/

CREATE TABLE IF NOT EXISTS "TemperaturePkt"(
 "id" SERIAL PRIMARY KEY,
 "degrees_c" numeric(11), 
 "degrees_f" numeric(11), 
 "tag" int(11)
);

