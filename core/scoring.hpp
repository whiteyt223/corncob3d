#ifndef CC_SCORING_HPP
#define CC_SCORING_HPP
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum CCScoreStatus {
    CC_SCORE_OK=0, CC_SCORE_INVALID_ARGUMENT=1, CC_SCORE_SHORT_BUFFER=2,
    CC_SCORE_DIVIDE_BY_ZERO=3, CC_SCORE_UNSUPPORTED_STOCKADE_CLOCK=4,
    CC_SCORE_UNSUPPORTED_EXTERNAL_COMMIT=5, CC_SCORE_INVALID_THEATER_CONTEXT=6
};
enum CCScoreMode {
    CC_SCORE_FINAL=0, CC_SCORE_ISCORE_LEAF=1, CC_SCORE_ISCORE_PREVIEW=2
};
enum {
    CC_SCORE_INPUT_BYTES=130, CC_SCORE_SCORED_BYTES=134,
    CC_SCORE_FILE_BYTES=140, CC_SCORE_PILOT_BYTES=623,
    CC_SCORE_MAX_OPEN_THEATERS=24, CC_SCORE_WORKSPACE_BYTES=4096
};
// Host structs are not packed disk records. Use the serialization functions.
typedef struct CCScoreFields { uint16_t words[65]; } CCScoreFields;
typedef struct CCScoreFile {
    CCScoreFields fields;
    uint32_t checksum;
    uint16_t error_code;
    uint8_t scan_code,scan_flag;
    uint16_t hash_pointer;
} CCScoreFile;
typedef struct CCScoreBreakdown {
    int32_t score,mobf;
    uint32_t pmsf,msf,factors_valid;
} CCScoreBreakdown;
// Names must have a NUL within 28 bytes. Theater array follows pilot open order.
typedef struct CCScoreTheater {
    uint8_t name[28];
    int16_t remaining,destroyed;
    uint16_t awarded;
} CCScoreTheater;
typedef struct CCScoreDefinition { uint8_t name[28]; uint16_t eligible; } CCScoreDefinition;
typedef struct CCScoreProgress {
    uint16_t awards[10],promotion,special_promotion,theater_dirty;
} CCScoreProgress;

uint32_t cc_score_decode_result(const uint8_t* bytes,uint32_t size,CCScoreFields* fields);
uint32_t cc_score_encode_result(const CCScoreFields* fields,uint8_t* bytes,uint32_t capacity);
uint32_t cc_score_decode_file(const uint8_t* bytes,uint32_t size,CCScoreFile* file);
uint32_t cc_score_encode_file(const CCScoreFile* file,uint8_t* bytes,uint32_t capacity);
uint32_t cc_score_encode_scored(const CCScoreFields* fields,int32_t score,uint8_t* bytes,uint32_t capacity);
uint32_t cc_score_compute(const uint8_t* result,uint32_t size,uint32_t mode,CCScoreBreakdown* output);
// Exact low-level MOAG accumulator. Caller must supply an accepted sortie.
// This does not apply career killed/captured flags or the external commit gates.
// Errors leave pilot/scored/output unchanged. Optional output may be null.
// Deluxe MOAG final commit uses CC_SCORE_ISCORE_LEAF; preserve raw status.
uint32_t cc_score_accumulate_mode(uint32_t mode,uint8_t* pilot,uint32_t pilot_size,const uint8_t* result,
    uint32_t result_size,uint8_t* scored,uint32_t scored_capacity,CCScoreBreakdown* output);
uint32_t cc_score_accumulate(uint8_t* pilot,uint32_t pilot_size,const uint8_t* result,
    uint32_t result_size,uint8_t* scored,uint32_t scored_capacity,CCScoreBreakdown* output);
// Ordinary MOAG awards/rank with a fresh notification context. Requires full
// cached metadata; friendly-fire punishment returns unsupported without changes.
uint32_t cc_score_progress(uint8_t* pilot,uint32_t pilot_size,
    CCScoreTheater* theaters,uint32_t theater_count,
    const CCScoreDefinition* definitions,uint32_t definition_count,
    int32_t theater_planes_lost,CCScoreProgress* output);
// This stateless leaf API rejects external commits. The complete transaction
// is handled by web/Career.finish with the source gates and browser storage.
uint32_t cc_score_commit_sortie(void);
// Optional aligned staging area for JS/Wasm callers without an allocator.
uint8_t* cc_score_workspace(void);
uint32_t cc_score_workspace_size(void);
#ifdef __cplusplus
}
#endif
#endif

extern "C" uint32_t cc_score_progress_mode(uint32_t other_worlds,uint8_t* pilot,uint32_t size,CCScoreTheater* theaters,uint32_t count,const CCScoreDefinition* definitions,uint32_t definition_count,int32_t theater_planes_lost,CCScoreProgress* output);
