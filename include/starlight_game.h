#pragma once
#include "starlight_graphics.h"
#include "starlight_vertex_generated.h"
#include "mj_controls.h"

// Assumption: Block size = 1

// TODO: Hard-coded for now
#define CHUNK_DIM_XZ 16
#define CHUNK_DIM_Y 128
/*
Name 					Blocks 	Chunks	Sections
32 chunks ("Extreme") 	512 	4225 	67,600
16 chunks ("Far") 		256 	1089 	17,424
12 chunks (default) 	192 	625 	10,000
8 chunks ("Normal") 	128 	289 	4624
4 chunks ("Short") 		64 		81 		1296
2 chunks ("Tiny") 		32 		25 		400
*/
#define CHUNK_RADIUS 12
#define CHUNK_DIAMETER (CHUNK_RADIUS + 1 + CHUNK_RADIUS)
#define NUM_CHUNKS (CHUNK_DIAMETER * CHUNK_DIAMETER)

enum ESide {
	West,
	Bottom,
	North,
	East,
	Top,
	South,
	Count,
};

// This makes it easier to change later
typedef uint16_t Block;

// Idea: Prioritize to load the chunks that the player is looking at?

// It might be an idea to allow for blocks to have a subtype.
// Useful for different kinds of wood, leaves, or flowers, for example.
// Then you don't need to check for every subtype.

struct Chunk {
	// Need this bool because position is always a valid value
	bool loaded;
	// XZ position
	int2 position;
	Block blocks[CHUNK_DIM_XZ * CHUNK_DIM_Y * CHUNK_DIM_XZ];
	//std::vector<ComplexBlock> complexBlocks;
	// Minecraft has 4 bits extra per block, stored as a separate array.
	// uint8_t metadata[CHUNK_DIM_Y * CHUNK_DIM_XZ * CHUNK_DIM_XZ >> 2];

	// Do we want to store Tile Entities here?
	// They are not part of the meshing.
	// We can manage them separately.
};

// Do we want to store Tile Entities per type?
// It's very likely that we'll get a ton of unique types.
// So for now, we can keep it in one big list.
// How do we handle the different size requirements?

// The user should be able to download any combination of mods
// The added blocks and items should not have conflicting IDs
// This should be resolved internally by the game.

struct TileEntity {
	uint16_t type;
	// extra info
	byte3 location;
};

struct NPC {

};

struct Player {

};

struct VisibleChunk {
	Chunk* chunk;
	void* data;
};

#include <vector>
struct TempMesh {
	// lazy
	int2 xz;
	std::vector<int32_t> indices;
	std::vector<Vertex> vertices;
};

struct _ENetHost;
typedef _ENetHost ENetHost;
struct _ENetPeer;
typedef _ENetPeer ENetPeer;

#define GAME_THREAD(name) void name(void*)
typedef GAME_THREAD(GameThread);

// Functions visible to game from platform

#define CALCULATE_DELTA_TIME(name) float name()
typedef CALCULATE_DELTA_TIME(CalculateDeltaTimeFunc);

#define CREATE_THREAD(name) void* name(GameThread*, void*)
typedef CREATE_THREAD(CreateThreadFunc);

#if 0
#define MALLOC_FUNC(name) void* MEM_CALL name(std::size_t)
typedef MALLOC_FUNC(MallocFunc);

#define FREE_FUNC(name) void MEM_CALL name(void*)
typedef FREE_FUNC(FreeFunc);

#define REALLOC_FUNC(name) void* MEM_CALL name(void*, std::size_t)
typedef REALLOC_FUNC(ReallocFunc);

#define NO_MEMORY_FUNC(name) void MEM_CALL name()
typedef NO_MEMORY_FUNC(NoMemoryFunc);
#endif

struct ImGuiContext;

struct HardwareInfo {
	uint32_t numLogicalThreads;
};

struct GameInfo {
	bool initialized;
	EGraphicsApi graphicsApi;
	graphics::API *gfxFuncs;

	// Maybe move this to a struct
	ENetHost* server;
	ENetPeer* peer;
	ENetHost* client;

	ImGuiContext* imguiState;

	//memory::SimpleArena* allocator;

	CalculateDeltaTimeFunc* CalculateDeltaTime;
	CreateThreadFunc* CreateThread;

	HardwareInfo *hardware;

#if 0
	MallocFunc* Malloc;
	FreeFunc* Free;
	ReallocFunc* Realloc;
	NoMemoryFunc* NoMemory;
#endif
	MJControls* controls;

	// Below this line is all game state

	// A grid of pointers to chunks in chunkPool
	VisibleChunk* chunkGrid;
	// Unordered chunk data
	Chunk* chunkPool;

	int32_t numChunks;

	//TileEntity* tileEntities;
	//int32_t numTileEntities;

	//NPC* npcs;
	//int32_t numNPCs;

	//Player* players;
	//int32_t numPlayers;

	void* renderData;
};

// Functions visible to platform from game
#define DESTROY_LOGGER(name) void name()
typedef DESTROY_LOGGER(DestroyLoggerFunc);

#define GAME_UPDATE(name) void name(struct GameInfo *)
typedef GAME_UPDATE(UpdateGameFunc);

#define GAME_DESTROY(name) void name()
typedef GAME_DESTROY(DestroyGameFunc);

// NOTE: Platform layer can't use exported functions in debug (linker errors!)
namespace game {
	SL_EXPORT(void) UpdateGame(GameInfo* gameInfo);

	SL_EXPORT(void) DestroyGame();
}
