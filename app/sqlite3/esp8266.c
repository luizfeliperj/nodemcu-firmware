/*
/* From: https://chromium.googlesource.com/chromium/src.git/+/4.1.249.1050/third_party/sqlite/src/os_symbian.cc
 * https://github.com/spsoft/spmemvfs/tree/master/spmemvfs
 * http://www.sqlite.org/src/doc/trunk/src/test_demovfs.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfstrace.c
 * http://www.sqlite.org/src/doc/trunk/src/test_onefile.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfs.c
**/

#include <c_stdio.h>
#include <c_stdlib.h>
#include <c_string.h>
#include <c_types.h>
#include <osapi.h>
#include <vfs.h>
#include <time.h>
#include <sqlite3.h>
#ifdef CACHE_JOURNAL
#include <spi_flash.h>
#endif

// #undef dbg_printf
// #define dbg_printf(...) 0
#define ESP8266_DEFAULT_MAXNAMESIZE 32

static int esp8266_Close(sqlite3_file*);
static int esp8266_Lock(sqlite3_file *id, int);
static int esp8266_Unlock(sqlite3_file*, int);
static int esp8266_Sync(sqlite3_file*, int);
static int esp8266_Open(sqlite3_vfs*, const char *, sqlite3_file *, int, int*);
static int esp8266_Read(sqlite3_file*, void*, int, sqlite3_int64);
static int esp8266_Write(sqlite3_file*, const void*, int, sqlite3_int64);
static int esp8266_Truncate(sqlite3_file*, sqlite3_int64);
static int esp8266_Delete(sqlite3_vfs*, const char *, int);
static int esp8266_FileSize(sqlite3_file*, sqlite3_int64*);
static int esp8266_Access(sqlite3_vfs*, const char*, int, int*);
static int esp8266_FullPathname( sqlite3_vfs*, const char *, int, char*);
static int esp8266_CheckReservedLock(sqlite3_file*, int *);
static int esp8266_FileControl(sqlite3_file *, int, void*);
static int esp8266_SectorSize(sqlite3_file*);
static int esp8266_DeviceCharacteristics(sqlite3_file*);
static void* esp8266_DlOpen(sqlite3_vfs*, const char *);
static void esp8266_DlError(sqlite3_vfs*, int, char*);
static void (*esp8266_DlSym (sqlite3_vfs*, void*, const char*))(void);
static void esp8266_DlClose(sqlite3_vfs*, void*);
static int esp8266_Randomness(sqlite3_vfs*, int, char*);
static int esp8266_Sleep(sqlite3_vfs*, int);
static int esp8266_CurrentTime(sqlite3_vfs*, double*);

#ifdef CACHE_JOURNAL
#define NUMPAGES 8

static int esp8266mem_Close(sqlite3_file*);
static int esp8266mem_Read(sqlite3_file*, void*, int, sqlite3_int64);
static int esp8266mem_Write(sqlite3_file*, const void*, int, sqlite3_int64);
static int esp8266cache_Read(sqlite3_file*, void*, int, sqlite3_int64);
static int esp8266cache_Write(sqlite3_file*, const void*, int, sqlite3_int64);
static int esp8266cache_Close(sqlite3_file*);
static int esp8266mem_FileSize(sqlite3_file*, sqlite3_int64*);
static int esp8266mem_Sync(sqlite3_file*, int);
#endif

struct esp8266_file {
  sqlite3_file base;
  int fd;
#ifdef CACHE_JOURNAL
  uint8_t *journal;
  uint8_t *writecache;
#endif
  char name[ESP8266_DEFAULT_MAXNAMESIZE];
};
typedef struct esp8266_file esp8266_file;

