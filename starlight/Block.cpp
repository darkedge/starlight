#include <stdint.h>

// Concept code

// This struct is not used as a base type to be inherited from.
// Instead, the contents are statically copied to objects
// that inherit its behavior. Makes for easier sorting.
// But maybe it's easier to just use inheritance...
struct BlockInfo {
	// The contents of this struct should largely be copied from
	// a database. It should have hot-swapping functionality.
	int32_t typeId;

	// Could be unsigned, 0 meaning never update
	// Maybe this does not need to be in the top Block type
	int32_t tickFrequency;

	// And then there is stuff which is used in-game only
	// Actually, this is more like a "BlockView" detail thing.
	// We don't want to store all details in the chunk.
	//glm::tvec3<int32_t> position;
};

// A bunch of info that describes the Block placing event.
// This might be hard to describe in data.
struct PlaceBlockParams {
	BlockInfo* block;
	struct Player* player;
	struct RayCastHit* raycast;
};

// This function purely takes instances of Block.
// No subclasses. It has access to the world state, for
// gameplay programming purposes.
// Although, maybe we want to call a super function
// for achievements and other callbacks?
// For example, we need to remesh the chunk
// Actually, this function should not actually place the block
// because that's a function of the chunk
void BlockPlaced(struct GameState* state, PlaceBlockParams* params) {

}
