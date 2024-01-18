#include "common.h"
#include "fs.h"

#define min(a,b) ((a<b)?a:b)

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

extern size_t ramdisk_read(void*, size_t, size_t);
extern size_t ramdisk_write(const void*, size_t, size_t);
extern size_t serial_write(const void*, size_t, size_t);
extern size_t events_read(void*, size_t, size_t);
extern size_t fb_write(const void*, size_t, size_t);
extern size_t fbsync_write(const void*, size_t, size_t);
extern size_t dispinfo_read(void*, size_t, size_t);

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin", 0, 0, 0,invalid_read, invalid_write},
  {"stdout", 0, 0, 0,invalid_read, invalid_write},
  {"stderr", 0, 0, 0,invalid_read, invalid_write},
  {"/dev/events", 0, 0, 0, events_read, invalid_write},
  {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
  {"/dev/fbsync", 0, 0, 0, invalid_read, fbsync_write},
  {"/proc/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
  {"/dev/tty", 0, 0, 0, invalid_read, serial_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  int fb = fs_open("/dev/fb", 0, 0);
  file_table[fb].size = screen_width() * screen_height() * 4;
}

int fs_open(const char* pathname, int flags, int mode){
  Log("open file : %s", pathname);
  for (int i = 0; i < NR_FILES; i++) {
    if(strcmp(pathname, file_table[i].name) == 0){
      return i;
    }
  }
  assert(!"The file doesn't exist");
}

size_t fs_read(int fd, void *buf, size_t len){
  //Log("fd = %d read : %s", fd, (char*)buf);
  Finfo fp= file_table[fd];
  if(fp.size>0){
    int free = fp.size - fp.open_offset;
    assert(free >= 0);
  }
  int size;
  if (fp.read){
    size = fp.read(buf, fp.open_offset, len);
  }
  else {
    size = ramdisk_read(buf, fp.disk_offset + fp.open_offset, min(len,fp.size-fp.open_offset));
  }
  file_table[fd].open_offset += size;
  return size;
}

size_t fs_write(int fd, void *buf, size_t len){
  //Log("fd = %d write : %s", fd, (char*)buf);
  Finfo fp=file_table[fd];
  if(fp.size>0){
    int free = fp.size - fp.open_offset;
    assert(free >= 0);
  }
  int size;
  if(!strcmp(fp.name,"stdout")||!strcmp(fp.name,"stderr")){
    printf("%s",buf);
  }else if(fp.read){
    size = fp.write(buf, fp.open_offset, len);
    file_table[fd].open_offset += size;
    return size;
  }else{
    size = ramdisk_write(buf, fp.disk_offset + fp.open_offset,min(len,fp.size-fp.open_offset));
    file_table[fd].open_offset += size;
    return size;
  }
}

size_t fs_lseek(int fd, size_t offset, int whence){
  Finfo fp=file_table[fd];
  if(fp.size>0){
    int free = fp.size - fp.open_offset;
    assert(free >= 0);
  }
  switch (whence){
    case SEEK_SET:
      file_table[fd].open_offset = offset;
      break;
    case SEEK_CUR:
      file_table[fd].open_offset += offset;
      break;
    case SEEK_END:
      file_table[fd].open_offset = file_table[fd].size;
      break;
    default:
      break;
  }
  return file_table[fd].open_offset;
}

int fs_close(int fd){
  file_table[fd].open_offset = 0;
  return 0;
}

