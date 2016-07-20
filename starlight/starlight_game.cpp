#include "starlight_game.h"
#include "starlight_input.h"
#include "starlight_log.h"
#include "starlight_transform.h"
#include "starlight_graphics.h"
#include <vectormath/scalar/cpp/vectormath_aos.h>
#include <cstdint>
#include "imgui.h"
#include "Network.h"
#include "Noise.h"
#include <enet/enet.h>

#include "starlight_d3d11.h"

// temp
#include <sstream>

using namespace Vectormath::Aos;

struct Camera {
	float m_fieldOfView = 60.0f * DEG2RAD; // Field of view angle (radians)
	float m_zNear = 0.3f;
	float m_zFar = 1000.0f;
};

static Transform s_player;
static int2 s_oldPlayerChunkPosition;
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
__inline Block GetBlock(Chunk* chunk, size_t x, size_t y, size_t z) {
	assert(chunk);
	assert(x >= 0 && x < CHUNK_DIM_XZ);
	assert(y >= 0 && y < CHUNK_DIM_Y);
	assert(z >= 0 && z < CHUNK_DIM_XZ);
	return chunk->blocks[y * CHUNK_DIM_XZ * CHUNK_DIM_XZ + z * CHUNK_DIM_XZ + x];
}

inline void SetBlock(Chunk* chunk, Block block, size_t x, size_t y, size_t z) {
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

static int2 WorldToChunkPosition(float x, float z) {
	int2 xz;
	xz.x = (int32_t)floorf(x / CHUNK_DIM_XZ);
	xz.z = (int32_t)floorf(z / CHUNK_DIM_XZ);
	return xz;
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

static void* GenerateChunkMesh(GameInfo* gameInfo, Chunk* chunk, int32_t cx, int32_t cz) {
	// Maybe this needs to be queued
	// TODO: Smarter meshing

	TempMesh mesh;

	// Y->Z->X
	for (int32_t y = 0; y < CHUNK_DIM_Y; y++) {
		for (int32_t bz = 0; bz < CHUNK_DIM_XZ; bz++) {
			for (int32_t bx = 0; bx < CHUNK_DIM_XZ; bx++) {
				if (GetBlock(chunk, bx, y, bz) != 0) {
					AddCubeTriangles(&mesh,
						cx * CHUNK_DIM_XZ + bx,
						y,
						cz * CHUNK_DIM_XZ + bz);
				}
			}
		}
	}

	//graphics
	return gameInfo->gfxFuncs->AddChunk(&mesh);
}


void GenerateChunk(Chunk* chunk, int32_t cx, int32_t cz) {
	//logger::LogInfo(std::string("Generating chunk: ") + std::to_string(cx) + ", " + std::to_string(cz));
	chunk->position.x = cx;
	chunk->position.z = cz;

	noise::perlin::State state = { 0 };
	noise::perlin::Initialize(&state, 0);

	// Blocks
	for (int32_t bz = 0; bz < CHUNK_DIM_XZ; bz++) {
		for (int32_t bx = 0; bx < CHUNK_DIM_XZ; bx++) {
			// Get height from noise
			float sample = noise::perlin::Noise(&state, 0.01f * (float)(cx * CHUNK_DIM_XZ + bx), 0.01f * (float)(cz * CHUNK_DIM_XZ + bz), 0.0f);
			size_t height = (size_t)(sample * 32) + 64;
			if (height < 0) height = 0;
			if (height >= CHUNK_DIM_Y) height = CHUNK_DIM_Y - 1;
			//for (int32_t y = 0; y < height; y++) {
			SetBlock(chunk, 1, bx, (int32_t)height, bz);
			//}
		}
	}
#if 0
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
#endif
}

void UpdateChunkGrid(GameInfo* gameInfo) {
	assert(gameInfo->chunkGrid);
	assert(gameInfo->chunkPool);

	Vector3 pos = s_player.GetPosition();
	int2 chunkPos = WorldToChunkPosition(pos.getX(), pos.getZ());

	int2 dc = chunkPos - s_oldPlayerChunkPosition;
	//logger::LogInfo(std::string("dc: ") + std::to_string(dc.x) + ", " + std::to_string(dc.z));

	// Movement in X-axis
	if (dc.x > 0) {
		// Copy right to left
		for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
			for(size_t z = 0; z < CHUNK_DIAMETER; z++) {
				if (x < dc.x) {
					// This chunk is going to be overwritten
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					if (chunk->data) gameInfo->gfxFuncs->DeleteChunk(chunk->data);
					chunk->data = nullptr;
					if (chunk->chunk) ZERO_MEM(chunk->chunk, sizeof(Chunk)); // TODO: Necessary?
				}
				if (x + dc.x < CHUNK_DIAMETER) {
					// Move chunk
					gameInfo->chunkGrid[x * CHUNK_DIAMETER + z] = gameInfo->chunkGrid[(x + dc.x) * CHUNK_DIAMETER + z];	
				} else {
					// Generate new chunk here later
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					// TODO: Only unload if needed
					if (chunk->chunk) chunk->chunk->loaded = false;
					ZERO_MEM(chunk, sizeof(VisibleChunk));
				}
			}
		}
	} else if (dc.x < 0) {
		// Copy left to right
		for (size_t x = CHUNK_DIAMETER - 1; x < CHUNK_DIAMETER; x--) {
			for(size_t z = 0; z < CHUNK_DIAMETER; z++) {
				if (x >= CHUNK_DIAMETER + dc.x || -dc.x >= CHUNK_DIAMETER) {
					// These chunks are going to be overwritten
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					if (chunk->data) gameInfo->gfxFuncs->DeleteChunk(chunk->data);
					chunk->data = nullptr;
					if (chunk->chunk) ZERO_MEM(chunk->chunk, sizeof(Chunk)); // TODO: Necessary?
				}
				if (x + dc.x < CHUNK_DIAMETER) {
					// Move chunk
					gameInfo->chunkGrid[x * CHUNK_DIAMETER + z] = gameInfo->chunkGrid[(x + dc.x) * CHUNK_DIAMETER + z];
				} else {
					// Generate new chunk here later
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					// TODO: Only unload if needed
					if (chunk->chunk) chunk->chunk->loaded = false;
					ZERO_MEM(chunk, sizeof(VisibleChunk));
				}
			}
		}
	}

	// Movement in Z-axis
	if (dc.z > 0) {
		// Copy down to up
		for(size_t z = 0; z < CHUNK_DIAMETER; z++) {
			for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
				if (z < dc.z) {
					// These chunks are going to be overwritten
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					if (chunk->data) gameInfo->gfxFuncs->DeleteChunk(chunk->data);
					chunk->data = nullptr;
					if (chunk->chunk) ZERO_MEM(chunk->chunk, sizeof(Chunk)); // TODO: Necessary?
				}
				if (z + dc.z < CHUNK_DIAMETER) {
					gameInfo->chunkGrid[x * CHUNK_DIAMETER + z] = gameInfo->chunkGrid[x * CHUNK_DIAMETER + (z + dc.z)];
				} else {
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					// TODO: Only unload if needed
					if (chunk->chunk) chunk->chunk->loaded = false;
					ZERO_MEM(chunk, sizeof(VisibleChunk));
				}
			}
		}
	} else if (dc.z < 0) {
		// Copy up to down
		for(size_t z = CHUNK_DIAMETER - 1; z < CHUNK_DIAMETER; z--) {
			for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
				if (z >= CHUNK_DIAMETER + dc.z || -dc.z >= CHUNK_DIAMETER) {
					// These chunks are going to be overwritten
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					if (chunk->data) gameInfo->gfxFuncs->DeleteChunk(chunk->data);
					chunk->data = nullptr;
					if (chunk->chunk) ZERO_MEM(chunk->chunk, sizeof(Chunk)); // TODO: Necessary?
				}
				if (z + dc.z < CHUNK_DIAMETER) {
					gameInfo->chunkGrid[x * CHUNK_DIAMETER + z] = gameInfo->chunkGrid[x * CHUNK_DIAMETER + (z + dc.z)];
				} else {
					VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
					// TODO: Only unload if needed
					if (chunk->chunk) chunk->chunk->loaded = false;
					ZERO_MEM(chunk, sizeof(VisibleChunk));
				}
			}
		}
	}

	// TODO: Chunks that need to load may already exist in the pool through chunk loaders/multiplayer

	// Pointers to the chunk pool
	Chunk* freeChunks[CHUNK_DIAMETER * CHUNK_DIAMETER] = { 0 };
	size_t numFreeChunks = 0;

	// List gaps in chunk pool
	for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
		for (size_t z = 0; z < CHUNK_DIAMETER; z++) {
			// TODO: This cannot work when we separate loaded chunks and visible chunks
			Chunk* chunk = &gameInfo->chunkPool[x * CHUNK_DIAMETER + z];
			if (!chunk->loaded) {
				assert(numFreeChunks < NUM_CHUNKS);
				freeChunks[numFreeChunks++] = chunk;
			}
		}
	}

	// Add missing chunks
	for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
		for (size_t z = 0; z < CHUNK_DIAMETER; z++) {
			VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
			if (!chunk->chunk) {
				// Missing chunk found, find space in chunkPool
				assert(numFreeChunks > 0);
				Chunk* freeChunk = freeChunks[--numFreeChunks];
				// TODO: Figure out if clearing here is better than during unloading
				ZERO_MEM(freeChunk, sizeof(Chunk));
				int32_t cx = (int32_t) (x + chunkPos.x - CHUNK_RADIUS);
				int32_t cz = (int32_t) (z + chunkPos.z - CHUNK_RADIUS);
				GenerateChunk(freeChunk, cx, cz);
				chunk->chunk = freeChunk;
				freeChunk->loaded = true;
				// TODO: This still embeds raw positional data instead of using a transform in rendering
				chunk->data = GenerateChunkMesh(gameInfo, freeChunk, cx, cz);
			}
		}
	}

	/*
	// Debug visible chunk grid
	for (size_t z = 0; z < CHUNK_DIAMETER; z++) {
		char buf[64] = { 0 };
		for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
			VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
			sprintf(buf, "%s%02zi.", buf, gameInfo->gfxFuncs->GetDataIndex(chunk->data));
		}
		logger::LogInfo(buf);
	}
	*/
}

