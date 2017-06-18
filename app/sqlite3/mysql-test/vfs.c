#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <vfs.h>

sint32_t vfs_close( int fd ) {
  int rc;
  FILE *fp = (FILE*) fd;

  rc = fclose(fp);
  if (rc) fprintf (stderr, "onClose: %s\n", strerror(errno));
  return rc ? VFS_RES_ERR : VFS_RES_OK;
}

sint32_t vfs_flush( int fd ) {
  int rc;
  FILE *fp = (FILE*) fd;

  rc = fflush(fp);
  if (rc) fprintf (stderr, "onFlush: %s\n", strerror(errno));
  return rc ? VFS_RES_ERR : VFS_RES_OK;
}

sint32_t vfs_read( int fd, void *ptr, size_t len ) {
  FILE *fp = (FILE*) fd;

  return fp ? fread (ptr, 1, len, fp) : VFS_RES_ERR;
}

sint32_t vfs_write( int fd, const void *ptr, size_t len ) {
  FILE *fp = (FILE*) fd;

  return fp ? fwrite (ptr, 1, len, fp) : VFS_RES_ERR;
}

sint32_t vfs_lseek( int fd, sint32_t off, int whence ) {
  int rc, pos = -1;
  FILE *fp = (FILE*) fd;

  switch (whence) {
    case VFS_SEEK_SET:
      pos = SEEK_SET;
      break;;

    case VFS_SEEK_CUR:
      pos = SEEK_CUR;
      break;;

    case VFS_SEEK_END:
      pos = SEEK_END;
      break;;
  }

  rc = fseek(fp, off, pos);

  return rc==-1 ? VFS_RES_ERR : ftell(fp);
}

uint32_t vfs_size( int fd ) {
  int pos;
  struct stat st;
  uint32_t sz = 0;
  FILE *fp = (FILE*) fd;

  if (!fp)
    return VFS_RES_ERR;
#if 0

  pos = ftell(fp);

  fseek(fp, 0, SEEK_SET);
  while (EOF != fgetc(fp))
    sz++;

  fseek(fp, pos, SEEK_SET);
#endif

  fseek (fp, 0, SEEK_END);
  sz = ftell (fp);

  return sz;
}

int vfs_open( const char *name, const char *mode ) {
  FILE *fp = NULL;
  fp = fopen(name, mode);

  return (int)fp;
}

sint32_t vfs_stat( const char *name, struct vfs_stat *buf ) {
  struct stat st;

  if (stat (name, &st))
    return VFS_RES_ERR;

  buf->size = st.st_size;
  strncpy (buf->name, name, FS_OBJ_NAME_LEN+1);
  buf->tm.year = 1990;
  buf->tm.mon = 1;
  buf->tm.day = 1;
  buf->tm.hour = 0;
  buf->tm.min = 0;
  buf->tm.sec = 0;
  buf->is_dir = S_ISDIR(st.st_mode);
  buf->tm_valid = 1;
  buf->is_rdonly = 0;
  buf->is_hidden = 0;
  buf->is_sys = 0;
  buf->is_arch = 0;

  return VFS_RES_OK;
}

sint32_t  vfs_remove( const char *name ) {
  return unlink (name);
}
