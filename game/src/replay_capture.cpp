#include "replay_capture.h"

#if defined(PLATFORMER_RECORD)

#include "bn_sram.h"

#include "player.h"
#include "replay.h"

namespace replay_capture {

namespace {

// The SRAM block of one level (layout in replay.h).
struct Block {
    uint8_t magic = replay::MAGIC;
    uint8_t flags = 0;
    uint16_t run_count = 0;
    uint16_t runs[replay::MAX_BLOCK_RUNS] = {};
};
static_assert(sizeof(Block) <= replay::BLOCK_SIZE);

Block block;
replay::Recorder recorder;
int recording_level = -1;

struct Sink {
    void operator()(const replay::Run& run) {
        if (block.run_count < replay::MAX_BLOCK_RUNS) {
            block.runs[block.run_count++] = replay::pack(run);
        } else {
            block.flags |= replay::FLAG_TRUNCATED;
        }
    }
};

void record(InputState& input) {
    Sink sink;
    recorder.step(input, sink);
}

void start_over() {
    recorder.clear();
    block = Block();
}

}  // namespace

void level_started(Player& player, unsigned level_index) {
    recording_level = int(level_index);
    start_over();
    player.set_input_hook(record);
}

void level_restarted() {
    start_over();
}

void level_finished() {
    if (recording_level < 0 || recording_level >= replay::LEVEL_BLOCKS) {
        return;
    }
    Sink sink;
    recorder.flush(sink);
    bn::sram::write_offset(block, replay::block_offset(recording_level));
    recording_level = -1;
}

}  // namespace replay_capture

#endif
