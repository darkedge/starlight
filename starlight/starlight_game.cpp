#include "starlight_game.h"
#include "starlight_log.h"
#include "starlight_lua.h"
#include "starlight_graphics.h"
#include <vectormath/scalar/cpp/vectormath_aos.h>
#include <cstdint>
#include <deque>
#include <condition_variable>
#include "imgui.h"
#include "Network.h"
#include "Noise.h"
#include <enet/enet.h>
#include "mj_config.h"
#include "mj_controls.h"

// temp
#include <sstream>

using namespace Vectormath::Aos;

// TODO: Move this to GameInfo or something
static int2 s_oldPlayerChunkPosition;

static int2 WorldToChunkPosition(float x, float z) {
    int2 xz;
    xz.x = (int32_t)floorf(x / CHUNK_DIM_XZ);
    xz.z = (int32_t)floorf(z / CHUNK_DIM_XZ);
    return xz;
}

static std::string s_controls[] = {
    "LookLeft",
    "LookRight",
    "MoveForward",
    "MoveBackward",
    "StrafeLeft",
    "StrafeRight",
    "Jump",
    "Crouch",
    "ReloadLua"
};

// Multiframe jobs
struct MultiFrameJobParams {
    Chunk* chunk;
    int32_t cx;
    int32_t cz;
};

#define MULTI_FRAME_FUNC(name) void name(MultiFrameJobParams*)
typedef MULTI_FRAME_FUNC(MultiFrameFunc);

// These are used in result parsing as well for confirmation purposes
struct MultiFrameJob {
    MultiFrameFunc* Run;
    // Params are a separate struct so we can do: job.Run(job.params)
    MultiFrameJobParams params;
};

// Single producer, multiple consumer
struct MultiFrameJobQueue {
    std::condition_variable cv;
    std::mutex mutex;
    std::deque<MultiFrameJob> queue;
};
static MultiFrameJobQueue s_jobQueue;

// Assumes queue is locked.
void EnqueueMultiFrameJob(MultiFrameJobQueue* queue, MultiFrameJob* job) {
    queue->queue.push_back(*job);
}

// Blocks until a job is available.
void DequeueMultiFrameJob(MultiFrameJobQueue* queue, MultiFrameJob* job) {
	std::unique_lock<std::mutex> lock(queue->mutex);
	while (!queue->queue.size()) {
		queue->cv.wait(lock); // release lock, block until notify_all, acquire lock
	}

    // not thread-safe: logger is not covered by mutex
    //logger::LogInfo(std::string("- job, ") + std::to_string(queue->numJobs));

    *job = queue->queue.front();
    queue->queue.pop_front();
}

// Multiframe results
#define PARSE_RESULT_FUNC(name) void name(GameInfo*, void*, MultiFrameJobParams* params)
typedef PARSE_RESULT_FUNC(ParseResultFunc);

// For now, the result is stored in dynamically allocated memory
// which needs to be deleted by the result parsing function.
struct MultiFrameResult {
    ParseResultFunc* Run;
    void* memory;
    MultiFrameJobParams params;
};

// Multiple producer, single consumer
struct MultiFrameResultArray {
    // This can simply be a vector because the array
    // is guaranteed to be cleared every frame
    std::vector<MultiFrameResult> array;
    // And a simple mutex is enough
    std::mutex mutex;
};
static MultiFrameResultArray s_resultArray;

void PushMultiFrameResult(MultiFrameResultArray* array, MultiFrameResult* result) {
    std::lock_guard<std::mutex> lock(array->mutex);
    array->array.push_back(*result);
}

// Assumes that the array is locked before this function is called
bool PopMultiFrameResult(MultiFrameResultArray* array, MultiFrameResult* result) {
    if (array->array.empty()) {
        return false;
    } else {
        *result = array->array.back();
        array->array.pop_back();
        return true;
    }
}

