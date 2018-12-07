#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>

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

// Size to use for chunk to copy - must be even (h16 padding to words)
#define COPY_BUF_SIZE 256

static int map_name_to_spiffs(const char *pathname,
                              char spiffs_name[SPIFFS_OBJ_NAME_LEN + 1])
{
  int i;
  char *name = strdup(pathname);
  if (!name) return -1;

  char *s = basename(name);
  int len = strlen(s);

  if (len > SPIFFS_OBJ_NAME_LEN) {
    free(name);
    return -1;
  }

  for (i=0; i<len; i++) {
    spiffs_name[i] = s[i];
  }

  for (   ; i <= SPIFFS_OBJ_NAME_LEN; i++) {
    spiffs_name[i] = 0;
  }
  
  free(name);
  return 0;
}

static int file_to_fs(const char *pathname)
{
  int ret;
  struct stat statbuf;
  int fd = open(pathname, O_RDONLY);
  char spiffs_name[SPIFFS_OBJ_NAME_LEN + 1]; // +1 for NUL
  spiffs_file sfh;
  int host_size, target_size;
  uint8_t copy_buf[COPY_BUF_SIZE];
  int i;
  
  ret = map_name_to_spiffs(pathname, spiffs_name);
  if (ret != 0) {
    return ret;
  }

  fd = open(pathname, O_RDONLY);
  if (fd == -1) {
    return fd;
  }

  ret = fstat(fd, &statbuf);
  if (ret != 0) {
    (void) close(fd);
    return ret;
  }

  if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
    // Oops - not a regular file
    (void) close(fd);
    return -1;
  }
  
  sfh = SPIFFS_open(&fs, spiffs_name, (SPIFFS_O_CREAT | SPIFFS_O_WRONLY | SPIFFS_O_EXCL), 0);
  if (sfh < 0) {
    (void) close(fd);
    return -1;
  }
  
  host_size = target_size = statbuf.st_size;
  if ((target_size & 1) != 0) {
    // Round up file size to whole (16-bit) word for h16
    target_size++;
  }

  do {
    int chunk_size = COPY_BUF_SIZE;
    if (host_size < chunk_size) {
      chunk_size = host_size;
    }
    
    ret = read(fd, copy_buf, chunk_size);
    if (ret != chunk_size) {
      (void) close(fd);
      return -1;
    }

    // Set all bytes beyond those read from the source file
    // (if any) to zero
    for (i=chunk_size; i<COPY_BUF_SIZE; i++) {
      copy_buf[i] = 0;
    }

    host_size -= chunk_size;
    if (host_size == 0) {
      // Last chunk may be one byte larger than read from source
      chunk_size = target_size;
    }

    ret = SPIFFS_write(&fs, sfh, copy_buf, chunk_size);
    if (ret != chunk_size) {
      (void) close(fd);
      (void) SPIFFS_close(&fs, sfh);
      return -1;
    }

    target_size -= chunk_size;
  } while (host_size > 0);

  ret = SPIFFS_close(&fs, sfh);
  if (ret != 0) {
    (void) close(fd);
    return ret;
  }
  
  return close(fd);
}

static int dir_to_fs(const char *pathname)
{
  int ret;
  struct dirent *de;
  DIR *dir = opendir(pathname);
  if (!dir) return -1;
  
  de = readdir(dir);
  while (de) {
    struct stat statbuf;
    char *name = malloc(strlen(pathname) + strlen(de->d_name) + 2);
    strcpy(name, pathname);
    strcat(name, "/");
    strcat(name, de->d_name);

    ret = stat(name, &statbuf);
    if (ret != 0) {
      free(name);
      (void) closedir(dir);
      return ret;
    }

    // Ignore everything other than a regular file
    if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
      ret = file_to_fs(name);
      if (ret != 0) {
        free(name);
        (void) closedir(dir);
        return ret;
      }
    }

    free(name);
    de = readdir(dir);
  }

  return closedir(dir);
}

static int write_fs(const char *pathname)
{
  int fd;
  int ret;
  
  fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (!fd) return -1;

  ret = write(fd, flash, flash_size);
  if (ret != flash_size) {
    (void) close(fd);
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  s32_t ret = 0;

  flash = malloc(flash_size);
  assert(flash);
  
  memset(flash, 0xff, flash_size);
         
  ret = SPIFFS_mount(&fs, &config, fs_work,
                     fs_fd_space, FS_FD_SPACE_SIZE,
                     0, 0, 0);

  if (ret == SPIFFS_ERR_NOT_A_FS) {
    ret = SPIFFS_format(&fs);

    if (ret == 0) {
      ret = SPIFFS_mount(&fs, &config, fs_work,
                         fs_fd_space, FS_FD_SPACE_SIZE,
                         0, 0, 0);
    }
  }
  
  if (ret != SPIFFS_OK) {
    fprintf(stderr, "Failed to mount\n");
    exit(1);
  }
  
  ret = dir_to_fs("tdir");
  if (ret == 0) {
    ret = write_fs("flash.img");
  } else {
    fprintf(stderr, "dir_to_fs() failed\n");
  }

  if (flash) {
    free(flash);
    flash = 0;
  }

  return ret;
}

static s32_t hal_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
  s32_t r = SPIFFS_ERR_INTERNAL;
  
  if ((addr >= SPIFFS_CFG_PHYS_ADDR(fs)) &&
      ((addr + size) <= flash_size)) {
    memcpy(dst, &flash[addr], size);
    r = SPIFFS_OK;
  }
  return r;
}

static s32_t hal_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
  s32_t r = SPIFFS_ERR_INTERNAL;
  
  if ((addr >= SPIFFS_CFG_PHYS_ADDR(fs)) &&
      ((addr + size) <= flash_size)) {
    memcpy(&flash[addr], src, size);
    r = SPIFFS_OK;
  }
  return r;
}

static s32_t hal_spiffs_erase(u32_t addr, u32_t size)
{
  s32_t r = SPIFFS_ERR_INTERNAL;
  
  if ((addr >= SPIFFS_CFG_PHYS_ADDR(fs)) &&
      ((addr + size) <= flash_size)) {
    memset(&flash[addr], 0xff, size);
    r = SPIFFS_OK;
  }
  return r;
}
