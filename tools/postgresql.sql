CREATE TABLE users (
	u_id bigserial NOT NULL,
	login varchar(32) NOT NULL,
	password varchar(32) NOT NULL,
	salt varchar NOT NULL,
	"name" varchar NULL,
	patron varchar NULL,
	family varchar NULL,
	mail varchar NULL,
	"comment" text NULL,
	accessdb varchar(8) NULL,
	proctablefieldflag1 int4 NOT NULL DEFAULT '-1'::integer,
	rawtablefieldflag int4 NOT NULL DEFAULT '-1'::integer,
	proctablefieldflag2 int4 NOT NULL DEFAULT '-1'::integer,
	phone varchar NULL,
	dms bool NULL,
	"start" int4 NULL DEFAULT date_part('epoch'::text, now()),
	finish int4 NULL,
	CONSTRAINT u_pkey PRIMARY KEY (u_id),
	CONSTRAINT u_unic UNIQUE (login)
);

CREATE TABLE dev (
	id bigserial NOT NULL,
	userid int8 NOT NULL,
	"instance" text NOT NULL DEFAULT ''::text,
	state int4 NOT NULL DEFAULT 1,
	created int4 NOT NULL DEFAULT date_part('epoch'::text, now()),
	updated int4 NOT NULL DEFAULT date_part('epoch'::text, now()),
	"name" text NOT NULL DEFAULT ''::text,
	notes text NOT NULL DEFAULT ''::text,
	send int4 NOT NULL DEFAULT 0,
	recv int4 NOT NULL DEFAULT 0,
	CONSTRAINT dev_pkey PRIMARY KEY (id),
	CONSTRAINT fk_dev_usrid FOREIGN KEY (userid) REFERENCES users(u_id) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE UNIQUE INDEX idx_dev_instance ON dev USING btree (instance);

CREATE TABLE device_description (
	id bigserial NOT NULL,
	imei int4 NOT NULL,
	device_name varchar NULL,
	device_serial_number varchar NULL,
	device_description varchar NULL,
	legend varchar NULL,
	owner int4 NOT NULL,
	color varchar NULL,
	"current" bool NOT NULL DEFAULT true,
	edit_time timestamp NOT NULL DEFAULT (now() + '10:00:00'::interval),
	CONSTRAINT pk_dd PRIMARY KEY (id),
	CONSTRAINT fk2_dd FOREIGN KEY (owner) REFERENCES users(u_id),
	CONSTRAINT fk_dd FOREIGN KEY (imei) REFERENCES devices(id)
);

CREATE TABLE devices (
	id bigserial NOT NULL,
	imei varchar NOT NULL,
	created_time timestamp NOT NULL DEFAULT (now() + '10:00:00'::interval),
	CONSTRAINT pk_d PRIMARY KEY (id),
	CONSTRAINT uni_d UNIQUE (imei)
);

INSERT INTO users
(u_id, login, password, salt, "name", patron, family, mail, "comment", accessdb, proctablefieldflag1, rawtablefieldflag, proctablefieldflag2, phone, dms, "start", finish)
VALUES(20, 'msktest', '4169ef7421fb8d0b4cc1f8c49ece3981', 'msktest', 'Тестовый', 'Тестовый', 'Тестовый', 'kiv@ikfia.ysn.ru', '2018-02-21 09:49:22', 'oper', 1572896, 255, 2310, NULL, true, 1518966000, 1546268400);

INSERT INTO devices
(id, imei, created_time)
VALUES(73, '300234069105060', '2018-01-10 22:35:08.846');

INSERT INTO device_description
(id, imei, device_name, device_serial_number, device_description, legend, owner, color, "current", edit_time)
VALUES(277, 73, '1806', 'J064RU', 'SBDR9602F02', NULL, 20, '#bf00bf', true, '2018-02-21 19:51:01.766');

INSERT INTO dev
(id, userid, "instance", state, created, updated, "name", notes, send, recv)
VALUES(5, 20, 'dnTVve-KAxg:APA91bEzNLT62cYeYYGot8bpumY9iMMEUavG8LGjbrbnE50F55h3A8owZ3snYQa8ns1XcfztmLQPpxDmPh2a5VKDvH4WHaCvdmvb7z-PpROsqxK4WBXtlq6GUK_N2wvHvvvAbwYRTke5', 1, 1519613382, 1519613382, 'Мобильный пользователь', '', 0, 0);

