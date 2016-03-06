#pragma once
#include "starlight_graphics.h"

struct GameInfo {
	bool initialized;
	EGraphicsApi graphicsApi;
};

namespace game {
	void Update(GameInfo* gameInfo, graphics::API* graphicsApi);
	void Destroy();
}
