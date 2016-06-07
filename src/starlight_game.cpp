#include "starlight_game.h"
#include "starlight_input.h"
#include "starlight_log.h"
#include "starlight_transform.h"
#include "starlight_graphics.h"
#include <vectormath/scalar/cpp/vectormath_aos.h>
//#include <process.h> // ?
#include <cstdint>
#include "imgui.h"
#include "Network.h"
#include "Noise.h"
#include <enet/enet.h>

// temp
#include <sstream>

using namespace Vectormath::Aos;

struct Camera {
	float m_fieldOfView = 60.0f * DEG2RAD; // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};

static Transform s_player;
static float s_oldX;
static float s_oldZ;
//static Camera s_camera;
static float s_deltaTime;

#if 0
static byte3 Offsets[ESide::Count] = {
	{ -1, 0, 0 },	// West
	{ 0, -1, 0 },	// Bottom
	{ 0, 0, -1 },	// North
	{ 1, 0, 0 },	// East
	{ 0, 1, 0 },	// Top
	{ 0, 0, 1 },	// South
};
#endif

// Currently returns by value... maybe we want to return by pointer when Block becomes a struct?
// Or use SoA and this keeps being an integer.
__inline Block GetBlock(Chunk* chunk, int32_t x, int32_t y, int32_t z) {
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

struct ChunkMesh {
	int32_t chunkIdx; // internal chunk id
	int32_t mesh; // internal mesh id
};

struct ChunkMeshList {
	int32_t* remove; // To remove all chunkMeshes belonging to one chunk
	int32_t numRemove;
	ChunkMesh* add;
	int32_t numAdd;
	ChunkMesh* chunkMeshes;
	int32_t numChunkMeshes;
};

static void AddCubeTriangles(TempMesh *mesh, int32_t x, int32_t y, int32_t z) {
	static float2 uv[4] = {
		{0,0},
		{1,0},
		{0,1},
		{1,1},
	};

	float3 v { (float) x, (float) y, (float) z };

	/*
	2--3
	|\ |
	| \|
	0--1
	*/
	Vertex vertices[] = {
		// -X
		{ uv[0], v + float3{ (0.0f), 0.0f, 0.0f } },
		{ uv[1], v + float3{ (0.0f), 0.0f, 1.0f } },
		{ uv[2], v + float3{ (0.0f), 1.0f, 0.0f } },
		{ uv[3], v + float3{ (0.0f), 1.0f, 1.0f } },
		// -Y
		{ uv[0], v + float3{ 0.0f, (0.0f), 0.0f } },
		{ uv[1], v + float3{ 1.0f, (0.0f), 0.0f } },
		{ uv[2], v + float3{ 0.0f, (0.0f), 1.0f } },
		{ uv[3], v + float3{ 1.0f, (0.0f), 1.0f } },
		// -Z
		{ uv[0], v + float3{ 1.0f, 0.0f, (0.0f) } },
		{ uv[1], v + float3{ 0.0f, 0.0f, (0.0f) } },
		{ uv[2], v + float3{ 1.0f, 1.0f, (0.0f) } },
		{ uv[3], v + float3{ 0.0f, 1.0f, (0.0f) } },
		// +X
		{ uv[0], v + float3{ (1.0f), 0.0f, 1.0f } },
		{ uv[1], v + float3{ (1.0f), 0.0f, 0.0f } },
		{ uv[2], v + float3{ (1.0f), 1.0f, 1.0f } },
		{ uv[3], v + float3{ (1.0f), 1.0f, 0.0f } },
		// +Y
		{ uv[0], v + float3{ 0.0f, (1.0f), 1.0f } },
		{ uv[1], v + float3{ 1.0f, (1.0f), 1.0f } },
		{ uv[2], v + float3{ 0.0f, (1.0f), 0.0f } },
		{ uv[3], v + float3{ 1.0f, (1.0f), 0.0f } },
		// +Z
		{ uv[0], v + float3{ 0.0f, 0.0f, (1.0f) } },
		{ uv[1], v + float3{ 1.0f, 0.0f, (1.0f) } },
		{ uv[2], v + float3{ 0.0f, 1.0f, (1.0f) } },
		{ uv[3], v + float3{ 1.0f, 1.0f, (1.0f) } },
	};

	int32_t indices[36];
	int32_t i = (int32_t) mesh->vertices.size();
	for(int32_t j = 0; j < 36; j += 6) {
		indices[j + 0] = i + 0;
		indices[j + 1] = i + 1;
		indices[j + 2] = i + 2;
		indices[j + 3] = i + 2;
		indices[j + 4] = i + 1;
		indices[j + 5] = i + 3;
		i += 4;
	}

	mesh->vertices.insert(mesh->vertices.end(), vertices, vertices + _countof(vertices));
	mesh->indices.insert(mesh->indices.end(), indices, indices + _countof(indices));
}

static int32_t s_chunk;

// MeshList contains all meshes related to chunks.
static void UpdateMeshList(GameInfo* gameInfo, graphics::API* graphics, int32_t) {
	assert(gameInfo);
	assert(gameInfo->chunkPool);
	//assert(gameInfo->chunkGrid);
	// Remove meshes related to this chunk
	// Maybe this needs to be queued

	//meshList->remove[meshList->numRemove++] = chunkIdx;

	TempMesh mesh;

	// Y->Z->X
	for (size_t cx = 0; cx < CHUNK_DIAMETER; cx++) {
		for (size_t cz = 0; cz < CHUNK_DIAMETER; cz++) {
			// Blocks
			for (int32_t y = 0; y < CHUNK_DIM_Y; y++) {
				for (int32_t bz = 0; bz < CHUNK_DIM_XZ; bz++) {
					for (int32_t bx = 0; bx < CHUNK_DIM_XZ; bx++) {
						Chunk* chunk = gameInfo->chunkGrid[cx * CHUNK_DIAMETER + cz];
						if (GetBlock(chunk, bx, y, bz) != 0) {
							AddCubeTriangles(&mesh,
								chunk->position.x * CHUNK_DIM_XZ + bx,
								y,
								chunk->position.z * CHUNK_DIM_XZ + bz);
						}
					}
				}
			}
		}
	}

	//graphics
	s_chunk = graphics->AddChunk(&mesh);
}


void GenerateChunk(Chunk* chunk, int32_t cx, int32_t cz) {
	chunk->position.x = cx;
	chunk->position.z = cz;

	// Blocks
#if 0
	for (int32_t bz = 0; bz < CHUNK_DIM_XZ; bz++) {
		for (int32_t bx = 0; bx < CHUNK_DIM_XZ; bx++) {
			// Get height from noise
			float sample = noise::perlin::Noise(&state, 0.01f * (float)(cx * CHUNK_DIM_XZ + bx), 0.01f * (float)(cz * CHUNK_DIM_XZ + bz), 0.0f);
			size_t height = (size_t)(sample * 32) + 64;
			if (height < 0) height = 0;
			if (height >= CHUNK_DIM_Y) height = CHUNK_DIM_Y - 1;
			//for (int32_t y = 0; y < height; y++) {
			SetBlock(&gameInfo->chunkPool[cx * CHUNK_DIAMETER + cz], 1, bx, (int32_t)height, bz);
			//}
		}
	}
#endif
	// Floor
	for (int32_t bz = 0; bz < CHUNK_DIM_XZ; bz++) {
		for (int32_t bx = 0; bx < CHUNK_DIM_XZ; bx++) {
			SetBlock(chunk, 1, bx, 0, bz);
		}
	}
	if (cx < 0) {
		SetBlock(chunk, 0, 2, 0, 0);
	}
	if (cz < 0) {
		SetBlock(chunk, 0, 0, 0, 2);
	}
	for (int32_t x = 0; x <= abs(cx); x++) {
		// X
		SetBlock(chunk, 1, 1, x, 0);
	}
	for (int32_t z = 0; z <= abs(cz); z++) {
		// Z
		SetBlock(chunk, 1, 0, z, 1);
	}
}

void UpdateChunkGrid(GameInfo* gameInfo) {
	assert(gameInfo->chunkGrid);
	assert(gameInfo->chunkPool);

	// Clear chunkGrid
	ZERO_MEM(gameInfo->chunkGrid, sizeof(Chunk*) * NUM_CHUNKS);

	// define center chunk
	// TODO: Might be wise to store integer position in player struct
	// And then offset with a float?
	Vector3 playerPos = s_player.GetPosition();

	// TODO: Maybe change this when player is at the edge of the world
	// So we don't drop chunks for nothing
	int32_t px = (int32_t)floorf(playerPos.getX() / CHUNK_DIM_XZ);
	int32_t pz = (int32_t)floorf(playerPos.getZ() / CHUNK_DIM_XZ);
	int2 basePos = { px, pz };

	// pseudocode:
	// clear grid
	// find persistent chunks: flag active, add to grid
	// mark rest inactive
	
	// find missing grid entries
	// call addChunk and add to grid

	Chunk* freeChunks[CHUNK_DIAMETER * CHUNK_DIAMETER] = { 0 };
	size_t numFreeChunks = 0;

	for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
		for (size_t z = 0; z < CHUNK_DIAMETER; z++) {
			Chunk* chunk = &gameInfo->chunkPool[x * CHUNK_DIAMETER + z];
			int2 relativePos = chunk->position - basePos;
			if (chunk->inUse && abs(relativePos.x) < CHUNK_RADIUS && abs(relativePos.z) < CHUNK_RADIUS) {
				// Set grid pointer to this chunk
				size_t gx = ((size_t) relativePos.x + CHUNK_RADIUS);
				size_t gz = ((size_t) relativePos.z + CHUNK_RADIUS);
				assert(gx < CHUNK_DIAMETER);
				assert(gz < CHUNK_DIAMETER);
				gameInfo->chunkGrid[gx * CHUNK_DIAMETER + gz] = chunk;
			}
			else {
				chunk->inUse = false;
				assert(numFreeChunks < NUM_CHUNKS);
				freeChunks[numFreeChunks++] = chunk;
			}
		}
	}

	// Add missing chunks
	for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
		for (size_t z = 0; z < CHUNK_DIAMETER; z++) {
			if (!gameInfo->chunkGrid[x * CHUNK_DIAMETER + z]) {
				// Missing chunk found, find space in chunkPool
				assert(numFreeChunks > 0);
				Chunk* freeChunk = freeChunks[--numFreeChunks];
				GenerateChunk(freeChunk, x, z);
				gameInfo->chunkGrid[x * CHUNK_DIAMETER + z] = freeChunk;
				freeChunk->inUse = true;
			}
		}
	}
}

