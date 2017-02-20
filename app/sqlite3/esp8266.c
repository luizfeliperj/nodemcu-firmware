/*
/* From: https://chromium.googlesource.com/chromium/src.git/+/4.1.249.1050/third_party/sqlite/src/os_symbian.cc
 * https://github.com/spsoft/spmemvfs/tree/master/spmemvfs
 * http://www.sqlite.org/src/doc/trunk/src/test_demovfs.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfstrace.c
 * http://www.sqlite.org/src/doc/trunk/src/test_onefile.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfs.c
**/

#include <c_types.h>
#include <vfs.h>
#include <time.h>
#include <sqlite3.h>

typedef struct esp8266_file esp8266_file;
struct esp8266_file {
  sqlite3_file base;
  int fd;
};

static int esp8266_Close(sqlite3_file*);
static int esp8266_Lock(sqlite3_file*, int);
static int esp8266_Sync(sqlite3_file*, int);
static int esp8266_SectorSize(sqlite3_file*);
static int esp8266_Unlock(sqlite3_file*, int);
static int esp8266_DeviceCharacteristics(sqlite3_file *);
static int esp8266_CheckReservedLock(sqlite3_file*, int*);
static int esp8266_FileControl(sqlite3_file*, int, void*);
static int esp8266_Truncate(sqlite3_file*, sqlite3_int64);
static int esp8266_FileSize(sqlite3_file*, sqlite3_int64*);
static int esp8266_Read(sqlite3_file*, void*, int, sqlite3_int64);
static int esp8266_Write(sqlite3_file*, const void*, int, sqlite3_int64);

static int esp8266_Open( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags )
{
	static const sqlite3_io_methods esp8266IoMethods = {
  		1,    // iVersion
  		esp8266_Close,
  		esp8266_Read,
  		esp8266_Write,
  		esp8266_Truncate,
  		esp8266_Sync,
  		esp8266_FileSize,
  		esp8266_Lock,
  		esp8266_Unlock,
  		esp8266_CheckReservedLock,
  		esp8266_FileControl,
  		esp8266_SectorSize,
  		esp8266_DeviceCharacteristics
	};
	int rc;
	char *mode;
	esp8266_file *p = (esp8266_file*) file;

	if ( path == NULL ) return SQLITE_IOERR;
	if( flags&SQLITE_OPEN_MAIN_JOURNAL ) return SQLITE_NOMEM;
	if( flags&SQLITE_OPEN_CREATE )    mode = "w+";
	if( flags&SQLITE_OPEN_READONLY )  mode = "r";
	if( flags&SQLITE_OPEN_READWRITE ) mode = "r+";


	memset(p, 0, sizeof(esp8266_file));

	p->fd = vfs_open (path, mode);
	if ( p->fd < 0 ) {
		return SQLITE_CANTOPEN;
	}

	p->base.pMethods = &esp8266IoMethods;
	return SQLITE_OK;
}

static int esp8266_Close(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	int rc = vfs_close(file->fd);
	return rc ? SQLITE_IOERR_CLOSE : SQLITE_OK;
}

static int esp8266_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	esp8266_file *file = (esp8266_file*) id;
	sint32_t ofst;
	size_t nRead;
	int rc;

	ofst = vfs_lseek(file->fd, offset, VFS_SEEK_SET);
	if (ofst != offset) {
		return SQLITE_IOERR_READ;
	}
	nRead = vfs_read(file->fd, buffer, amount);

	if ( nRead == amount ) {
		return SQLITE_OK;
	} else if ( nRead >= 0 ) {
		return SQLITE_IOERR_SHORT_READ;
	}

	return SQLITE_IOERR_READ;
}

static int esp8266_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	esp8266_file *file = (esp8266_file*) id;
	sint32_t ofst;
	size_t nWrite;
	int rc;

	ofst = vfs_lseek (file->fd, offset, VFS_SEEK_SET);
	if ( ofst != offset ) {
		return SQLITE_IOERR_WRITE;
	}

	nWrite = vfs_write(file->fd, buffer, amount);
	if ( nWrite != amount ) {
		return SQLITE_IOERR_WRITE;
	}

	return SQLITE_OK;
}

static int esp8266_Truncate(sqlite3_file *id, sqlite3_int64 bytes)
{
	return SQLITE_IOERR_TRUNCATE;
}

static int esp8266_Delete( sqlite3_vfs * vfs, const char * path, int syncDir )
{
	sint32_t rc = vfs_remove( path );
	if (rc == VFS_RES_ERR)
		return SQLITE_IOERR_DELETE;

	return SQLITE_OK;
}

static int esp8266_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp8266_file *file = (esp8266_file*) id;
	return vfs_size(file->fd);
}

static int esp8266_Sync(sqlite3_file *id, int flags)
{
	return SQLITE_OK;
}

static int esp8266_Access( sqlite3_vfs * vfs, const char * path, int flags, int * result )
{
	*result = 0;
	return SQLITE_OK;
}

static int esp8266_FullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath )
{
	strncpy( fullpath, path, len );
	fullpath[ len - 1 ] = '\0';

	return SQLITE_OK;
}

static int esp8266_Lock(sqlite3_file *id, int lock_type)
{
	return SQLITE_OK;
}

static int esp8266_Unlock(sqlite3_file *id, int lock_type)
{
	return SQLITE_OK;
}

static int esp8266_CheckReservedLock(sqlite3_file *id, int *result)
{
	*result = 0;
	return SQLITE_OK;
}

static int esp8266_FileControl(sqlite3_file *id, int op, void *arg)
{
	return SQLITE_OK;
}

static int esp8266_SectorSize(sqlite3_file *id)
{
	return 0;
}

static int esp8266_DeviceCharacteristics(sqlite3_file *id)
{
	return 0;
}

static void * esp8266_DlOpen( sqlite3_vfs * vfs, const char * path )
{
	return NULL;
}

static void esp8266_DlError( sqlite3_vfs * vfs, int len, char * errmsg )
{
	return;
}

static void ( * esp8266_DlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void )
{
	return NULL;
}

static void esp8266_DlClose( sqlite3_vfs * vfs, void * handle )
{
	return;
}

static int esp8266_Randomness( sqlite3_vfs * vfs, int len, char * buffer )
{
	return SQLITE_OK;
}

static int esp8266_Sleep( sqlite3_vfs * vfs, int microseconds )
{
	return 0;
}

static int esp8266_CurrentTime( sqlite3_vfs * vfs, double * result )
{
	time_t t = time(0);
	*result = t / 86400.0 + 2440587.5;
	return SQLITE_OK;
}

int sqlite3_os_init(void){
  static sqlite3_vfs vfs = {
    1,                       // iVersion
    sizeof(esp8266_file),    // szOsFile
    FS_OBJ_NAME_LEN,         // mxPathname
    NULL,                    // pNext
    "esp8266",               // name
    0,                       // pAppData
    esp8266_Open,            // xOpen
    esp8266_Delete,          // xDelete
    esp8266_Access,          // xAccess
    esp8266_FullPathname,    // xFullPathname
    esp8266_DlOpen,          // xDlOpen
    esp8266_DlError,         // xDlError
    esp8266_DlSym,           // xDlSym
    esp8266_DlClose,         // xDlClose
    esp8266_Randomness,      // xRandomness
    esp8266_Sleep,           // xSleep
    esp8266_CurrentTime,     // xCurrentTime
    0                        // xGetLastError
  };

  sqlite3_vfs_register(&vfs, 1);
  return SQLITE_OK;
}

int sqlite3_os_end(void){
  return SQLITE_OK;
}
