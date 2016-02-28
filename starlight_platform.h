#pragma once

namespace renderer {
	class IGraphicsApi;
}

namespace platform {
	bool LoadRenderApi(renderer::IGraphicsApi* renderApi);
	float CalculateDeltaTime();
}

