/*
 * ieee754.h
 *
 *  Created on: 14.09.2015
 *  http://beej.us/guide/bgnet/examples/ieee754.c
 */

#ifndef IEEE754_H_
#define IEEE754_H_

#include <stdint.h>
#include <inttypes.h>

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

uint64_t pack754(long double f, unsigned bits, unsigned expbits);
long double unpack754(uint64_t i, unsigned bits, unsigned expbits);

#endif /* IEEE754_H_ */
