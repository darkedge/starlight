#include "starlight_game.h"
#include "starlight_input.h"
#include "starlight_log.h"
#include "starlight_generated.h"
#include "starlight_transform.h"
#include "starlight_renderer.h"
#include "starlight_platform.h"
//#include <process.h> // ?
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include "starlight_glm.h"
#include "imgui.h"

struct Camera {
	float m_fieldOfView = glm::radians(60.0f); // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};

static Transform s_player;
static Camera s_camera;
static float s_deltaTime;

int32_t s_mesh;

int32_t CreateCube(renderer::IGraphicsApi* graphicsApi) {
	Vertex vertices[8] =
	{
		{ { -1.0f, -1.0f, -1.0f },{ 0.0f, 0.0f, 0.0f } }, // 0
		{ { -1.0f,  1.0f, -1.0f },{ 0.0f, 1.0f, 0.0f } }, // 1
		{ { 1.0f,  1.0f, -1.0f },{ 1.0f, 1.0f, 0.0f } }, // 2
		{ { 1.0f, -1.0f, -1.0f },{ 1.0f, 0.0f, 0.0f } }, // 3
		{ { -1.0f, -1.0f,  1.0f },{ 0.0f, 0.0f, 1.0f } }, // 4
		{ { -1.0f,  1.0f,  1.0f },{ 0.0f, 1.0f, 1.0f } }, // 5
		{ { 1.0f,  1.0f,  1.0f },{ 1.0f, 1.0f, 1.0f } }, // 6
		{ { 1.0f, -1.0f,  1.0f },{ 1.0f, 0.0f, 1.0f } }  // 7
	};

	int32_t indices[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	return graphicsApi->UploadMesh(vertices, _countof(vertices), indices, _countof(indices));
}

void game::Init(renderer::IGraphicsApi* graphicsApi) {
	input::Init();
	logger::Init();

	{
		// Load Fonts
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("external/imgui-1.47/extra_fonts/DroidSans.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	}
	
	LogInfo("Сука блять!");

	s_player.SetPosition(0, 0, -10);

	// Cube
	s_mesh = CreateCube(graphicsApi);
	
	// Timing
	s_deltaTime = 0.0f;
}

#if 0
void MoveCamera() {
	// TODO: probably need to check if i'm typing something in ImGui or not
	static glm::vec2 lastRotation;
	static glm::vec2 currentRotation;

	if (input::GetKeyDown('M'))
	{
		input::SetMouseGrabbed(!input::IsMouseGrabbed());
	}

	// Reset
	if (input::GetKeyDown('R'))
	{
		currentRotation = lastRotation = { 0, 0 };
		s_player.SetPosition(glm::vec3(0, 0, 0));
		s_player.SetRotation(glm::quat(1, 0, 0, 0));
	}

	if (input::IsMouseGrabbed())
	{
		// Rotation
		const float ROT_SPEED = 0.0025f;
		currentRotation -= ROT_SPEED * input::GetMouseDelta();
		if (currentRotation.y < glm::radians(-89.0f))
		{
			currentRotation.y = glm::radians(-89.0f);
		}
		if (currentRotation.y > glm::radians(89.0f))
		{
			currentRotation.y = glm::radians(89.0f);
		}
		if (currentRotation.x != lastRotation.x || currentRotation.y != lastRotation.y)
		{
			s_player.SetRotation(glm::quat(glm::vec3(currentRotation.y, currentRotation.x, 0.0f)));
			lastRotation = currentRotation;
		}
	}

	// Translation
	const float SPEED = 20.0f;
	glm::vec3 translation(0, 0, 0);
	if (input::GetKey('W'))		translation += s_player.Forward();
	if (input::GetKey('A'))		translation -= s_player.Right();
	if (input::GetKey('S'))		translation -= s_player.Forward();
	if (input::GetKey('D'))		translation += s_player.Right();
	if (input::GetKey(VK_LCONTROL) || input::GetKey('C') || input::GetKey(VK_LSHIFT)) translation -= glm::vec3(0, 1, 0);
	if (input::GetKey(VK_SPACE)) translation += glm::vec3(0, 1, 0);
	if (translation != glm::vec3(0, 0, 0))
	{
		glm::vec3 pos = s_player.GetPosition();
		pos += glm::normalize(translation) * SPEED * s_deltaTime;
		s_player.SetPosition(pos);
		//printf("pos: %.1f, %.1f, %.1f\n", m_player.GetPosition().x, m_player.GetPosition().y, m_player.GetPosition().z);
	}
}
#endif

void game::Update(renderer::IGraphicsApi* graphicsApi) {
	// Timing
	s_deltaTime = platform::CalculateDeltaTime();

	// Begin logic
	input::BeginFrame();
	//MoveCamera(); // TODO
	input::EndFrame();

	// Does not render, but builds display lists
	logger::Render();

	graphicsApi->SetPlayerCameraViewMatrix(s_player.GetViewMatrix());


#if 0
	renderer::SetPerCamera(&s_perCamera);

	auto windowSize = platform::GetWindowSize();
	if (windowSize.x > 0 && windowSize.y > 0) {
		glm::mat4 projectionMatrix = glm::perspectiveFovLH(glm::radians(45.0f), (float)windowSize.x, (float)windowSize.y, 0.1f, 100.0f);
		s_perCamera.view = s_player.GetViewMatrix();
		s_perFrame.projection = projectionMatrix;
		renderer::SetPerFrame(&s_perFrame);

		s_viewport.Width = static_cast<float>(windowSize.x);
		s_viewport.Height = static_cast<float>(windowSize.y);
	}

	//s_perObject.worldMatrix = glm::mat4();
	s_perObject.worldMatrix = glm::translate(glm::vec3(0, 0, 10.0f));
#endif
}

void game::Destroy() {
	// Free dynamic memory used by game here
}
