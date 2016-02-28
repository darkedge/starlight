#pragma once
#include "starlight_renderer.h"
#include "starlight_glm.h"

namespace platform {
	glm::ivec2 GetWindowSize();
	bool LoadRenderApi(renderer::IGraphicsApi* renderApi);
	float CalculateDeltaTime();
}