static sqlite3_vfs  esp8266Vfs = {
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

static const sqlite3_io_methods esp8266IoMethods = {
  1,    // iVersion
#ifdef CACHE_JOURNAL
  esp8266cache_Close,
  esp8266cache_Read,
  esp8266cache_Write,
#else
  esp8266_Close,
  esp8266_Read,
  esp8266_Write,
#endif
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

#ifdef CACHE_JOURNAL
static const sqlite3_io_methods esp8266MemMethods = {
  1,    // iVersion
  esp8266mem_Close,
  esp8266mem_Read,
  esp8266mem_Write,
  esp8266_Truncate,
  esp8266mem_Sync,
  esp8266mem_FileSize,
  esp8266_Lock,
  esp8266_Unlock,
  esp8266_CheckReservedLock,
  esp8266_FileControl,
  esp8266_SectorSize,
  esp8266_DeviceCharacteristics
};

static int esp8266mem_Close(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	sqlite3_free (file->journal);

	dbg_printf("esp8266mem_Close: %s OK\n", file->name);
	return SQLITE_OK;
}

static int esp8266mem_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	sint32_t ofst;
	esp8266_file *file = (esp8266_file*) id;
	uint16_t *sz = (uint16_t*)file->journal;
	uint8_t *pBuf = file->journal + sizeof(uint16_t);

	ofst = (sint32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp8266mem_Read: 1r %s %ld %d\n", file->name, ofst, amount);
	if (ofst + amount >= *sz) {
		int cpsz = amount - ( (ofst + amount) - *sz );
		if (cpsz > 0) {
			dbg_printf("esp8266mem_Read: 2r %s %d CPSZ\n", file->name, cpsz);
			memcpy (buffer, pBuf + ofst, cpsz);
		}
		dbg_printf("esp8266mem_Read: 3r %s PART [%d] [%ld] [%d] [%d] OK\n", file->name, cpsz, ofst, amount, *sz);
		return SQLITE_IOERR_SHORT_READ;
	}

	memcpy (buffer, pBuf + ofst, amount);

	dbg_printf("esp8266mem_Read: %s [%ld] [%d] OK\n", file->name, ofst, amount);
	return SQLITE_OK;
}

static int esp8266mem_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	sint32_t ofst;
	esp8266_file *file = (esp8266_file*) id;
	uint16_t *sz = (uint16_t*)file->journal;

	ofst = (sint32_t)(offset & 0x7FFFFFFF);
	if (ofst + amount > NUMPAGES*SQLITE_DEFAULT_PAGE_SIZE && ofst + amount >= *sz) {
		void *ptr = sqlite3_realloc(file->journal, ofst + amount + sizeof(uint16_t));
		if (ptr == NULL) {
			dbg_printf("esp8266mem_Write: 1w %s [%ld] [%d] FAIL\n", file->name, ofst, amount);
			return SQLITE_IOERR_WRITE;
		}
		else {
			sz = (uint16_t*)ptr;
			file->journal = ptr;
			dbg_printf("esp8266mem_Write: 2w %s [%ld] [%d] REALLOC\n", file->name, ofst, amount);
		}
	}

	*sz = ofst + amount;
	memcpy (file->journal + sizeof(uint16_t) + ofst, buffer, amount);

	dbg_printf("esp8266mem_Write: %s [%ld] [%d] OK\n", file->name, ofst, amount);
	return SQLITE_OK;
}

static int esp8266mem_Sync(sqlite3_file *id, int flags)
{
	esp8266_file *file = (esp8266_file*) id;
	dbg_printf("esp8266mem_Sync: %s OK\n", file->name);
	return  SQLITE_OK;
}

static int esp8266mem_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp8266_file *file = (esp8266_file*) id;
	uint16_t *sz = (uint16_t*)file->journal;

	*size = 0LL | *sz;
	dbg_printf("esp8266mem_FileSize: %s [%d] OK\n", file->name, *sz);
	return SQLITE_OK;
}
#endif

static int esp8266_Open( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags )
{
	int rc;
	char *mode = "r";
	esp8266_file *p = (esp8266_file*) file;

	if ( path == NULL ) return SQLITE_IOERR;
	if( flags&SQLITE_OPEN_READONLY )  mode = "r";
	if( flags&SQLITE_OPEN_READWRITE || flags&SQLITE_OPEN_MAIN_JOURNAL ) {
		int result;
		if (SQLITE_OK != esp8266_Access(vfs, path, flags, &result))
			return SQLITE_CANTOPEN;

		if (result == 1)
			mode = "r+";
		else
			mode = "w+";
	}

	dbg_printf("esp8266_Open: 1o %s %s\n", path, mode);
	memset (p, 0, sizeof(esp8266_file));

        strncpy (p->name, path, ESP8266_DEFAULT_MAXNAMESIZE);
	p->name[ESP8266_DEFAULT_MAXNAMESIZE-1] = '\0';

#ifdef CACHE_JOURNAL
	p->writecache = NULL;

	if( flags&SQLITE_OPEN_MAIN_JOURNAL ) {
		p->base.pMethods = &esp8266MemMethods;
		p->journal = sqlite3_malloc(NUMPAGES*SQLITE_DEFAULT_PAGE_SIZE + sizeof(uint16_t));
		if (! p->journal )
			return SQLITE_NOMEM;

		p->fd = 0;
		memset (p->journal, 0, NUMPAGES*SQLITE_DEFAULT_PAGE_SIZE + sizeof(uint16_t));
		dbg_printf("esp8266_Open: 2o %s %d MEM OK\n", p->name, p->fd);
		return SQLITE_OK;
	}
#endif

	p->fd = vfs_open (path, mode);
	if ( p->fd <= 0 ) {
		return SQLITE_CANTOPEN;
	}

	p->base.pMethods = &esp8266IoMethods;
	dbg_printf("esp8266_Open: 2o %s %d OK\n", p->name, p->fd);
	return SQLITE_OK;
}