void Init(GameInfo* gameInfo, graphics::API* graphicsApi) {
	input::Init();

	ImGui::SetCurrentContext(gameInfo->imguiState);

	// TODO: Move this stuff after a main menu etc.
	s_player.SetPosition(8.5f, 65.0f, 8.5f);
	s_oldX = s_player.GetPosition().getX();
	s_oldZ = s_player.GetPosition().getZ();

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

	gameInfo->numChunks = NUM_CHUNKS;

	//noise::perlin::State state = { 0 };
	//noise::perlin::Initialize(&state, 0);

	// Chunkpool needs to be zeromem'd (set inUse flags to false)
	gameInfo->chunkPool = new Chunk[NUM_CHUNKS];
	ZERO_MEM(gameInfo->chunkPool, sizeof(Chunk) * NUM_CHUNKS);

	// chunkGrid is cleared at UpdateChunkGrid
	gameInfo->chunkGrid = new Chunk*[NUM_CHUNKS];
	UpdateChunkGrid(gameInfo);

	UpdateMeshList(gameInfo, graphicsApi, 0);
}

void MoveCamera() {
	// TODO: probably need to check if i'm typing something in ImGui or not
	static float2 lastRotation;
	static float2 currentRotation;

	if (ImGui::GetIO().KeysDown[(intptr_t)'M'])
	{
		input::SetMouseGrabbed(!input::IsMouseGrabbed());
	}

	// Reset
	if (ImGui::GetIO().KeysDown[(intptr_t)'R'])
	{
		currentRotation = lastRotation = { 0, 0 };
		s_player.SetPosition(8.5f, 65.0f, 8.5f);
		s_player.SetRotation(Quat(0, 0, 0, 1));
	}

	//if (input::IsMouseGrabbed())
	{
		// Rotation
		//const float ROT_SPEED = 0.0025f;
		if (ImGui::GetIO().KeysDown[(intptr_t)'Q'])		currentRotation.y += 2.0f * s_deltaTime;
		if (ImGui::GetIO().KeysDown[(intptr_t)'E'])		currentRotation.y -= 2.0f * s_deltaTime;
		if (ImGui::GetIO().KeysDown[(intptr_t)'Z'])		currentRotation.x += 2.0f * s_deltaTime;
		if (ImGui::GetIO().KeysDown[(intptr_t)'X'])		currentRotation.x -= 2.0f * s_deltaTime;
		//currentRotation -= ROT_SPEED * input::GetMouseDelta();
		if (currentRotation.x < -89.0f * DEG2RAD)
		{
			currentRotation.x = -89.0f * DEG2RAD;
		}
		if (currentRotation.x > 89.0f * DEG2RAD)
		{
			currentRotation.x = 89.0f * DEG2RAD;
		}
		if (currentRotation.x != lastRotation.x || currentRotation.y != lastRotation.y)
		{
			s_player.SetRotation(Quat::rotationY(currentRotation.y) * Quat::rotationX(currentRotation.x));
			lastRotation = currentRotation;
		}
	}

	// Translation
	const float SPEED = 10.0f;
	Vector3 translation(0, 0, 0);
	if (ImGui::GetIO().KeysDown[(intptr_t)'W'])		translation += s_player.Forward();
	if (ImGui::GetIO().KeysDown[(intptr_t)'A'])		translation -= s_player.Right();
	if (ImGui::GetIO().KeysDown[(intptr_t)'S'])		translation -= s_player.Forward();
	if (ImGui::GetIO().KeysDown[(intptr_t)'D'])		translation += s_player.Right();
	if (ImGui::GetIO().KeysDown[VK_LCONTROL] || ImGui::GetIO().KeysDown[(intptr_t)'C'] || ImGui::GetIO().KeysDown[VK_LSHIFT]) translation -= Vector3(0, 1, 0);
	if (ImGui::GetIO().KeysDown[VK_SPACE]) translation += Vector3(0, 1, 0);
	if (lengthSqr(translation) != 0.0f)
	{
		Vector3 pos = s_player.GetPosition();
		pos += normalize(translation) * SPEED * s_deltaTime;
		s_player.SetPosition(pos);
		//printf("pos: %.1f, %.1f, %.1f\n", m_player.GetPosition().x, m_player.GetPosition().y, m_player.GetPosition().z);
	}
}

