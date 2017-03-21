#ifndef _OSAPI_H
#define _OSAPI_H
int os_get_random(unsigned char *buf, size_t len);

void *os_malloc(size_t size);
void os_free(void *ptr);
void *os_realloc(void *ptr, size_t size);
#endif
