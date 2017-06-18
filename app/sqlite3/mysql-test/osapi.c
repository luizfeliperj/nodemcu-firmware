#include <c_stdlib.h>
#include <malloc.h>
#include <time.h>
#include "user_interface.h"

#undef dbg_printf
#define dbg_printf(...) 0

unsigned long system_get_free_heap_size_integer = 48 * 1024;

int os_get_random(unsigned char *buf, size_t len)
{
   srand(time(NULL));
   for (int i = 0; i < len; i++)
     buf[i] = (char)(rand() & 0xFF);

   return len;
}

void *os_malloc(size_t size)
{
	long before = system_get_free_heap_size();
	system_get_free_heap_size_integer-=size;
	dbg_printf("os_malloc: Before %ld, after %ld\n", before, system_get_free_heap_size());
	return malloc (size);
}

void os_free(void *ptr)
{
	long before = system_get_free_heap_size();
	system_get_free_heap_size_integer+=malloc_usable_size(ptr);
	dbg_printf("os_free: Before %ld, after %ld\n", before, system_get_free_heap_size());
	return free(ptr);
}

void *os_realloc(void *ptr, size_t size)
{
	long before = system_get_free_heap_size();
	system_get_free_heap_size_integer+=malloc_usable_size(ptr);
	system_get_free_heap_size_integer-=size;
	dbg_printf("os_realloc: Before %ld, after %ld\n", before, system_get_free_heap_size());
	return realloc(ptr, size);
}
