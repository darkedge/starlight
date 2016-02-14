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

#include <sstream>
#define STR(x) #x
#define XSTR(x) STR(x)
#define D3D_TRY(expr) \
do { \
	HRESULT	hr = expr; \
	if (FAILED( hr )) { \
		std::stringstream s;\
		s << __FILE__ << "(" << __LINE__ << "): " << STR(expr) << " failed";\
		logger::LogInfo(s.str());\
		_CrtDbgBreak(); \
	} \
} while (0)


// shaders (generated)
// changed to defines to prevent visual studio hanging
#include "SimplePixelShader.h"
#define PixelShaderBlob g_SimplePixelShader
#include "SimpleVertexShader.h"
#define VertexShaderBlob g_SimpleVertexShader

struct Camera {
	float m_fieldOfView = glm::radians(60.0f); // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
};

struct Mesh {
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_indexBuffer = nullptr;
	int32_t m_indexCount = -1;
};

static Transform s_player;
static Camera s_camera;
// Todo: texture
static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;
static float s_deltaTime;

static ID3D11RasterizerState* s_rasterizerState = nullptr;

static ID3D11PixelShader* s_pixelShader = nullptr;
static ID3D11VertexShader* s_vertexShader = nullptr;
static ID3D11InputLayout* s_inputLayout = nullptr;

static D3D11_VIEWPORT s_viewport;

enum ConstantBuffer
{
	CB_Appliation,
	CB_Frame,
	CB_Object,
	NumConstantBuffers
};

ID3D11Buffer* s_constantBuffers[NumConstantBuffers];

Mesh s_mesh;

Mesh CreateCube() {
	Vertex vertices[8] =
	{
		{ {-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f} }, // 0
		{ {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f} }, // 1
		{ {1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 0.0f} }, // 2
		{ {1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f} }, // 3
		{ {-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f} }, // 4
		{ {-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f} }, // 5
		{ {1.0f,  1.0f,  1.0f}, {1.0f, 1.0f, 1.0f} }, // 6
		{ {1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f} }  // 7
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

		D3D_TRY(renderer::GetDevice()->CreateBuffer(&bufferDesc, &dataDesc, &mesh.m_vertexBuffer));
	}

	// index buffer
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };

		bufferDesc.ByteWidth = sizeof(indices);
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

		dataDesc.pSysMem = indices;

		D3D_TRY(renderer::GetDevice()->CreateBuffer(&bufferDesc, &dataDesc, &mesh.m_indexBuffer));
	}

	mesh.m_indexCount = _countof(indices);

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

	// Constant buffer
	// Create the constant buffers for the variables defined in the vertex shader.
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(glm::mat4);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[CB_Appliation]));
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[CB_Frame]));
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[CB_Object]));


	// Cube
	s_mesh = CreateCube();


	// Timing
	s_deltaTime = 0.0f;
	QueryPerformanceFrequency(&s_perfFreq);
	QueryPerformanceCounter(&s_lastTime);
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

	ID3D11DeviceContext* context = renderer::GetDeviceContext();

	auto windowSize = platform::GetWindowSize();

	// Constant Buffers
	//glm::mat4 projectionMatrix = glm::perspectiveLH(s_camera.m_fieldOfView, 16.0f / 9.0f, s_camera.m_zNear, s_camera.m_zFar);
	glm::mat4 projectionMatrix = glm::perspectiveFovLH(glm::radians(45.0f), (float) windowSize.x, (float) windowSize.y, 0.1f, 100.0f);
	context->UpdateSubresource(s_constantBuffers[CB_Appliation], 0, nullptr, &projectionMatrix, 0, 0);
	glm::mat4 viewMatrix = s_player.GetViewMatrix();
	context->UpdateSubresource(s_constantBuffers[CB_Frame], 0, nullptr, &viewMatrix, 0, 0);
	glm::mat4 identity;
	context->UpdateSubresource(s_constantBuffers[CB_Object], 0, nullptr, &identity, 0, 0);

	// Input Assembler
	const uint32_t stride = sizeof(Vertex);
	const uint32_t offset = 0;
	context->IASetInputLayout(s_inputLayout);
	context->IASetVertexBuffers(0, 1, &s_mesh.m_vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(s_mesh.m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Vertex Shader
	context->VSSetShader(s_vertexShader, nullptr, 0);
	context->VSSetConstantBuffers(0, 3, s_constantBuffers);

	// Rasterizer
	context->RSSetState(s_rasterizerState);
	context->RSSetViewports(1, &s_viewport);

	// Pixel Shader
	context->PSSetShader(s_pixelShader, nullptr, 0);

	// Output Merger
	//context->OMSetRenderTargets(1, &renderer::GetRenderTargetView(), renderer::GetDepthStencilView());
	context->OMSetRenderTargets(1, &renderer::GetRenderTargetView(), nullptr);
	context->OMSetDepthStencilState(renderer::GetDepthStencilState(), 1);

	// Draw call
	context->DrawIndexed(s_mesh.m_indexCount, 0, 0);

	// Restore modified state
	context->IASetInputLayout(nullptr);
	context->PSSetShader(nullptr, nullptr, 0);
	context->VSSetShader(nullptr, nullptr, 0);

	// Does not render, but builds display lists
	logger::Render();
}
