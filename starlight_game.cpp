#include "starlight_game.h"
#include "starlight_input.h"
#include "starlight_log.h"
#include "starlight_generated.h"
#include "starlight_transform.h"
#include "starlight_renderer.h"
#include "starlight_platform.h"
#include <process.h>
#include <cstdint>
#include <Windows.h>
#include <d3d11.h>
#include <glm/gtc/matrix_transform.hpp>
#include "starlight_glm.h"

// shaders (generated)
// changed to defines to prevent visual studio hanging
#include "SimplePixelShader.h"
#define PixelShaderBlob g_SimplePixelShader
#include "SimpleVertexShader.h"
#define VertexShaderBlob g_SimpleVertexShader

static ID3D11Buffer* s_constantBuffers[renderer::NumConstantBuffers];

struct Camera {
	float m_fieldOfView = glm::radians(60.0f); // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};

static Transform s_player;
static Camera s_camera;
// Todo: texture
static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;
static float s_deltaTime;

static ID3D11RasterizerState* s_rasterizerState;
static ID3D11PixelShader* s_pixelShader;
static ID3D11VertexShader* s_vertexShader;
static ID3D11InputLayout* s_inputLayout;

static D3D11_VIEWPORT s_viewport;

static PerCamera s_perCamera;
static PerFrame s_perFrame;
static PerObject s_perObject;

Mesh s_mesh;

Mesh CreateCube() {
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

	uint16_t indices[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	// Create the primitive object.
	Mesh mesh;

	// vertex buffer
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };

		bufferDesc.ByteWidth = sizeof(vertices);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

		dataDesc.pSysMem = vertices;

		D3D_TRY(renderer::GetDevice()->CreateBuffer(&bufferDesc, &dataDesc, &mesh.vertexBuffer));
	}

	// index buffer
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };

		bufferDesc.ByteWidth = sizeof(indices);
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

		dataDesc.pSysMem = indices;

		D3D_TRY(renderer::GetDevice()->CreateBuffer(&bufferDesc, &dataDesc, &mesh.indexBuffer));
	}

	mesh.numIndices = _countof(indices);

	return mesh;
}

void game::Init() {
	input::Init();
	logger::Init();
	LogInfo("Сука блять!");

	s_player.SetPosition(0, 0, -10);

	auto windowSize = platform::GetWindowSize();

	// Rasterizer State
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	D3D_TRY(renderer::GetDevice()->CreateRasterizerState(&rasterizerDesc, &s_rasterizerState));

	// Initialize the viewport to occupy the entire client area.
	s_viewport.Width = static_cast<float>(windowSize.x);
	s_viewport.Height = static_cast<float>(windowSize.y);
	s_viewport.TopLeftX = 0.0f;
	s_viewport.TopLeftY = 0.0f;
	s_viewport.MinDepth = 0.0f;
	s_viewport.MaxDepth = 1.0f;

	// Shaders
	s_pixelShader = renderer::CreatePixelShader(PixelShaderBlob, sizeof(PixelShaderBlob));
	s_vertexShader = renderer::CreateVertexShader(VertexShaderBlob, sizeof(VertexShaderBlob));

	// TODO Sampler (for texturing)


	// Input layout
	D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	D3D_TRY(renderer::GetDevice()->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), VertexShaderBlob, sizeof(VertexShaderBlob), &s_inputLayout));

	// Cube
	s_mesh = CreateCube();


	// Timing
	s_deltaTime = 0.0f;
	QueryPerformanceFrequency(&s_perfFreq);
	QueryPerformanceCounter(&s_lastTime);

	// Constant buffer descriptor
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	// Per frame constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerFrame);
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[renderer::Frame]));

	// Per camera constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerCamera);
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[renderer::Camera]));

	// Per object constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerObject);
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[renderer::Object]));
}

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

void game::Update() {
	// Timing
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	s_deltaTime = float(currentTime.QuadPart - s_lastTime.QuadPart) / float(s_perfFreq.QuadPart);
	s_lastTime = currentTime;

	// Begin logic
	input::BeginFrame();
	MoveCamera();
	input::EndFrame();

	// Does not render, but builds display lists
	logger::Render();

	auto windowSize = platform::GetWindowSize();
	if (windowSize.x > 0 && windowSize.y > 0) {
		glm::mat4 projectionMatrix = glm::perspectiveFovLH(glm::radians(45.0f), (float)windowSize.x, (float)windowSize.y, 0.1f, 100.0f);
		s_perCamera.view = s_player.GetViewMatrix();
		renderer::SetPerCamera(&s_perCamera);
		s_perFrame.projection = projectionMatrix;
		renderer::SetPerFrame(&s_perFrame);

		s_viewport.Width = static_cast<float>(windowSize.x);
		s_viewport.Height = static_cast<float>(windowSize.y);
	}

	//s_perObject.worldMatrix = glm::mat4();
	s_perObject.worldMatrix = glm::translate(glm::vec3(0, 0, 10.0f));
}

void game::CreateDrawCommands() {
	renderer::DrawCommand cmd;
	ZeroMemory(&cmd, sizeof(cmd));

	cmd.mesh = &s_mesh;
	cmd.pipelineState.inputLayout = s_inputLayout;
	cmd.pipelineState.numViewports = 1;
	cmd.pipelineState.viewports = &s_viewport;
	cmd.pipelineState.pixelShader = s_pixelShader;
	cmd.pipelineState.vertexShader = s_vertexShader;
	cmd.pipelineState.rasterizerState = s_rasterizerState;
	cmd.perObject = &s_perObject;

	renderer::AddDrawCommand(cmd);
}

void game::Destroy() {
	SafeRelease(s_rasterizerState);
	SafeRelease(s_pixelShader);
	SafeRelease(s_vertexShader);
	SafeRelease(s_inputLayout);
	SafeRelease(s_mesh.indexBuffer);
	SafeRelease(s_mesh.vertexBuffer);
}
