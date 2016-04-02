#pragma once
#include "starlight_glm.h"

namespace input {
	bool GetKey(int key);

	bool GetKeyDown(int key);

	bool GetKeyUp(int key);

	bool GetMouse(int button);

	bool GetMouseDown(int button);

	bool GetMouseUp(int button);

	bool IsMouseGrabbed();

	// In pixels. Origin is top-left
	glm::vec2 GetMouseDelta();

	// In pixels. Origin is top-left
	glm::vec2 GetMousePosition();

	void SetMouseButton(int button, bool pressed);

	void SetKey(int key, bool pressed);

	void SetMousePosition(const glm::vec2 pos);

	void Init();

	void BeginFrame();

	void EndFrame();

	void SetMouseGrabbed(bool grabMouse);
}
