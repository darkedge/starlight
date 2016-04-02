// Wrapper header for glm to prevent warning spam
#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
