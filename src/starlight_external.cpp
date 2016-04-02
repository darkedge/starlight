// This currently only contains stb implementations.
// For MSVC: The rest has been moved to filters so that
// different compiler settings can be applied.
// For other compilers, we could do the same by doing the
// linking stage separately.

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif

// stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
