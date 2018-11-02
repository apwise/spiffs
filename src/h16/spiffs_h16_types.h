#ifndef SPIFFS_H16_TYPES_H_
#define SPIFFS_H16_TYPES_H_

#include <stdint.h>

typedef struct {
  int32_t low  : 15; // Low-order 15 bits
  int32_t zero :  1; // Constant zero bit (sign of lower word)
  int32_t high : 16; // Upper 16 bits, including sign
} dbl_t;

static inline dbl_t to_dbl(int32_t d)
{
  dbl_t r;
  r.low  = (d >>  1) & 0x7fff;
  r.zero = (d >> 15) & 0x0001; // Put the units bit here (for host)
  r.high = (d >> 16) & 0xffff;
  return r;
}

static inline int32_t from_dbl(dbl_t d)
{
  int32_t r;
  r = (d.high << 16) | (d.low << 1) | d.zero;
  return r;
}

#define GSZ(x) ((u32_t) from_dbl(x))
#define PSZ(x) to_dbl((int32_t) (x))

#endif
