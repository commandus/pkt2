
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
);

