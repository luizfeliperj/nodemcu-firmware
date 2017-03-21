#ifndef _USER_INTERFACE_H
#define _USER_INTERFACE_H

extern unsigned long system_get_free_heap_size_integer;

inline unsigned long system_get_free_heap_size(void) {
	return system_get_free_heap_size_integer;
}
#endif
