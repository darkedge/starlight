#pragma once
#include "starlight_graphics.h"
#include "starlight_memory.h"
#include "starlight_vertex_generated.h"

// Assumption: Block size = 1

// TODO: Hard-coded for now
#define CHUNK_DIM_XZ 16
#define CHUNK_DIM_Y 128

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

#include <vector>
struct TempMesh {
	// lazy
	std::vector<int32_t> indices;
	std::vector<Vertex> vertices;
};

struct _ENetHost;
typedef _ENetHost ENetHost;
struct _ENetPeer;
typedef _ENetPeer ENetPeer;

// Functions visible to game from platform

#define CALCULATE_DELTA_TIME(name) float name()
typedef CALCULATE_DELTA_TIME(CalculateDeltaTimeFunc);

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

struct GameInfo {
	bool initialized;
	EGraphicsApi graphicsApi;

	// Maybe move this to a struct
	ENetHost* server;
	ENetPeer* peer;
	ENetHost* client;

	void* imguiState;

	//memory::SimpleArena* allocator;

	CalculateDeltaTimeFunc* CalculateDeltaTime;
#if 0
	MallocFunc* Malloc;
	FreeFunc* Free;
	ReallocFunc* Realloc;
	NoMemoryFunc* NoMemory;
#endif

	// Below this line is all game state

	// The idea is to have a ring-buffer of chunks
	// But the problem is that we might have to wait for chunks to load
	// So maybe we need a separate buffer?
	Chunk* chunks;
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

#define GAME_UPDATE(name) void name(struct GameInfo *,class graphics::API *)
typedef GAME_UPDATE(UpdateGameFunc);

#define GAME_DESTROY(name) void name()
typedef GAME_DESTROY(DestroyGameFunc);

// Maybe separate struct for logger etc?

struct GameFuncs {
	DestroyLoggerFunc* DestroyLogger;
	UpdateGameFunc* UpdateGame;
	DestroyGameFunc* DestroyGame;
	bool valid;
};

// NOTE: Platform layer can't use exported functions in debug (linker errors!)
namespace game {
	extern "C"
	__declspec(dllexport)
	void __cdecl UpdateGame(GameInfo* gameInfo, graphics::API* graphicsApi);

	extern "C"
	__declspec(dllexport)
	void __cdecl DestroyGame();
}
