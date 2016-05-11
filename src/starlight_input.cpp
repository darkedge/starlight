#include "starlight_input.h"

#define NUM_KEYBOARD_KEYS 0x100
#define NUM_MOUSE_BUTTONS 5

// Keyboard
static bool prev[NUM_KEYBOARD_KEYS];
static bool keys[NUM_KEYBOARD_KEYS];
static bool down[NUM_KEYBOARD_KEYS];
static bool up[NUM_KEYBOARD_KEYS];

// Mouse
static bool mousePrev[NUM_MOUSE_BUTTONS];
static bool mouseButtons[NUM_MOUSE_BUTTONS];
static bool mouseDown[NUM_MOUSE_BUTTONS];
static bool mouseUp[NUM_MOUSE_BUTTONS];

static bool isMouseGrabbed;
static float2 lastMousePos;
static float2 mouseDelta;
static float2 mousePos;

bool input::GetKey(int key)
{
	return keys[key];
}

bool input::GetKeyDown(int key)
{
	return down[key];
}

bool input::GetKeyUp(int key)
{
	return up[key];
}

bool input::GetMouse(int button)
{
	return mouseButtons[button];
}

bool input::GetMouseDown(int button)
{
	return mouseDown[button];
}

bool input::GetMouseUp(int button)
{
	return mouseUp[button];
}

bool input::IsMouseGrabbed()
{
	return isMouseGrabbed;
}

// In pixels. Origin is top-left
float2 input::GetMouseDelta()
{
	return mouseDelta;
}

// In pixels. Origin is top-left
float2 input::GetMousePosition()
{
	return mousePos;
}

void input::SetMouseButton(int button, bool pressed)
{
	mouseButtons[button] = pressed;
}

void input::SetKey(int key, bool pressed)
{
	keys[key] = pressed;
}

void input::SetMousePosition(float2 pos)
{
	mousePos = pos;
	mouseDelta += pos - lastMousePos;
}

void input::Init()
{
	// Keyboard
	memset(prev, int(false), NUM_KEYBOARD_KEYS * sizeof(bool));
	memset(keys, int(false), NUM_KEYBOARD_KEYS * sizeof(bool));
	memset(down, int(false), NUM_KEYBOARD_KEYS * sizeof(bool));
	memset(up, int(false), NUM_KEYBOARD_KEYS * sizeof(bool));

	// Mouse
	memset(mousePrev, int(false), NUM_MOUSE_BUTTONS * sizeof(bool));
	memset(mouseButtons, int(false), NUM_MOUSE_BUTTONS * sizeof(bool));
	memset(mouseDown, int(false), NUM_MOUSE_BUTTONS * sizeof(bool));
	memset(mouseUp, int(false), NUM_MOUSE_BUTTONS * sizeof(bool));

	isMouseGrabbed = false;
	mouseDelta = { 0, 0 };
}

void input::BeginFrame()
{
	// Keyboard
	{
		bool changes[NUM_KEYBOARD_KEYS];
		for (unsigned int i = 0; i < NUM_KEYBOARD_KEYS; i++)
		{
			changes[i] = keys[i] ^ prev[i];
			down[i] = changes[i] & keys[i];
			up[i] = changes[i] & !keys[i];
		}
	}

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
}

void input::EndFrame()
{
	memcpy(prev, keys, NUM_KEYBOARD_KEYS * sizeof(bool));
	memcpy(mousePrev, mouseButtons, NUM_MOUSE_BUTTONS * sizeof(bool));
	mouseDelta = { 0, 0 };
	lastMousePos = mousePos;
}

void input::SetMouseGrabbed(bool grabMouse)
{
	// TODO
#if 0
	isMouseGrabbed = grabMouse;
	if (isMouseGrabbed)
	{
		glfwSetInputMode(mj::Application::GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode(mj::Application::GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
#endif
}
