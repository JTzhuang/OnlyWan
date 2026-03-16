/**
 * @file avi_writer.h
 * @brief Simple AVI video writer for RGB frames
 *
 * This creates Motion JPEG (MJPG) AVI files from RGB frame data.
 */

#ifndef AVI_WRITER_H
#define AVI_WRITER_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write 32-bit little-endian integer
 */
static inline void write_u32_le(FILE* f, uint32_t val) {
    uint8_t bytes[4] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF),
        (uint8_t)((val >> 16) & 0xFF),
        (uint8_t)((val >> 24) & 0xFF)
    };
    fwrite(bytes, 1, 4, f);
}

/**
 * @brief Write 16-bit little-endian integer
 */
static inline void write_u16_le(FILE* f, uint16_t val) {
    uint8_t bytes[2] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF)
    };
    fwrite(bytes, 1, 2, f);
}

/**
 * @brief AVI index entry
 */
typedef struct {
    uint32_t offset;
    uint32_t size;
} avi_index_entry;

/**
 * @brief Create an MJPG AVI file from RGB frames
 *
 * @param filename Output AVI file name
 * @param frames Array of RGB frame data (width * height * 3 bytes per frame)
 * @param num_frames Number of frames
 * @param width Frame width
 * @param height Frame height
 * @param fps Frames per second
 * @param quality JPEG quality (0-100, default 90)
 * @return 0 on success, -1 on failure
 *
 * Note: This function requires a JPEG encoder. For now, it's a stub
 * that writes placeholder data. Full implementation needs JPEG encoding.
 */
int create_mjpg_avi_from_rgb_frames(const char* filename,
                                    const uint8_t** frames,
                                    int num_frames,
                                    int width,
                                    int height,
                                    int fps,
                                    int quality);

#ifdef __cplusplus
}
#endif

#endif /* AVI_WRITER_H */
