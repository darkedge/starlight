#pragma once
#include "starlight_graphics.h"
#include <enet/enet.h>
#include <vector>

// Assumption: Block size = 1

// TODO: Hard-coded for now
#define BUFFER_DIM_X 3
#define BUFFER_DIM_Z BUFFER_DIM_X
#define BUFFER_CHUNK_COUNT (BUFFER_DIM_X * BUFFER_DIM_Z)
#define CHUNK_DIM_X 16
#define CHUNK_DIM_Y 64
#define CHUNK_DIM_Z CHUNK_DIM_X
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

// We should be able to place items with extra data
//void PlaceBlock(Chunk* chunk, )

// And then, a chunk has a grid of SimpleBlocks
// and a dynamic list of BlockExtraData.

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
	std::vector<ComplexBlock> complexBlocks;
};

struct GameInfo {
	bool initialized;
	EGraphicsApi graphicsApi;

	ENetHost* server;
	ENetPeer* peer;
	ENetHost* client;

	// The idea is to have a ring-buffer of chunks
	// But the problem is that we might have to wait for chunks to load
	// So maybe we need a separate buffer?
	Chunk* chunks;
};

namespace game {
	void Update(GameInfo* gameInfo, graphics::API* graphicsApi);
	void Destroy();
}
