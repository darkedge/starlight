#include "starlight_game.h"
#include "starlight_input.h"
#include "starlight_log.h"
#include "starlight_transform.h"
#include "starlight_graphics.h"
#include "starlight_platform.h"
#include "starlight_generated.h"
//#include <process.h> // ?
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include "starlight_glm.h"
#include "imgui.h"
#include "Network.h"
#include "Noise.h"
#include <enet/enet.h>

// temp
#include <sstream>

struct Camera {
	float m_fieldOfView = glm::radians(60.0f); // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};

static Transform s_player;
static Camera s_camera;
static float s_deltaTime;

static glm::ivec3 Offsets[ESide::Count] = {
	{ -1, 0, 0 },	// West
	{ 0, -1, 0 },	// Bottom
	{ 0, 0, -1 },	// North
	{ 1, 0, 0 },	// East
	{ 0, 1, 0 },	// Top
	{ 0, 0, 1 },	// South
};

// Currently returns by value... maybe we want to return by pointer when Block becomes a struct?
inline Block GetBlock(Chunk* chunk, int32_t x, int32_t y, int32_t z) {
	assert(chunk);
	assert(x >= 0 && x < CHUNK_DIM_XZ);
	assert(y >= 0 && y < CHUNK_DIM_Y);
	assert(z >= 0 && z < CHUNK_DIM_XZ);
	return chunk->blocks[y * CHUNK_DIM_XZ * CHUNK_DIM_XZ + z * CHUNK_DIM_XZ + x];
}

inline void SetBlock(Chunk* chunk, Block block, int32_t x, int32_t y, int32_t z) {
	// Chunk storage format: Y->Z->X
	//  / Y
	//  --> X
	// |
	// V Z
	assert(chunk);
	assert(x >= 0 && x < CHUNK_DIM_XZ);
	assert(y >= 0 && y < CHUNK_DIM_Y);
	assert(z >= 0 && z < CHUNK_DIM_XZ);
	chunk->blocks[y * CHUNK_DIM_XZ * CHUNK_DIM_XZ + z * CHUNK_DIM_XZ + x] = block;
}

static void UpdateChunkMesh(Chunk* chunk) {
	TempMesh mesh;

	/*
	* These are just working variables for the algorithm - almost all taken
	* directly from Mikola Lysenko's javascript implementation.
	*/
	glm::tvec3<int32_t> x{ 0, 0, 0 };
	glm::tvec3<int32_t> q{ 0, 0, 0 };
	glm::tvec3<int32_t> du{ 0, 0, 0 };
	glm::tvec3<int32_t> dv{ 0, 0, 0 };

	/*
	* We create a mask - this will contain the groups of matching voxel faces
	* as we proceed through the chunk in 6 directions - once for each face.
	*/
	Block mask[CHUNK_DIM_XZ * CHUNK_DIM_Y] = { 0 };

	/*
	We start with the lesser-spotted bool for-loop (also known as the old flippy floppy).

	The variable backFace will be TRUE on the first iteration and FALSE on the second - this allows
	us to track which direction the indices should run during creation of the quad.

	This loop runs twice, and the inner loop 3 times - totally 6 iterations - one for each
	voxel face.
	*/

	// Which way the quad is facing
	glm::tvec3<int32_t> sides[6] = {
		{ -1, 0, 0 },	// West
		{ 0, -1, 0 },	// Bottom
		{ 0, 0, -1 },	// North
		{ 1, 0, 0 },	// East
		{ 0, 1, 0 },	// Top
		{ 0, 0, 1 },	// South
	};

	glm::tvec3<int32_t> side;

	for (int32_t p = 0; p < 6; p++)
	{
		int32_t d = p % 3;
		bool backFace = p < 3;

		// 0 1 2 3 4 5
		int32_t u = (d + 1) % 3; // 1 2 0
		int32_t v = (d + 2) % 3; // 2 0 1

		x[0] = 0;
		x[1] = 0;
		x[2] = 0;

		q[0] = 0;
		q[1] = 0;
		q[2] = 0;
		q[d] = 1;

		// Here we're keeping track of the side that we're meshing.
		side = sides[p];

		// We move through the dimension from front to back
		for (x[d] = -1; x[d] < CHUNK_DIM_XZ;)
		{
			// We compute the mask
			int32_t n = 0;

			for (x[v] = 0; x[v] < CHUNK_DIM_Y; x[v]++)
			{
				for (x[u] = 0; x[u] < CHUNK_DIM_XZ; x[u]++)
				{
					// Here we retrieve two voxel faces for comparison.
					Block block0 = 0;
					Block block1 = 0;
					if (x[d] >= 0)
					{
						auto f = x + side;
						block0 = GetBlock(chunk, f.x, f.y, f.z);
					}
					if (x[d] < CHUNK_DIM_XZ - 1)
					{
						auto f = x + q + side;
						block1 = GetBlock(chunk, f.x, f.y, f.z);
					}

					/*
					* Note that we're using the equals function in the voxel face class here, which lets the faces
					* be compared based on any number of attributes.
					*
					* Also, we choose the face to add to the mask depending on whether we're moving through on a backface or not.
					*/
					if (block0 == block1)
					{
						mask[n] = 0;
					}
					else
					{
						if (backFace)
						{
							mask[n] = block1;
						}
						else
						{
							mask[n] = block0;
						}
					}
					n++;
				} // End u loop
			} // End v loop

			x[d]++;

			// Now we generate the mesh for the mask
			n = 0;

			for (int32_t j = 0; j < CHUNK_DIM_Y; j++)
			{
				for (int32_t i = 0; i < CHUNK_DIM_XZ;)
				{
					if (mask[n])
					{
						int32_t width = 0;
						int32_t height = 0;
						// Compute width
						for (width = 1; i + width < CHUNK_DIM_XZ && mask[n + width] != 0 && mask[n + width] == mask[n]; width++) {}

						// Compute height
						bool done = false;
						for (height = 1; j + height < CHUNK_DIM_Y; height++)
						{
							for (int32_t k = 0; k < width; k++)
							{
								if (mask[n + k + height * CHUNK_DIM_XZ] == 0 || !mask[n + k + height * CHUNK_DIM_XZ] == mask[n])
								{
									done = true;
									break;
								}
							}

							if (done)
							{
								break;
							}
						}

						/*
						* Here we check the "transparent" attribute in the Block class to ensure that we don't mesh
						* any culled faces.
						*/
						if (mask[n] != 0)
						{
							/*
							* Add quad
							*/
							x[u] = i;
							x[v] = j;

							du[0] = 0;
							du[1] = 0;
							du[2] = 0;
							du[u] = width;

							dv[0] = 0;
							dv[1] = 0;
							dv[2] = 0;
							dv[v] = height;

							/*
							* And here we call the quad function in order to render a merged quad in the scene.
							*
							* We pass mask[n] to the function, which is an instance of the Block class containing
							* all the attributes of the face - which allows for variables to be passed to shaders - for
							* example lighting values used to create ambient occlusion.
							*/
							//mask[n]->side = side;
							//AddBlockFaceTris(mesh,
							glm::vec3 bottomLeft(x.x, x.y, x.z);
							glm::vec3 topLeft(x.x + du.x, x.y + du.y, x.z + du.z);
							glm::vec3 topRight(x.x + du.x + dv.x, x.y + du.y + dv.y, x.z + du.z + dv.z);
							glm::vec3 bottomRight(x.x + dv.x, x.y + dv.y, x.z + dv.z);
							//Block voxel = mask[n];

							glm::vec3 vertices[4] = {
								bottomLeft,
								topLeft,
								topRight,
								bottomRight,
							};

							if (p == ESide::North || p == ESide::South)
							{
								std::swap(width, height);
							}

							// Rotate by shifting by one
							glm::vec2 texcoords[] = {
								glm::vec2(0.0f, float(width)),
								glm::vec2(float(height), float(width)),
								glm::vec2(float(height), 0.0f),
								glm::vec2(0.0f, 0.0f),
								glm::vec2(0.0f, float(width)),
							};

							glm::vec2 flipped[] = {
								glm::vec2(float(height), float(width)),
								glm::vec2(0.0f, float(width)),
								glm::vec2(0.0f, 0.0f),
								glm::vec2(float(height), 0.0f),
								glm::vec2(float(height), float(width)),
							};

							glm::vec2 *texcoordptr = texcoords;

							int32_t indices[6];
							{
								if (backFace)
								{
									int32_t a[6] = { 0, 3, 2, 2, 1, 0 };
									memcpy(indices, a, sizeof(indices));
								}
								else
								{
									int32_t a[6] = { 0, 1, 2, 2, 3, 0 };
									memcpy(indices, a, sizeof(indices));
								}
							}

							// Offset with existing data count
							for (int32_t k = 0; k < 6; k++)
							{
								indices[k] += (int32_t) mesh.indices.size();
							}

							switch (p)
							{
							case ESide::North:
								texcoordptr = flipped;
								break;
							case ESide::East:
								texcoordptr = texcoords + 1;
								break;
							case ESide::South:
								texcoordptr = texcoords;
								break;
							case ESide::West:
								texcoordptr = flipped + 1;
								break;
							case ESide::Top:
								texcoordptr = texcoords + 1;
								break;
							case ESide::Bottom:
								texcoordptr = flipped + 1;
								break;
							default:
								break;
							}

							for (int32_t k = 0; k < 4; k++) {
								Vertex vertex;
								vertex.position = vertices[k];
								vertex.texCoord = texcoordptr[k];
								mesh.vertices.push_back(vertex);
							}

							mesh.indices.insert(mesh.indices.end(), &indices[0], indices + 6);
						}

						// We zero out the mask
						for (int32_t l = 0; l < height; l++)
						{
							for (int32_t k = 0; k < width; k++)
							{
								mask[n + k + l * CHUNK_DIM_XZ] = 0;
							}
						}


						// And then finally increment the counters and continue
						i += width;
						n += width;
					}
					else
					{
						i++;
						n++;
					}
				}
			} // End mesh loop
		} // End chunk size loop
	} // End axis loop
}

#if 0
int32_t s_mesh;

int32_t CreateCube(graphics::API* graphicsApi) {
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

	return graphicsApi->UploadMesh(vertices, COUNT_OF(vertices), indices, COUNT_OF(indices));
}
#endif

void Init(GameInfo* gameInfo, graphics::API* graphicsApi) {
	input::Init();

	//assert(!gameInfo->chunks);
	//gameInfo->chunks = new Chunk[BUFFER_CHUNK_COUNT];
	//ZERO_MEM(gameInfo->chunks, BUFFER_CHUNK_COUNT * sizeof(Chunk));
	
	// TODO: Move this stuff after a main menu etc.
	s_player.SetPosition(0, 0, -10);

	// Cube
	//s_mesh = CreateCube(graphicsApi);
	
	// Timing
	s_deltaTime = 0.0f;

	// Addon loading
	// This should happen according to a configuration file.
	// So in the root folder there would be an /addons/ folder
	// which contains a separate folder for every addon
	// and a file which lists the ones to be loaded (file could also be in root).

	// Open file, read contents in one go
	// Allocate memory to contain the list of addons
	// Then, for every line, load that addon (or something)
	// It's useful to have an error and output stream here
	// Even basic gameplay can (should?) be loaded as an addon

	// Create one chunk

	gameInfo->numChunks = 1;
	gameInfo->chunks = new Chunk;
	ZERO_MEM(gameInfo->chunks, sizeof(Chunk));
	for (int32_t y = 0; y < CHUNK_DIM_Y / 2; y++) {
		for (int32_t x = 0; x < CHUNK_DIM_XZ; x++) for (int32_t z = 0; z < CHUNK_DIM_XZ; z++) {
			SetBlock(gameInfo->chunks, 1, x, y, z);
		}
	}

	//graphicsApi->CreateMesh();

	noise::perlin::State* state = new noise::perlin::State;
	ZERO_MEM(state, sizeof(*state));
	noise::perlin::Initialize(state, 0);
	float f = noise::perlin::Noise(state, 0.5f, 0.5f, 0.0f);
	f = f;
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

void game::Update(GameInfo* gameInfo, graphics::API* graphicsApi) {
	if(!gameInfo->initialized) {
		Init(gameInfo, graphicsApi);
		gameInfo->initialized = true;
	}

	// Timing
	s_deltaTime = platform::CalculateDeltaTime();

	// Begin logic
	input::BeginFrame();
	//MoveCamera(); // TODO
	input::EndFrame();

	network::Update(gameInfo);
	network::DrawDebugMenu(gameInfo);

	// Debug menu
	bool stay = true;
	ImGui::Begin("Debug Menu", &stay);
	if (ImGui::Button("Load D3D10")) gameInfo->graphicsApi = EGraphicsApi::D3D10;
	if (ImGui::Button("Load D3D11")) gameInfo->graphicsApi = EGraphicsApi::D3D11;
	ImGui::End();

	// Network chat test
	static char buf[128];
	ImGui::Begin("Chat Input");
	if (ImGui::InputText("Chat", buf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		flatbuffers::FlatBufferBuilder builder;
		auto str = builder.CreateString(buf);
		auto chat = network::CreateChat(builder, str);
		auto pkg = network::CreatePacket(builder, network::Message::Chat, chat.Union());
		builder.Finish(pkg);
		ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(),	ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(gameInfo->peer, 0, packet);
		//enet_host_flush (host);
		
		logger::LogInfo(std::string(buf));
		ZERO_MEM(buf, sizeof(buf));
	}
	if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
		ImGui::SetKeyboardFocusHere(-1); // Auto focus
	ImGui::End();

	// Does not render, but builds display lists
	logger::Render();

	graphicsApi->SetPlayerCameraViewMatrix(s_player.GetViewMatrix());

	// Gameplay code concept
#if 0
	// Poll input? TODO: Have an abstraction here (reconfigurable controls etc)
	if (input::GetKeyDown(input::)) {
		
	}

	// Tick all running add-ons
	// For this, they need to be registered
	// Mods cannot be added at runtime (that would be overkill)
	// So that array can be allocated once
#endif


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
