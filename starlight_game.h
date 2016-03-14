#pragma once
#include "starlight_graphics.h"
#include "starlight_memory.h"

// Assumption: Block size = 1

// TODO: Hard-coded for now
#define CHUNK_DIM_XZ 16
#define CHUNK_DIM_Y 128

// Idea: Prioritize to load the chunks that the player is looking at?

// It might be an idea to allow for blocks to have a subtype.
// Useful for different kinds of wood, leaves, or flowers, for example.
// Then you don't need to check for every subtype.

struct Chunk {
	// XZ position
	glm::tvec2<int32_t> position;
	uint16_t blocks[CHUNK_DIM_XZ * CHUNK_DIM_Y * CHUNK_DIM_XZ];
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
	glm::tvec3<uint8_t> location;
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

	// Below this line is all game state

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
