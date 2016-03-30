#pragma once
#include <string>

#if 0
int Stricmp(const char* str1, const char* str2) {
	int d;
	while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) {
		str1++;
		str2++;
	}
	return d;
}

int Strnicmp(const char* str1, const char* str2, int count) {
	int d = 0;
	while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) {
		str1++; str2++; count--;
	}
	return d;
}
#endif

#ifdef _MSC_VER
  #if _MSC_VER >= 1800
    #define SL_CALL __vectorcall
  #else
    #define SL_CALL __cdecl
  #endif
#else
#define SL_CALL
#endif

#define COUNT_OF(X) (sizeof(X) / sizeof((X)[0]))
#define ZERO_MEM(X, Y) (memset(X, 0, Y));
