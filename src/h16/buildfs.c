#include <assert.h>

#include "spiffs.h"

static s32_t hal_spiffs_read(u32_t addr, u32_t size, u8_t *dst);
static s32_t hal_spiffs_write(u32_t addr, u32_t size, u8_t *src);
static s32_t hal_spiffs_erase(u32_t addr, u32_t size);


static spiffs fs;
static spiffs_config config =
  {
    hal_spiffs_read,
    hal_spiffs_write,
    hal_spiffs_erase
  };

static u8_t fs_work[SPIFFS_CFG_LOG_PAGE_SZ(fs) * 2];
#define FS_FD_SPACE_SIZE 256
static u8_t fs_fd_space[FS_FD_SPACE_SIZE];

static const size_t flash_size = SPIFFS_CFG_PHYS_ADDR(fs) + SPIFFS_CFG_PHYS_SZ(fs);
static uint8_t *flash = 0;

int main(int argc, char **argv)
{
  s32_t ret;

  flash = malloc(flash_size);
  assert(flash);
  
  memset(flash, 0xff, flash_size);
         
  ret = SPIFFS_mount(&fs, &config, fs_work,
                     fs_fd_space, FS_FD_SPACE_SIZE,
                     0, 0, 0);

  if (ret != SPIFFS_OK) {
    fprintf(stderr, "Failed to mount\n");
    exit(1);
  }

  if (flash) {
    free(flash);
    flash = 0;
  }

  return 0;
}

static s32_t hal_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
  s32_t r = SPIFFS_ERR_INTERNAL;
  
  if ((addr >= SPIFFS_CFG_PHYS_ADDR(fs)) &&
      ((addr + size - SPIFFS_CFG_PHYS_ADDR(fs)) <= SPIFFS_CFG_PHYS_SZ(fs))) {
    memcpy(dst, &flash[addr], size);
    r = SPIFFS_OK;
  }
  return r;
}

static s32_t hal_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
  s32_t r = SPIFFS_ERR_INTERNAL;
  
  if ((addr >= SPIFFS_CFG_PHYS_ADDR(fs)) &&
      ((addr + size - SPIFFS_CFG_PHYS_ADDR(fs)) <= SPIFFS_CFG_PHYS_SZ(fs))) {
    memcpy(&flash[addr], src, size);
    r = SPIFFS_OK;
  }
  return r;
}

static s32_t hal_spiffs_erase(u32_t addr, u32_t size)
{
  s32_t r = SPIFFS_ERR_INTERNAL;
  
  if ((addr >= SPIFFS_CFG_PHYS_ADDR(fs)) &&
      ((addr + size - SPIFFS_CFG_PHYS_ADDR(fs)) <= SPIFFS_CFG_PHYS_SZ(fs))) {
    memset(&flash[addr], 0xff, size);
    r = SPIFFS_OK;
  }
  return r;
}
