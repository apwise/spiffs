#ifndef SPIFFS_H16_CONV_H_
#define SPIFFS_H16_CONV_H_

#include <stdbool.h>

static inline u16_t spiffs_word_swap_bytes(u16_t w) { return (w >> 8) | (w << 8); }

// 31 30 29 28 27 26 25 24 | 23 22 21 20 19 18 17 16 | 15 14 13 12 11 10 09 08 | 07 06 05 04 03 02 01 00
// 31 30 29 28 27 26 25 24 | 23 22 21 20 19 18 17 16 | 00 15 14 13 12 11 10 09 | 08 07 06 05 04 03 02 01
// 08 07 06 05 04 03 02 01 | 00 15 14 12 12 11 10 09 | 23 22 21 20 19 18 17 16 | 31 30 29 28 27 26 25 24
static inline u32_t spiffs_long_to_dbl(u32_t w) { return (( w >> 24)              |
                                                          ((w >> 8) & 0x0000ff00) |
                                                          ((w & 0x0000fe00) << 7) |
                                                          ( w << 23)            );}
static inline u32_t spiffs_dbl_to_long(u32_t w) { return (( w >> 23)              |
                                                          ((w >> 7) & 0x0000fe00) |
                                                          ((w & 0x0000ff00) << 8) |
                                                          ( w << 24)            );}

typedef struct spiffs_page_object_ix_header_s spiffs_page_object_ix_header;
void spiffs_h16_get_name(const spiffs_page_object_ix_header *p,       u8_t *nm);
void spiffs_h16_put_name(      spiffs_page_object_ix_header *p, const u8_t *nm);
bool spiffs_h16_cmp_name(const spiffs_page_object_ix_header *p, const u8_t *nm);

#endif /* SPIFFS_H16_CONV_H_ */
