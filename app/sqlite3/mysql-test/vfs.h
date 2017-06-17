#ifndef __VFS_H__
#define __VFS_H__

#include <c_string.h>
#include <c_stdint.h>

enum vfs_seek {
  VFS_SEEK_SET = 0,
  VFS_SEEK_CUR,
  VFS_SEEK_END
};

enum vfs_result {
  VFS_RES_OK  = 0,
  VFS_RES_ERR = -1
};

struct vfs_time {
  int year, mon, day;
  int hour, min, sec;
};
typedef struct vfs_time vfs_time;

struct vfs_stat {
  uint32_t size;
  char name[FS_OBJ_NAME_LEN+1];
  struct vfs_time tm;
  uint8_t tm_valid;
  uint8_t is_dir;
  uint8_t is_rdonly;
  uint8_t is_hidden;
  uint8_t is_sys;
  uint8_t is_arch;
};

sint32_t vfs_close( int fd );
sint32_t vfs_read( int fd, void *ptr, size_t len );
sint32_t vfs_write( int fd, const void *ptr, size_t len );
sint32_t vfs_lseek( int fd, sint32_t off, int whence );
sint32_t vfs_tell( int fd );
sint32_t vfs_flush( int fd );
uint32_t vfs_size( int fd );
int vfs_open( const char *name, const char *mode );
sint32_t vfs_stat( const char *name, struct vfs_stat *st );
sint32_t vfs_remove( const char *name );
#endif
