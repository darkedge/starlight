#include "starlight_memory.h"

// Global memory stuff

void* MEM_CALL memory::slmalloc(size_t size) {
	return malloc(size);
}

void MEM_CALL memory::slfree(void* ptr) {
	return free(ptr);
}

void* MEM_CALL memory::slrealloc(void* ptr, size_t size) {
	return realloc(ptr, size);
}