PARSE_RESULT_FUNC(UploadMeshData);
void UploadMeshData(GameInfo* gameInfo, void* memory, MultiFrameJobParams* params) {
    Vector3 pos = gameInfo->player.GetPosition();
    int2 pxz = WorldToChunkPosition(pos.getX(), pos.getZ());

    TempMesh* mesh = (TempMesh*) memory;
    
    // Check if chunk is out of sight
    int32_t x = params->cx - pxz.x;
    int32_t z = params->cz - pxz.z;
    if (abs(x) <= CHUNK_RADIUS && abs(z) <= CHUNK_RADIUS) {
        // The VisibleChunk might have moved, so recalculate destination
        x += CHUNK_RADIUS;
        z += CHUNK_RADIUS;
        VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];

        // Skip if we meshed the same chunk twice (happens if we moved fast enough)
        if (!chunk->data) {
            chunk->data = gameInfo->gfxFuncs->AddChunk(mesh);
        }
    }
    delete mesh;
}

// Currently returns by value... maybe we want to return by pointer when Block becomes a struct?
// Or use SoA and this keeps being an integer.
// TODO: Change 0 to "AIR" or something
inline Block GetBlock(Chunk* chunk, size_t x, size_t y, size_t z) {
    assert(chunk);
    //assert(x >= 0 && x < CHUNK_DIM_XZ);
    //assert(y >= 0 && y < CHUNK_DIM_Y);
    //assert(z >= 0 && z < CHUNK_DIM_XZ);
    if (x >= CHUNK_DIM_XZ) return Block(0);
    if (y >= CHUNK_DIM_Y) return Block(0);
    if (z >= CHUNK_DIM_XZ) return Block(0);
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

/*
    2--3
    |\ |
    | \|
    0--1
*/
static void AddQuad(TempMesh *mesh, float3 v0, float3 v1, float3 v2, float3 v3, size_t width, size_t height, Block type) {
    float tx = (float) type;
    Vertex vertices[] = {
        { { 0, 0, tx }, v0 },
        { { (float) width, 0, tx }, v1 },
        { { 0, (float) height, tx }, v2 },
        { { (float) width, (float) height, tx }, v3 },
    };

    // Get base vertex index
    int32_t size = (int32_t)mesh->vertices.size();

    // Add vertices
    mesh->vertices.insert(mesh->vertices.end(), vertices, vertices + 4);

    // Add indices
    int32_t indices[] = {size + 0, size + 1, size + 2, size + 2, size + 1, size + 3};
    mesh->indices.insert(mesh->indices.end(), indices, indices + 6);
}

MULTI_FRAME_FUNC(GenerateChunkMesh);
void GenerateChunkMesh(MultiFrameJobParams* params) {
    Chunk* chunk = params->chunk;
    int32_t cx = params->cx;
    int32_t cz = params->cz;

    // TODO: Do not bake chunk world position in here

    TempMesh* mesh = new TempMesh;

    // Note: This assertion guarantees that the mask is large enough
    assert(CHUNK_DIM_Y >= CHUNK_DIM_XZ);
    // 16 * 128 * 2 = 4 KB stack space, should be safe
    Block slice[CHUNK_DIM_XZ * CHUNK_DIM_Y];

    size_t xyz[] = { 0, 0, 0 }; // xyz yzx zxy
    size_t max[] = { CHUNK_DIM_XZ, CHUNK_DIM_Y, CHUNK_DIM_XZ };

    // Y->Z->X
    // For every side
    for (size_t i = 0; i < 6; i++) {
        // -X, -Y, -Z, +X, +Y, +Z
        // X: for(x, y/z, y/z)
        // Y: for(y, z, x)
        // Z: for(z, x/y, x/y)
        
        size_t d0 = (i + 0) % 3; // x y z
        size_t d1 = (i + 1) % 3; // y z x
        size_t d2 = (i + 2) % 3; // z x y

        ptrdiff_t backface = i / 3 * 2 - 1; // -1 -1 -1 +1 +1 +1

        // Traverse the chunk
        for (xyz[d0] = 0; xyz[d0] < max[d0]; xyz[d0]++) {

            // Fill in slice
            for (xyz[d1] = 0; xyz[d1] < max[d1]; xyz[d1]++) {
                for (xyz[d2] = 0; xyz[d2] < max[d2]; xyz[d2]++) {
                    Block block = GetBlock(chunk, xyz[0], xyz[1], xyz[2]);
                    if (block) {
                        // Check neighbor
                        xyz[d0] += backface;
                        if (GetBlock(chunk, xyz[0], xyz[1], xyz[2])) {
                            slice[xyz[d1] * max[d2] + xyz[d2]] = Block(0);
                        } else {
                            slice[xyz[d1] * max[d2] + xyz[d2]] = block;
                        }
                        xyz[d0] -= backface;
                    } else {
                        slice[xyz[d1] * max[d2] + xyz[d2]] = Block(0);
                    }
                }
            }

            // Mesh the slice
            for (xyz[d1] = 0; xyz[d1] < max[d1]; xyz[d1]++) {
                for (xyz[d2] = 0; xyz[d2] < max[d2]; /*xyz[d2]++*/) {
                    Block type = slice[xyz[d1] * max[d2] + xyz[d2]];

                    // Skip air (or already meshed quads)
                    if (type == Block(0)) {
                        xyz[d2]++;
                        continue;
                    }

                    size_t width = 1;

                    // Find the largest line
                    for (size_t d22 = xyz[d2] + 1; d22 < max[d2]; d22++) {
                        if (slice[xyz[d1] * max[d2] + d22] != type) break;
                        width++;
                    }

                    size_t height = 1;

                    // Find the largest rectangle
                    bool done = false;
                    for (size_t d11 = xyz[d1] + 1; d11 < max[d1]; d11++) {
                        // Find lines of the same width
                        for (size_t d22 = xyz[d2]; d22 < xyz[d2] + width; d22++) {
                            if (slice[d11 * max[d2] + d22] != type) {
                                done = true;
                                break;
                            }
                        }
                        if (done) break;
                        height++;
                    }

                    float w[] = { 0, 0, 0 };
                    w[d2] = (float) width;
                    float h[] = { 0, 0, 0 };
                    h[d1] = (float) height;

                    float3 v {cx * CHUNK_DIM_XZ + (float) xyz[0], (float) xyz[1], cz * CHUNK_DIM_XZ + (float) xyz[2]};

                    // FIXME: Awkward way to shift front faces by one block
                    if (backface > 0) {
                        float f[] = { 0, 0, 0 };
                        f[d0] += 1.0f;
                        v += float3{ f[0], f[1], f[2] };
                    }

                    Block face = type;
                    // Grass faces (TODO: Magic number)
                    if (type == 1) {
                        switch (i) {
                            case 1: // -Y
                            {
                                face = 2;
                                break;
                            }
                            case 4: // +Y
                            {
                                face = 0;
                                break;
                            }
                            default: // Sides
                            {
                                face = 3;
                                break;
                            }
                        }
                    }

                    // Now we have a quad
                    switch (i) {
                        case 0: // -X
                        {
                            AddQuad(mesh,
                                v,
                                v + float3{ w[0], w[1], w[2] },
                                v + float3{ h[0], h[1], h[2] },
                                v + float3{ w[0] + h[0], w[1] + h[1], w[2] + h[2] }, width, height, face);
                            break;
                        }
                        case 1: // -Y
                        {
                            AddQuad(mesh,
                                v,
                                v + float3{ w[0], w[1], w[2] },
                                v + float3{ h[0], h[1], h[2] },
                                v + float3{ w[0] + h[0], w[1] + h[1], w[2] + h[2] }, width, height, face);
                            break;
                        }
                        case 2: // -Z
                        {
                            AddQuad(mesh,
                                v + float3{ h[0], h[1], h[2] },
                                v,
                                v + float3{ w[0] + h[0], w[1] + h[1], w[2] + h[2] },
                                v + float3{ w[0], w[1], w[2] },
                                height, width, face);
                            break;
                        }
                        case 3: // +X
                        {
                            AddQuad(mesh,
                                v + float3{ w[0], w[1], w[2] },
                                v,
                                v + float3{ w[0] + h[0], w[1] + h[1], w[2] + h[2] },
                                v + float3{ h[0], h[1], h[2] },
                                width, height, face);
                            break;
                        }
                        case 4: // +Y
                        {
                            AddQuad(mesh,
                                v + float3{ h[0], h[1], h[2] },
                                v + float3{ w[0] + h[0], w[1] + h[1], w[2] + h[2] },
                                v,
                                v + float3{ w[0], w[1], w[2] }, width, height, face);
                            break;
                        }
                        case 5: // +Z
                        {
                            AddQuad(mesh,
                                v,
                                v + float3{ h[0], h[1], h[2] },
                                v + float3{ w[0], w[1], w[2] },
                                v + float3{ w[0] + h[0], w[1] + h[1], w[2] + h[2] },
                                height, width, face);
                            break;
                        }
                    }

                    // Zero the quad in the slice
                    for (size_t d11 = xyz[d1]; d11 < xyz[d1] + height; d11++) {
                        for (size_t d22 = xyz[d2]; d22 < xyz[d2] + width; d22++) {
                            slice[d11 * max[d2] + d22] = Block(0);
                        }
                    }

                    // Advance search position for next quad
                    xyz[d2] += width;
                }
            }
        }
    }

    //graphics
    mesh->xz = int2 {cx * CHUNK_DIM_XZ, cz * CHUNK_DIM_XZ};

    MultiFrameResult result = {};
    result.Run = &UploadMeshData;
    result.memory = mesh;
    result.params = *params;
    PushMultiFrameResult(&s_resultArray, &result);
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
            for (int32_t y = 0; y < height; y++) {
                SetBlock(chunk, 1, bx, y, bz);
            }
        }
    }
}

