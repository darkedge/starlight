#pragma once
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4201)
#endif
#include "glm/glm.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

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
