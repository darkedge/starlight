#pragma once
#include "starlight.h"

namespace input {
	bool GetKey(int key);

	bool GetKeyDown(int key);

	bool GetKeyUp(int key);

	bool GetMouse(int button);

	bool GetMouseDown(int button);

	bool GetMouseUp(int button);

	bool IsMouseGrabbed();

	// In pixels. Origin is top-left
	float2 GetMouseDelta();

	// In pixels. Origin is top-left
	float2 GetMousePosition();

	void SetMouseButton(int button, bool pressed);

	void SetKey(int key, bool pressed);

	void SetMousePosition(float2 pos);

	void Init();

	void BeginFrame();

	void EndFrame();

	void SetMouseGrabbed(bool grabMouse);
}
