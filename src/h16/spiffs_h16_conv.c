#include "spiffs.h"
#include "spiffs_nucleus.h"

void spiffs_h16_get_name(const spiffs_page_object_ix_header *p,       u8_t *nm)
{
  int i;
  bool copying = true;
  for (i=0; ((i<SPIFFS_OBJ_NAME_LEN) && (copying)); i++) {
    u8_t c = p->name[i];
    if (c == 0) {
      copying = false;
      // c remains zero
    } else {
      c |= 0x80; // Forced-8 parity
    }
    nm[i] = c;
  }
}

void spiffs_h16_put_name(      spiffs_page_object_ix_header *p, const u8_t *nm)
{
  int i;
  bool copying = true;
  for (i=0; i<SPIFFS_OBJ_NAME_LEN; i++) {
    u8_t c = 0;
    if (copying) {
      c = nm[i];
      if (c == 0) {
        copying = false;
        // c remains zero
      } else {
        c |= 0x80; // Forced-8 parity
      }
    }
    // if (!copying) keep writing to clear all bytes of string
    p->name[i] = c;
  }
}
  
bool spiffs_h16_cmp_name(const spiffs_page_object_ix_header *p, const u8_t *nm)
{
  int i;
  bool comparing = true;
  for (i=0; ((i<SPIFFS_OBJ_NAME_LEN) && (comparing)); i++) {
    if ((p->name[i] & 0x7f) != nm[i]) {
      return false;
    }
    if (!nm[i]) {
      // Both characters are NUL (since they match)
      comparing = false;
    }
  }
  return true;
}