static int esp8266_Close(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	int rc = vfs_close(file->fd);
	dbg_printf("esp8266_Close: %s %d %d\n", file->name, file->fd, rc);
	return rc ? SQLITE_IOERR_CLOSE : SQLITE_OK;
}

#ifdef CACHE_JOURNAL
static int esp8266cache_Close(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	if (file->writecache != NULL)
		sqlite3_free(file->writecache);

	return esp8266_Close(id);
}

static int esp8266cache_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	sint32_t ofst;
	uint16_t block;
	esp8266_file *file = (esp8266_file*) id;

	ofst = (sint32_t)(offset & 0x7FFFFFFF);

	return esp8266_Read(id, buffer, amount, offset);
}

static int esp8266cache_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	sint32_t ofst;
	uint16_t block, *savedblock;
	esp8266_file *file = (esp8266_file*) id;

	ofst = (sint32_t)(offset & 0x7FFFFFFF);

	if (file->writecache == NULL) {
		file->writecache = sqlite3_malloc(SPI_FLASH_SEC_SIZE + sizeof(uint16_t));
		if (! file->writecache )
			return SQLITE_NOMEM;
		memset (file->writecache, 0, SPI_FLASH_SEC_SIZE + sizeof(uint16_t));
	}

	savedblock = (uint16_t*) file->writecache;

	if ((ofst+amount)/SPI_FLASH_SEC_SIZE != *savedblock) {
		dbg_printf("esp8266cache_Write: writing is crossing borders %ld/%d\n", (ofst+amount)/SPI_FLASH_SEC_SIZE, *savedblock);
	}

	return esp8266_Write(id, buffer, amount, offset);
}
#endif

static int esp8266_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	size_t nRead;
	sint32_t ofst, iofst;
	esp8266_file *file = (esp8266_file*) id;

	iofst = (sint32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp8266_Read: 1r %s %d %d %lld[%ld] \n", file->name, file->fd, amount, offset, iofst);
	ofst = vfs_lseek(file->fd, iofst, VFS_SEEK_SET);
	if (ofst != iofst) {
	        dbg_printf("esp8266_Read: 2r %ld != %ld FAIL\n", ofst, iofst);
		return SQLITE_IOERR_SHORT_READ /* SQLITE_IOERR_SEEK */;
	}

	nRead = vfs_read(file->fd, buffer, amount);
	if ( nRead == amount ) {
	        dbg_printf("esp8266_Read: 3r %s %u %d OK\n", file->name, nRead, amount);
		return SQLITE_OK;
	} else if ( nRead >= 0 ) {
	        dbg_printf("esp8266_Read: 3r %s %u %d FAIL\n", file->name, nRead, amount);
		return SQLITE_IOERR_SHORT_READ;
	}

	dbg_printf("esp8266_Read: 4r %s FAIL\n", file->name);
	return SQLITE_IOERR_READ;
}

static int esp8266_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	size_t nWrite;
	sint32_t ofst, iofst;
	esp8266_file *file = (esp8266_file*) id;

	iofst = (sint32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp8266_Write: 1w %s %d %d %lld[%ld] \n", file->name, file->fd, amount, offset, iofst);
	ofst = vfs_lseek(file->fd, iofst, VFS_SEEK_SET);
	if (ofst != iofst) {
		return SQLITE_IOERR_SEEK;
	}

	nWrite = vfs_write(file->fd, buffer, amount);
	if ( nWrite != amount ) {
		dbg_printf("esp8266_Write: 2w %s %u %d\n", file->name, nWrite, amount);
		return SQLITE_IOERR_WRITE;
	}

	dbg_printf("esp8266_Write: 3w %s OK\n", file->name);
	return SQLITE_OK;
}

