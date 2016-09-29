#pragma once
#include <map>
#include <string>

#define NUM_KEYBOARD_KEYS 0x100
//#define NUM_MOUSE_BUTTONS 5

class MJControls {
public:
	bool AssociateKey(int key, const std::string& name) {
		if (key >= 0 && key < NUM_KEYBOARD_KEYS) {
			configuration[name] = key;
			return true;
		} else {
			return false;
		}
	}
	int GetAssociatedKey(const std::string& name) {
		if (configuration.count(name) == 0) {
			return -1;
		} else {
			return configuration.at(name);
		}
	}
	bool GetKey(const std::string& name) {
		if (configuration.count(name) == 0) {
			return false;
		}
		return keys[configuration.at(name)];
	}
	bool GetKeyDown(const std::string& name) {
		if (configuration.count(name) == 0) {
			return false;
		}
		return down[configuration.at(name)];
	}
	bool GetKeyUp(const std::string& name) {
		if (configuration.count(name) == 0) {
			return false;
		}
		return up[configuration.at(name)];
	}
	// Returns the index of a key pressed this frame, otherwise 0.
	int AnyKey() {
		return anyKey;
	}

#if 0
	bool GetMouse(int button);
	bool GetMouseDown(int button);
	bool GetMouseUp(int button);
	// In pixels. Origin is top-left
	float2 GetMouseDelta();
	// In pixels. Origin is top-left
	float2 GetMousePosition();
	void SetMouseGrabbed(bool grabMouse);
	void SetMouseButton(int button, bool pressed);
	void SetMousePosition(float2 pos);
	bool IsMouseGrabbed();
#endif

	void ChangeKeyState(int key, bool pressed) {
		keys[key] = pressed;
	}

	void BeginFrame() {
		// Keyboard
		anyKey = 0;
		{
			bool changes[NUM_KEYBOARD_KEYS];
			for (unsigned int i = 0; i < NUM_KEYBOARD_KEYS; i++)
			{
				changes[i] = keys[i] ^ prev[i];
				down[i] = changes[i] & keys[i];
				if (down[i]) {
					anyKey = i;
				}
				up[i] = changes[i] & !keys[i];
			}
		}

#if 0
		// Mouse
		{
			bool changes[NUM_MOUSE_BUTTONS];
			for (unsigned int i = 0; i < NUM_MOUSE_BUTTONS; i++)
			{
				changes[i] = mouseButtons[i] ^ mousePrev[i];
				mouseDown[i] = changes[i] & mouseButtons[i];
				mouseUp[i] = changes[i] & !mouseButtons[i];
			}
		}
#endif
	}

	void EndFrame() {
		memcpy(prev, keys, NUM_KEYBOARD_KEYS * sizeof(bool));
#if 0
		memcpy(mousePrev, mouseButtons, NUM_MOUSE_BUTTONS * sizeof(bool));
		mouseDelta = { 0, 0 };
		lastMousePos = mousePos;
#endif
	}

private:
	int anyKey;
	bool prev[NUM_KEYBOARD_KEYS];
	bool keys[NUM_KEYBOARD_KEYS];
	bool down[NUM_KEYBOARD_KEYS];
	bool up[NUM_KEYBOARD_KEYS];

#if 0
	// Mouse
	bool mousePrev[NUM_MOUSE_BUTTONS];
	bool mouseButtons[NUM_MOUSE_BUTTONS];
	bool mouseDown[NUM_MOUSE_BUTTONS];
	bool mouseUp[NUM_MOUSE_BUTTONS];

	bool isMouseGrabbed;
	float2 lastMousePos;
	float2 mouseDelta;
	float2 mousePos;
#endif

	std::map<std::string, int> configuration;
};
