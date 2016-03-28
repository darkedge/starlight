#include "starlight_memory.h"

// Global memory functions

void* MEM_CALL operator new(std::size_t n) throw() {
	return memory::malloc(n);
}

void* MEM_CALL operator new[](std::size_t s) throw() {
	return operator new(s);
}

void MEM_CALL operator delete(void * p) throw() {
	return memory::free(p);
}

void MEM_CALL operator delete[](void *p) throw() {
	return operator delete(p);
}

// Wrappers

void* MEM_CALL memory::malloc(std::size_t size) {
	return ::malloc(size);
}

void MEM_CALL memory::free(void* ptr) {
	return ::free(ptr);
}

void* MEM_CALL memory::realloc(void* ptr, std::size_t size) {
	return ::realloc(ptr, size);
}

void MEM_CALL memory::no_memory() {

}