void ResetPosition(GameInfo* gameInfo) {
	// Set chunk position of player
	Vector3 pos = s_player.GetPosition();
	s_oldPlayerChunkPosition = WorldToChunkPosition(pos.getX(), pos.getZ());

	UpdateChunkGrid(gameInfo);

	Chunk* chunk = gameInfo->chunkGrid[CHUNK_RADIUS * CHUNK_DIAMETER + CHUNK_RADIUS].chunk;

	size_t x = CHUNK_DIM_XZ / 2;
	size_t z = CHUNK_DIM_XZ / 2;
	size_t y = CHUNK_DIM_Y - 1;
	// Find first 
	while (!GetBlock(chunk, x, y, z)) {
		y--;
		if (y >= CHUNK_DIM_Y) {
			y = CHUNK_DIM_Y / 2;
			break;
		}
	}

	s_player.SetPosition(x + 0.5f, (float) y + 2.5f, z + 0.5f);
	s_player.SetRotation(Quat(0, 0, 0, 1));
}

GAME_THREAD(test);
void test(void* args) {
	
}

void Init(GameInfo* gameInfo) {
	input::Init();

	ImGui::SetCurrentContext(gameInfo->imguiState);

	// Cube
	//s_mesh = CreateCube(graphicsApi);
	
	// Timing
	s_deltaTime =  0.0f;

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

	// Chunkpool needs to be zeromem'd (set loaded flags to false)
	gameInfo->chunkPool = new Chunk[NUM_CHUNKS];
	ZERO_MEM(gameInfo->chunkPool, sizeof(Chunk) * NUM_CHUNKS);

	gameInfo->chunkGrid = new VisibleChunk[NUM_CHUNKS];
	ZERO_MEM(gameInfo->chunkGrid, sizeof(VisibleChunk) * NUM_CHUNKS);

	ResetPosition(gameInfo);

	// Background chunk loading thread
	gameInfo->CreateThread(&test, nullptr);
}

