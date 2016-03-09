#include "starlight_glm.h"
// All the stuff needed for networking.

// We will create one of these every network tick.
// These should be tailored per client to prevent cheating.
// Keyword: Potential visible set
struct Snapshot {

};

// TODO (Client): Keep a ringbuffer of Snapshots (100 ms).


// Client -> Server.
// Sent every client frame.
struct Command {
	int32_t timestamp;
	// player euler view angles in fixed point
	//glm::tvec<int32_t> orientation;
	uint32_t inputFlags;
	uint32_t heldItem;
	// TODO: desired movement vector
};
