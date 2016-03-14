#pragma once
#include "starlight_graphics.h"
#include "starlight_memory.h"

// Assumption: Block size = 1

// TODO: Hard-coded for now
#define BUFFER_DIM_X 3
#define BUFFER_DIM_Z BUFFER_DIM_X
#define BUFFER_CHUNK_COUNT (BUFFER_DIM_X * BUFFER_DIM_Z)
#if 0

// Idea: Prioritize to load the chunks that the player is looking at?

// Different from Block for now
// because we should save memory
// perhaps rename Block to BlockView?
struct SimpleBlock {
	int32_t type;
};

// So there are two kinds of blocks.
// Ones store extra data, and ones that don't.
// Nomenclature?
struct BlockExtraData {

};

// Big object: do not allocate on stack
// 8B + 64kB
#endif

struct ComplexBlock {
	uint16_t type;
	// extra info
	glm::tvec3<uint8_t> location;
};

struct Chunk {
	// XZ position
	glm::tvec2<int32_t> position;
	uint16_t blocks[CHUNK_DIM_X * CHUNK_DIM_Y * CHUNK_DIM_Z];
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

};

struct NPC {

};

struct Player {

};

struct _ENetHost;
typedef _ENetHost ENetHost;
struct _ENetPeer;
typedef _ENetPeer ENetPeer;

struct GameInfo {
	bool initialized;
	EGraphicsApi graphicsApi;

	// Maybe move this to a struct
	ENetHost* server;
	ENetPeer* peer;
	ENetHost* client;

	memory::SimpleArena* allocator;

	// The idea is to have a ring-buffer of chunks
	// But the problem is that we might have to wait for chunks to load
	// So maybe we need a separate buffer?
	Chunk* chunks;
	int32_t numChunks;

	TileEntity* tileEntities;
	int32_t numTileEntities;

	NPC* npcs;
	int32_t numNPCs;

	Player* players;
	int32_t numPlayers;
};

namespace game {
	void Update(GameInfo* gameInfo, graphics::API* graphicsApi);
	void Destroy();
}