static inline void DeleteChunkData(GameInfo* gameInfo, VisibleChunk* chunk) {
	assert(gameInfo);
	assert(chunk);

	// Delete chunk + data
	if (chunk->data) gameInfo->gfxFuncs->DeleteChunk(chunk->data);
	chunk->data = nullptr;
	if (chunk->chunk) chunk->chunk->loaded = false;
}

void UpdateChunkGrid(GameInfo* gameInfo) {
    assert(gameInfo->chunkGrid);
    assert(gameInfo->chunkPool);

    Vector3 pos = gameInfo->player.GetPosition();
    int2 chunkPos = WorldToChunkPosition(pos.getX(), pos.getZ());

    int2 dc = chunkPos - s_oldPlayerChunkPosition;
    //logger::LogInfo(std::string("dc: ") + std::to_string(dc.x) + ", " + std::to_string(dc.z));

    // Movement in X-axis
    if (dc.x > 0) {
        // Copy right to left
        for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
            for(size_t z = 0; z < CHUNK_DIAMETER; z++) {
                VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
                if (x < dc.x) {
					DeleteChunkData(gameInfo, chunk);
                }
                if (x + dc.x < CHUNK_DIAMETER) {
                    // Move chunk
                    *chunk = gameInfo->chunkGrid[(x + dc.x) * CHUNK_DIAMETER + z];  
                } else {
                    // Generate new chunk here
                    ZERO_MEM(chunk, sizeof(VisibleChunk));
                }
            }
        }
    } else if (dc.x < 0) {
        // Copy left to right
        for (size_t x = CHUNK_DIAMETER - 1; x < CHUNK_DIAMETER; x--) {
            for(size_t z = 0; z < CHUNK_DIAMETER; z++) {
                VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
                if (x >= CHUNK_DIAMETER + dc.x || -dc.x >= CHUNK_DIAMETER) {
					DeleteChunkData(gameInfo, chunk);
                }
                if (x + dc.x < CHUNK_DIAMETER) {
                    // Move chunk
                    *chunk = gameInfo->chunkGrid[(x + dc.x) * CHUNK_DIAMETER + z];
                } else {
                    // Generate new chunk here
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
                VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
                if (z < dc.z) {
					DeleteChunkData(gameInfo, chunk);
                }
                if (z + dc.z < CHUNK_DIAMETER) {
                    // Move chunk
                    *chunk = gameInfo->chunkGrid[x * CHUNK_DIAMETER + (z + dc.z)];
                } else {
                    // Generate new chunk here
                    ZERO_MEM(chunk, sizeof(VisibleChunk));
                }
            }
        }
    } else if (dc.z < 0) {
        // Copy up to down
        for(size_t z = CHUNK_DIAMETER - 1; z < CHUNK_DIAMETER; z--) {
            for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
                VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
                if (z >= CHUNK_DIAMETER + dc.z || -dc.z >= CHUNK_DIAMETER) {
					DeleteChunkData(gameInfo, chunk);
                }
                if (z + dc.z < CHUNK_DIAMETER) {
                    // Move chunk
                    *chunk = gameInfo->chunkGrid[x * CHUNK_DIAMETER + (z + dc.z)];
                } else {
                    // Generate new chunk here
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

    //logger::LogInfo("Found " + std::to_string(numFreeChunks) + " unloaded chunks");

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
            }
        }
    }

    // Meshing step
    {
        std::lock_guard<std::mutex> lock(s_jobQueue.mutex);
        for (size_t x = 0; x < CHUNK_DIAMETER; x++) {
            for (size_t z = 0; z < CHUNK_DIAMETER; z++) {
                VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];
                if (!chunk->data) {
                    // TODO: This still embeds raw positional data instead of using a transform in rendering
                    int32_t cx = (int32_t) (x + chunkPos.x - CHUNK_RADIUS);
                    int32_t cz = (int32_t) (z + chunkPos.z - CHUNK_RADIUS);
                    //chunk->data = GenerateChunkMesh(gameInfo, chunk->chunk, cx, cz);
                    MultiFrameJob job = {};
                    job.Run = &GenerateChunkMesh;
                    job.params.cx = cx;
                    job.params.cz = cz;
                    job.params.chunk = chunk->chunk;
                    EnqueueMultiFrameJob(&s_jobQueue, &job);
                }
            }
        }
        s_jobQueue.cv.notify_all();
    }

    /*
    // Print visible chunk grid
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
    Vector3 pos = gameInfo->player.GetPosition();
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

    gameInfo->player.SetPosition(x + 0.5f, (float) y + 2.5f, z + 0.5f);
    gameInfo->player.SetRotation(Quat(0, 0, 0, 1));
}

GAME_THREAD(MultiFrameWorkerThread);
void MultiFrameWorkerThread(void* args) {
    while (true) {
        MultiFrameJob job;
        DequeueMultiFrameJob(&s_jobQueue, &job);
        job.Run(&job.params);
    }
}

void Init(GameInfo* gameInfo) {
    // Timing
    gameInfo->deltaTime = 0.0f;

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

    gameInfo->numChunks = NUM_CHUNKS;

    // Chunkpool needs to be zeromem'd (set loaded flags to false)
    gameInfo->chunkPool = new Chunk[NUM_CHUNKS];
    ZERO_MEM(gameInfo->chunkPool, sizeof(Chunk) * NUM_CHUNKS);

    gameInfo->chunkGrid = new VisibleChunk[NUM_CHUNKS];
    ZERO_MEM(gameInfo->chunkGrid, sizeof(VisibleChunk) * NUM_CHUNKS);

	// Background threads
	for (uint32_t i = 0; i < gameInfo->hardware->numLogicalThreads; i++) {
		gameInfo->CreateThread(&MultiFrameWorkerThread, nullptr);
	}

    for (const std::string& str : s_controls) {
        gameInfo->controls->AssociateKey(gameInfo->config->GetIntProperty(str, -1), str);
    }

    luaCreateVM();
    luaReload();
    SetLuaGameInfo(gameInfo);

	// This function requires the threads to be created
    ResetPosition(gameInfo);
}

#if 0
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
        if (ImGui::GetIO().KeysDown[(intptr_t)'Q'])     currentRotation.y += 2.0f * s_deltaTime;
        if (ImGui::GetIO().KeysDown[(intptr_t)'E'])     currentRotation.y -= 2.0f * s_deltaTime;
        if (ImGui::GetIO().KeysDown[(intptr_t)'Z'])     currentRotation.x += 2.0f * s_deltaTime;
        if (ImGui::GetIO().KeysDown[(intptr_t)'X'])     currentRotation.x -= 2.0f * s_deltaTime;
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
            gameInfo->player.SetRotation(Quat::rotationY(currentRotation.y) * Quat::rotationX(currentRotation.x));
            lastRotation = currentRotation;
        }
    }

    // Translation
    const float SPEED = 300.0f;
    Vector3 translation(0, 0, 0);
    if (ImGui::GetIO().KeysDown[(intptr_t)'W'])     translation += gameInfo->player.Forward();
    if (ImGui::GetIO().KeysDown[(intptr_t)'A'])     translation -= gameInfo->player.Right();
    if (ImGui::GetIO().KeysDown[(intptr_t)'S'])     translation -= gameInfo->player.Forward();
    if (ImGui::GetIO().KeysDown[(intptr_t)'D'])     translation += gameInfo->player.Right();
    // FIXME: Cross-platform
    if (ImGui::GetIO().KeysDown[VK_LCONTROL] || ImGui::GetIO().KeysDown[(intptr_t)'C'] || ImGui::GetIO().KeysDown[VK_LSHIFT]) translation -= Vector3(0, 1, 0);
    if (ImGui::GetIO().KeysDown[VK_SPACE]) translation += Vector3(0, 1, 0);
    if (lengthSqr(translation) != 0.0f)
    {
        Vector3 pos = gameInfo->player.GetPosition();
        pos += normalize(translation) * SPEED * s_deltaTime;
        gameInfo->player.SetPosition(pos);
        //printf("pos: %.1f, %.1f, %.1f\n", m_player.GetPosition().x, m_player.GetPosition().y, m_player.GetPosition().z);
    }
}
#endif

SL_EXPORT(void) game::UpdateGame(GameInfo* gameInfo) {
    if(!gameInfo->initialized) {
        Init(gameInfo);
        gameInfo->initialized = true;
    }

    // ImGui in the .dll is unset after code reload
    ImGui::SetCurrentContext(gameInfo->imguiState);

    //slSetJVMContext(gameInfo);

    // Timing
    gameInfo->deltaTime = gameInfo->CalculateDeltaTime();

    static bool showMainMenuBar = true;
    // FIXME: Cross-platform
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

    luaUpdate(gameInfo);

    Vector3 pos = gameInfo->player.GetPosition();

    // Debug menu
    bool stay = true;
    ImGui::Begin("game::UpdateGame", &stay);
    //if (ImGui::Button("Load D3D11")) gameInfo->graphicsApi = EGraphicsApi::D3D11;
    if (ImGui::InputFloat3("Position", (float*) &pos, -1, ImGuiInputTextFlags_EnterReturnsTrue)) {
        gameInfo->player.SetPosition(pos);
    }
    Quat rot = gameInfo->player.GetRotation();
    ImGui::InputFloat4("Rotation", (float*)&rot);

    // FPS bar
    static float arr[128] = { 0 };
    for (size_t i = 0; i < 127; i++) {
        arr[i] = arr[i + 1];
    }
    arr[127] = gameInfo->deltaTime * 1000.0f;
    ImGui::PlotLines("CPU Time", arr, COUNT_OF(arr), 0, NULL, 0.0f, 33.0f, ImVec2(0,80));

    {
        std::lock_guard<std::mutex> lock(s_jobQueue.mutex);
        ImGui::Text((std::string("Queued jobs: ") + std::to_string(s_jobQueue.queue.size())).c_str());
    }
    {
        std::lock_guard<std::mutex> lock(s_resultArray.mutex);
        ImGui::Text((std::string("Queued results: ") + std::to_string(s_resultArray.array.size())).c_str());
    }
    

    ImGui::End();

    if (gameInfo->controls->GetKeyDown("ReloadLua")) {
        luaReload();
    }

    // Controls
    ImGui::Begin("Controls");
    for (const std::string& str : s_controls) {
        if (ImGui::Button(str.c_str())) {
            ImGui::OpenPopup(str.c_str());
        }
        ImGui::SameLine(); ImGui::Text(std::to_string(gameInfo->controls->GetAssociatedKey(str)).c_str());
        if (ImGui::BeginPopupModal(str.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text((std::string("Press a key for ") + str).c_str());
            ImGui::Separator();

            int key = gameInfo->controls->AnyKey();
            if (key) {
                gameInfo->controls->AssociateKey(key, str);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("Cancel", ImVec2(120,0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    ImGui::End();

    int2 newXZ = WorldToChunkPosition(pos.getX(), pos.getZ());
    if (newXZ != s_oldPlayerChunkPosition) {
        //logger::LogInfo(std::string("new position: ") + std::to_string(newXZ.x) + std::string(", ") + std::to_string(newXZ.z));
        UpdateChunkGrid(gameInfo);
        s_oldPlayerChunkPosition = newXZ;
    }

    network::Update(gameInfo);
    //network::DrawDebugMenu(gameInfo);

    {
        // Remove jobs that are outdated
        std::lock_guard<std::mutex> lock(s_jobQueue.mutex);
        s_jobQueue.queue.erase(std::remove_if(s_jobQueue.queue.begin(), s_jobQueue.queue.end(), [&](MultiFrameJob& job) 
        {
            int32_t x = job.params.cx - s_oldPlayerChunkPosition.x;
            int32_t z = job.params.cz - s_oldPlayerChunkPosition.z;
			if (abs(x) <= CHUNK_RADIUS && abs(z) <= CHUNK_RADIUS) {
				// The VisibleChunk might have moved, so recalculate destination
				x += CHUNK_RADIUS;
				z += CHUNK_RADIUS;
				VisibleChunk* chunk = &gameInfo->chunkGrid[x * CHUNK_DIAMETER + z];

				// Skip if we meshed the same chunk twice (happens if we moved fast enough)
				return !!chunk->data;
			} else {
				return true;
			}
        }), s_jobQueue.queue.end());
    }
    
	{
		// Gather multiframe job results
		MultiFrameResult result;
		std::lock_guard<std::mutex> lock(s_resultArray.mutex);
		while (PopMultiFrameResult(&s_resultArray, &result)) {
			result.Run(gameInfo, result.memory, &result.params);
		}
	}

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
        ENetPacket* packet = enet_packet_create(builder.GetBufferPointer(), builder.GetSize(),  ENET_PACKET_FLAG_RELIABLE);
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

    gameInfo->gfxFuncs->SetPlayerCameraViewMatrix(gameInfo->player.GetViewMatrix());

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

SL_EXPORT(void) game::DestroyGame(GameInfo* gameInfo) {
    // Free dynamic memory used by game here
    luaDestroyVM();

    // Save controls
    for (const std::string& str : s_controls) {
        gameInfo->config->SetIntProperty(str, gameInfo->controls->GetAssociatedKey(str));
    }
}
