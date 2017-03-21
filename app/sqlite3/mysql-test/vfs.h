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

struct vfs_item {
  int fs_type;
  const struct vfs_item_fns *fns;
};
typedef const struct vfs_item vfs_item;

struct vfs_time {
  int year, mon, day;
  int hour, min, sec;
};
typedef struct vfs_time vfs_time;

// directory item functions
struct vfs_item_fns {
  void (*close)( const struct vfs_item *di );
  uint32_t (*size)( const struct vfs_item *di );
  sint32_t (*time)( const struct vfs_item *di, struct vfs_time *tm );
  const char *(*name)( const struct vfs_item *di );
  sint32_t (*is_dir)( const struct vfs_item *di );
  sint32_t (*is_rdonly)( const struct vfs_item *di );
  sint32_t (*is_hidden)( const struct vfs_item *di );
  sint32_t (*is_sys)( const struct vfs_item *di );
  sint32_t (*is_arch)( const struct vfs_item *di );
};
typedef const struct vfs_item_fns vfs_item_fns;

sint32_t vfs_close( int fd );
sint32_t vfs_read( int fd, void *ptr, size_t len );
sint32_t vfs_write( int fd, const void *ptr, size_t len );
sint32_t vfs_lseek( int fd, sint32_t off, int whence );
sint32_t vfs_tell( int fd );
sint32_t vfs_flush( int fd );
uint32_t vfs_size( int fd );
int vfs_open( const char *name, const char *mode );
vfs_item *vfs_stat( const char *name );
void vfs_closeitem( vfs_item *item );
sint32_t  vfs_remove( const char *name );
const char *vfs_item_name( vfs_item *item );
#endif