static int esp8266_Truncate(sqlite3_file *id, sqlite3_int64 bytes)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_Truncate:\n");
	return 0 ? SQLITE_IOERR_TRUNCATE : SQLITE_OK;
}

static int esp8266_Delete( sqlite3_vfs * vfs, const char * path, int syncDir )
{
	sint32_t rc = vfs_remove( path );
	if (rc == VFS_RES_ERR)
		return SQLITE_IOERR_DELETE;

	dbg_printf("esp8266_Delete: %s OK\n", path);
	return SQLITE_OK;
}

static int esp8266_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp8266_file *file = (esp8266_file*) id;
	*size = 0LL | vfs_size(file->fd);
	dbg_printf("esp8266_FileSize: %s %u[%lld]\n", file->name, vfs_size(file->fd), *size);
	return SQLITE_OK;
}

static int esp8266_Sync(sqlite3_file *id, int flags)
{
	esp8266_file *file = (esp8266_file*) id;

	int rc = vfs_flush(file->fd);
	dbg_printf("esp8266_Sync: %d\n", rc);

	return rc ? SQLITE_IOERR_FSYNC : SQLITE_OK;
}

static int esp8266_Access( sqlite3_vfs * vfs, const char * path, int flags, int * result )
{
	vfs_item *item = vfs_stat(path);
	*result = (item!=NULL);
	if (item) vfs_closeitem(item);

	dbg_printf("esp8266_Access: %d\n", *result);
	return SQLITE_OK;
}

static int esp8266_FullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath )
{
	strncpy( fullpath, path, len );
	fullpath[ len - 1 ] = '\0';

	dbg_printf("esp8266_FullPathname: %s\n", fullpath);
	return SQLITE_OK;
}

static int esp8266_Lock(sqlite3_file *id, int lock_type)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_Lock:\n");
	return SQLITE_OK;
}

static int esp8266_Unlock(sqlite3_file *id, int lock_type)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_Unlock:\n");
	return SQLITE_OK;
}

static int esp8266_CheckReservedLock(sqlite3_file *id, int *result)
{
	esp8266_file *file = (esp8266_file*) id;

	*result = 0;

	dbg_printf("esp8266_CheckReservedLock:\n");
	return SQLITE_OK;
}

static int esp8266_FileControl(sqlite3_file *id, int op, void *arg)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_FileControl:\n");
	return SQLITE_OK;
}

static int esp8266_SectorSize(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_SectorSize:\n");
	return 0;
}

static int esp8266_DeviceCharacteristics(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_DeviceCharacteristics:\n");
	return 0;
}

static void * esp8266_DlOpen( sqlite3_vfs * vfs, const char * path )
{
	dbg_printf("esp8266_DlOpen:\n");
	return NULL;
}

static void esp8266_DlError( sqlite3_vfs * vfs, int len, char * errmsg )
{
	dbg_printf("esp8266_DlError:\n");
	return;
}

static void ( * esp8266_DlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void )
{
	dbg_printf("esp8266_DlSym:\n");
	return NULL;
}

static void esp8266_DlClose( sqlite3_vfs * vfs, void * handle )
{
	dbg_printf("esp8266_DlClose:\n");
	return;
}

static int esp8266_Randomness( sqlite3_vfs * vfs, int len, char * buffer )
{
	int rc = os_get_random(buffer, len);
	dbg_printf("esp8266_Randomness: %d\n", rc);
	return SQLITE_OK;
}

static int esp8266_Sleep( sqlite3_vfs * vfs, int microseconds )
{
	dbg_printf("esp8266_Sleep:\n");
	return SQLITE_OK;
}

static int esp8266_CurrentTime( sqlite3_vfs * vfs, double * result )
{
	time_t t = time(NULL);
	*result = t / 86400.0 + 2440587.5;
	dbg_printf("esp8266_CurrentTime: %g\n", *result);
	return SQLITE_OK;
}

int sqlite3_os_init(void){
  sqlite3_vfs_register(&esp8266Vfs, 1);
  return SQLITE_OK;
}

int sqlite3_os_end(void){
  return SQLITE_OK;
}
