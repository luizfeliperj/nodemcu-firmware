#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include "user_interface.h"

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
	system_get_free_heap_size_integer+=size;
	return malloc (size);
}

void os_free(void *ptr)
{
	system_get_free_heap_size_integer-=malloc_usable_size(ptr);
	return free(ptr);
}

void *os_realloc(void *ptr, size_t size)
{
	system_get_free_heap_size_integer-=malloc_usable_size(ptr);
	system_get_free_heap_size_integer+=size;
	return realloc(ptr, size);
}
