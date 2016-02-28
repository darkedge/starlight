#pragma once

namespace renderer {
	class IGraphicsApi;
}

namespace game {
	void Init(renderer::IGraphicsApi* graphicsApi);
	void Update(renderer::IGraphicsApi* graphicsApi);
	void Destroy();
}