void MoveCamera(GameInfo* gameInfo) {
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
		ResetPosition(gameInfo);
		currentRotation = lastRotation = { 0, 0 };
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
	const float SPEED = 30.0f;
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
void __cdecl game::UpdateGame(GameInfo* gameInfo) {
	if(!gameInfo->initialized) {
		Init(gameInfo);
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

	MoveCamera(gameInfo);
	input::EndFrame();

	Vector3 pos = s_player.GetPosition();

	// Debug menu
	bool stay = true;
	ImGui::Begin("game::UpdateGame", &stay);
	//if (ImGui::Button("Load D3D11")) gameInfo->graphicsApi = EGraphicsApi::D3D11;
	if (ImGui::InputFloat3("Position", (float*) &pos, -1, ImGuiInputTextFlags_EnterReturnsTrue)) {
		s_player.SetPosition(pos);
	}
	Quat rot = s_player.GetRotation();
	ImGui::InputFloat4("Rotation", (float*)&rot);
	ImGui::End();

	int2 newXZ = WorldToChunkPosition(pos.getX(), pos.getZ());
	if (newXZ != s_oldPlayerChunkPosition) {
		//logger::LogInfo(std::string("new position: ") + std::to_string(newXZ.x) + std::string(", ") + std::to_string(newXZ.z));
		UpdateChunkGrid(gameInfo);
		s_oldPlayerChunkPosition = newXZ;
	}

	network::Update(gameInfo);
	//network::DrawDebugMenu(gameInfo);

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
	logger::Render();

	gameInfo->gfxFuncs->SetPlayerCameraViewMatrix(s_player.GetViewMatrix());

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
}

extern "C"
__declspec(dllexport)
void __cdecl game::DestroyGame() {
	// Free dynamic memory used by game here
}
