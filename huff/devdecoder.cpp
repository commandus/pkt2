static void Buffer_Write_Byte
(
	void *outbuffer,
	int index,
	unsigned char symbol
)
{
	((unsigned char *)outbuffer)[index] = symbol;
}

/**
 * @brief Copy first two bytes(used) as is, decode others to len
 * @param buff_out output buffer
 * @param buff_in input buffer
 * @param len bytes to decode + used(2 bytes)
 * @return count of written to output buffer bytes count
 * @author kiv@ikfia.ysn.ru
 */
unsigned int dec_hafman
(
	void *buff_out, 
	unsigned char *buff_in, 
	unsigned int len
)
{
	// длина в битах символа
	const unsigned char tabl_len[10] = {0x01, 0x04, 0x05, 0x06, 0x07, 0x07, 0x05, 0x05, 0x04, 0x02};
	// убрать (заменить?) 6 со всеми 0 !!
	const unsigned char tabl_haf_revers[10] = {0x01, 0x04, 0x0C, 0x30, 0x10, 0x50, 0x00, 0x1C, 0x08, 0x02};
	// для декомпрессии таблица
	const unsigned char tabl_haf_symbol[10] = {0, 1, 2, 3, 4, 0xfc, 0xfd, 0xfe, 0xff, 0xAA};
	// индекс во входном буфере
	unsigned char j, k;
	unsigned int bufpos = 0;
	// индекс во входном буфере!
	unsigned int bufpos_out = 0;
	// индекс в выходном буфере!
	unsigned int bitbuf = 0;
	// битовый буфер
	unsigned char symbol;
	// текущий проверяемый символ
	unsigned char bit_mask;
	unsigned int symb_next;
	// след. байт подгрузки - грузит в инт для сдвигов!
	int priznak = 0;
	// префикс
	unsigned char symblen, symbcode;
	// текущие из массивов
	unsigned char bits = 16;
	// длина - битовый буфер
	unsigned char tab_mask[8] = {0x01, 0x03,0x07,0xF,0x1F,0x3F,0x7F,0xFF};	// от 0 !! кол бит 1 2 3 4 5 6 7 8
	// сначала 2 байта used
	symbol = buff_in[bufpos];	// 2 байта

	bufpos++;
	Buffer_Write_Byte(buff_out, bufpos_out, symbol);	// msb used !!!
	bufpos_out++;
	symbol = buff_in[bufpos];
	//если 2 байта
	bufpos++;
	Buffer_Write_Byte(buff_out, bufpos_out, symbol);	// lsb used !!!
	bufpos_out++;
	// чтение 2 байта байта из буфера
	bitbuf = buff_in[bufpos];	// первая загрузка сразу 2 байта - далее по одному
	bufpos++;
	bitbuf |= buff_in[bufpos] * 0x100;	// если 2 байта
	bufpos++;
	bits = 16;

	while (bufpos < (len - 4))	// used и 2 уже загружены !!
	{
		// поиск в табл. символа
		for (j = 0; j < 8; j++)	// в начале всегда символ или префикс
		{
			bit_mask = tab_mask[j];
			// 8v1-0
			symbol = (unsigned char)(bitbuf & bit_mask);
			symblen = 0;	// в начале нет символа
			// проверка побитная на наличие в табл. - если да символ в выходной буфер, нет след.сдвиг
			for (k = 0; k < 10; k++)	// 0 -9 символов с префиксом в табл
			{
				if ((symbol == tabl_haf_revers[k]) && (symbol != 0))	// как будет [6] 0x00 ??
				{
					// найден не 0
					if (symbol == 0x02)	// tabl_haf_revers[9] pref или k= 9 ? те последний
						priznak = 1;
					symbcode = tabl_haf_symbol[k];
					symblen = tabl_len[k];
					break;
				}
				else 
					if ((symbol == tabl_haf_revers[k]) && (symbol == 0))
					{
						//уже прошло 5 сдвигов - это 0xfd
						if (j == 6)
						{
							symbcode = tabl_haf_symbol[k]; // символ найден
							symblen = tabl_len[k];
							
							break;
							//выход из for(k=0; k< 10 ; k++)
						}
					}
					else 
					{
						symblen = 0; 
					}
			}
			// выход из поиска символе - проверка найден или нет
			if (symblen == 0)
			{
				continue;
			} // надо повторить след сдвиг ошибка нет симв. если уже 7 !!!
			return 1; // 9v1-0
			bitbuf = bitbuf >> symblen; // символ найден длина известна - сдвигаем
			// если общая длина оставшихся битов менее 8 можно добавить след.байт из буфера
			// только его надо сдвигать впритык к оставшимся!
			bits = bits - symblen;

		#ifdef DEBUG_LEVEL85
			sprintf(buffer0,"%i, %02X, %i", symblen, symbcode, bits); //
			std::cerr << "dec_Hafman: symblen,symbcode,bits = " << (char *) buffer0 << std::endl;
		#endif

			if (bits < 8) // подгрузить и выровнять байт!!
			{
				symb_next = buff_in[bufpos] * 0x100 ; // int!! buff_in[bufpos]
				bufpos++;
				symb_next = symb_next >> (8 - bits); //
				bitbuf |= (unsigned int)symb_next;
				bits = bits + 8;
			}
			//
			if (priznak == 1)
			// взять след.8 бит на выход
			{
				symbcode = (unsigned char)( bitbuf & 0xFF);
				bits = bits - 8;
				bitbuf = bitbuf >> 8;
				Buffer_Write_Byte(buff_out, bufpos_out, symbcode);  // сохранить буфер 1 памяти
				bufpos_out++;
				if (bits < 8) // подгрузить и выровнять байт!!
				{
					symb_next = buff_in[bufpos] * 0x100; // int!!
					bufpos++;
					symb_next = symb_next >> (8 - bits); //
					bitbuf |= (unsigned int)symb_next;
					bits = bits + 8;
				}
			}
			else // нет признака - сам символ // сохранить
			{
				// bitbuf сдвинут bits уменьшен байт подгружен
				Buffer_Write_Byte(buff_out, bufpos_out, symbcode); // сохранить в буфер 1 памяти
				bufpos_out++;
			}
			break; // выход из for
		} // for (j=0; j< 8 ; j++)
		//выход из for по байту (если сдвигов уже 8 (symblen = 0 ??) ошибка не найден!!), если до - надо брать следующий 10v1-0
		if (symblen == 0)
		{
			return 0;
		} // ошибка нет симв. если уже 7 !!!
	 } //  while
	// буфер входной пуст
#ifdef DEBUG_LEVEL85
	sprintf(buffer0,"%i, %04X ", bufpos_out, bufpos_out); //
	std::cerr << "dec_Hafman: bufpos_out = " << (char *) buffer0 << std::endl;
#endif
	// в буфере 1 памяти - данные
	// Buffer_To_Page(1, LOGGER_DEC_HAF); //LOGGER_DEC_HAF LOGGER_MEM+23
	return bufpos_out;
}