extern "C"
__declspec(dllexport)
void __cdecl game::UpdateGame(GameInfo* gameInfo, graphics::API* graphicsApi) {
	if(!gameInfo->initialized) {
		Init(gameInfo, graphicsApi);
		gameInfo->initialized = true;
	}

	// Timing
	s_deltaTime = gameInfo->CalculateDeltaTime();

	// Begin logic
	input::BeginFrame();

	static bool showMainMenuBar = true;
	if(ImGui::GetIO().KeysDown[VK_F3]) {
		showMainMenuBar = !showMainMenuBar;
	}

	if(showMainMenuBar) {
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit", nullptr, nullptr)) {
				// TODO
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	MoveCamera();
	input::EndFrame();

	float newX = floorf(s_player.GetPosition().getX());
	float newZ = floorf(s_player.GetPosition().getZ());
	if (newX != s_oldX || newZ != s_oldZ) {
		UpdateChunkGrid(gameInfo);
		s_oldX = newX;
		s_oldZ = newZ;
	}

	network::Update(gameInfo);
	//network::DrawDebugMenu(gameInfo);

	// Debug menu
	bool stay = true;
	ImGui::Begin("game::UpdateGame", &stay);
	//if (ImGui::Button("Load D3D10")) gameInfo->graphicsApi = EGraphicsApi::D3D10;
	//if (ImGui::Button("Load D3D11")) gameInfo->graphicsApi = EGraphicsApi::D3D11;
	Vector3 pos = s_player.GetPosition();
	ImGui::InputFloat3("Position", (float*) &pos);
	Quat rot = s_player.GetRotation();
	ImGui::InputFloat4("Rotation", (float*)&rot);
	ImGui::End();

	// Network chat test
#if 0
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
		
		logger::LogInfo(std::string(buf));
		ZERO_MEM(buf, sizeof(buf));
	}
	if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
		ImGui::SetKeyboardFocusHere(-1); // Auto focus
	ImGui::End();
#endif

	// Does not render, but builds display lists
	//logger::Render();

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
	s_perObject.worldMatrix = glm::translate(Vector3(0, 0, 10.0f));
#endif
}

extern "C"
__declspec(dllexport)
void __cdecl game::DestroyGame() {
	// Free dynamic memory used by game here
}
