#include "starlight_game.h"
#include "starlight_input.h"
#include "starlight_log.h"
#include "starlight_generated.h"
#include "starlight_transform.h"
#include "starlight_renderer.h"
#include <process.h>
#include <cstdint>
#include <Windows.h> // for input

// shaders (generated)
// changed to defines to prevent visual studio hanging
#include "PixelShader.h"
#define PixelShaderBytes g_PixelShader
#include "VertexShader.h"
#define VertexShaderBytes g_VertexShader

struct Camera {
	float m_fieldOfView = glm::radians(60.0f); // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};


Transform m_player;
Camera s_camera;
// Todo: texture
static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;
static float s_deltaTime;

ID3D11PixelShader* s_pixelShader;

void game::Init() {
	input::Init();
	logger::Init();
	LogInfo("Сука блять!");

	m_player.SetPosition(0, 0, 32);

	// Load shader and stuff
	s_pixelShader = renderer::CreatePixelShader(PixelShaderBytes, sizeof(PixelShaderBytes));
	


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
		m_player.SetPosition(glm::vec3(0, 0, 0));
		m_player.SetRotation(glm::quat(1, 0, 0, 0));
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
			m_player.SetRotation(glm::quat(glm::vec3(currentRotation.y, currentRotation.x, 0.0f)));
			lastRotation = currentRotation;
		}
	}

	// Translation
	const float SPEED = 20.0f;
	glm::vec3 translation(0, 0, 0);
	if (input::GetKey('W'))		translation += m_player.Forward();
	if (input::GetKey('A'))		translation -= m_player.Right();
	if (input::GetKey('S'))		translation -= m_player.Forward();
	if (input::GetKey('D'))		translation += m_player.Right();
	if (input::GetKey(VK_LCONTROL) || input::GetKey('C') || input::GetKey(VK_LSHIFT)) translation -= glm::vec3(0, 1, 0);
	if (input::GetKey(VK_SPACE)) translation += glm::vec3(0, 1, 0);
	if (translation != glm::vec3(0, 0, 0))
	{
		glm::vec3 pos = m_player.GetPosition();
		pos += glm::normalize(translation) * SPEED * s_deltaTime;
		m_player.SetPosition(pos);
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

	// Begin render
#if 0
	glm::mat4 viewMatrix = m_Camera.get_ViewMatrix();
	glm::mat4 projectionMatrix = m_Camera.get_ProjectionMatrix();
	glm::mat4 viewProjectionMatrix = viewMatrix * projectionMatrix;
	glm::mat4 translationMatrix, rotationMatrix, scaleMatrix, worldMatrix;

	PerObjectConstantBufferData perObjectConstantBufferData;
	PerFrameConstantBufferData perFrameConstantBufferData;

	ID3D11Buffer *pixelShaderConstantBuffers[] = { m_d3dMaterialPropertiesConstantBuffer.Get(), m_d3dLightPropertiesConstantBuffer.Get() };

	ID3D11Buffer *vertexShaderConstantBuffers[] = { m_d3dPerObjectConstantBuffer.Get(), m_d3dPerFrameConstantBuffer.Get() };
	ID3D11ShaderResourceView *textures[] = { m_AluminumTexture.Get(), m_AluminumNormalMap.Get() };

	/************************************************************************/
	/* PER FRAME CONSTANT BUFFER                                            */
	/************************************************************************/
	perFrameConstantBufferData.ViewProjectionMatrix = viewProjectionMatrix;
	glm::vec3 camPos = m_Camera.get_Translation();
	perFrameConstantBufferData.EyePosition = XMFLOAT3(camPos.m128_f32[0], camPos.m128_f32[1], camPos.m128_f32[2]);
	m_d3dDeviceContext->UpdateSubresource(m_d3dPerFrameConstantBuffer.Get(), 0, nullptr, &perFrameConstantBufferData, 0, 0);

	// Reset shader stuff
	m_d3dDeviceContext->PSSetShader(m_reflectionPixelShader.Get(), nullptr, 0);
	m_d3dDeviceContext->PSSetSamplers(0, 1, m_d3dSamplerState.GetAddressOf()); // Sampler
	m_d3dDeviceContext->PSSetShaderResources(0, 1, m_skyboxTexture.GetAddressOf()); // TextureCube
	m_d3dDeviceContext->IASetInputLayout(m_d3dVertexPositionNormalTextureInputLayout.Get());
	m_d3dDeviceContext->OMSetRenderTargets(1, m_d3dRenderTargetView.GetAddressOf(), m_d3dDepthStencilView.Get());
	m_d3dDeviceContext->OMSetDepthStencilState(m_d3dDepthStencilState.Get(), 0);
	m_d3dDeviceContext->RSSetState(m_d3dRasterizerState.Get());
	m_d3dDeviceContext->VSSetShader(m_d3dSimpleVertexShader.Get(), nullptr, 0);

	// Constant Buffers
	m_d3dDeviceContext->VSSetConstantBuffers(0, 2, vertexShaderConstantBuffers);
	perObjectConstantBufferData.WorldMatrix = worldMatrix;
	perObjectConstantBufferData.InverseTransposeWorldMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));
	perObjectConstantBufferData.WorldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

	m_d3dDeviceContext->UpdateSubresource(m_d3dPerObjectConstantBuffer.Get(), 0, nullptr, &perObjectConstantBufferData, 0, 0);

	uint32_t strides[] = { (m_big) ? sizeof(VertexPositionNormalTextureTangentBinormal) : sizeof(VertexPositionNormalTexture) };
	const uint32_t offsets[] = { 0 };

	m_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3dDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), strides, offsets);
	m_d3dDeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_d3dDeviceContext->DrawIndexed(m_IndexCount, 0, 0);
#endif

	logger::Render();
}
