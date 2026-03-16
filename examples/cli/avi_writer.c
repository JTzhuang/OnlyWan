/**
 * @file avi_writer.c
 * @brief Simple AVI video writer implementation
 *
 * Minimal AVI writer for testing purposes.
 * Creates placeholder AVI files that can be extended later.
 */

#include "avi_writer.h"
#include <stdlib.h>

// Static buffer for padding
static uint8_t padding_buffer[16] = {0};

int create_mjpg_avi_from_rgb_frames(const char* filename,
                                    const uint8_t** frames,
                                    int num_frames,
                                    int width,
                                    int height,
                                    int fps,
                                    int quality) {
    (void)quality;  // Unused for now

    if (num_frames == 0 || !frames) {
        return -1;
    }

    FILE* f = fopen(filename, "wb");
    if (!f) {
        return -1;
    }

    // Write minimal AVI header for testing
    // This is a placeholder - a full implementation would encode JPEG frames

    // RIFF header
    fwrite("RIFF", 4, 1, f);
    uint32_t file_size = 0;  // Placeholder
    fwrite(&file_size, 4, 1, f);

    // AVI signature
    fwrite("AVI ", 4, 1, f);

    // LIST header for hdrl
    fwrite("LIST", 4, 1, f);
    uint32_t hdrl_size = 60;
    fwrite(&hdrl_size, 4, 1, f);
    fwrite("hdrl", 4, 1, f);

    // avih chunk
    fwrite("avih", 4, 1, f);
    uint32_t avih_size = 56;
    fwrite(&avih_size, 4, 1, f);

    // Write some basic AVI header fields
    uint32_t microsec_per_frame = 1000000 / fps;
    uint32_t max_bytes_per_sec = 0;
    uint32_t flags = 0x110;
    uint32_t total_frames = num_frames;
    uint32_t initial_frames = 0;
    uint32_t num_streams = 1;
    uint32_t suggested_buffer = width * height * 3;

    fwrite(&microsec_per_frame, 4, 1, f);
    fwrite(&max_bytes_per_sec, 4, 1, f);
    fwrite(padding_buffer, 4, 1, f);  // Padding
    fwrite(&flags, 4, 1, f);
    fwrite(&total_frames, 4, 1, f);
    fwrite(&initial_frames, 4, 1, f);
    fwrite(&num_streams, 4, 1, f);
    fwrite(&suggested_buffer, 4, 1, f);
    fwrite(&width, 4, 1, f);
    fwrite(&height, 4, 1, f);
    fwrite(padding_buffer, 16, 1, f);  // Reserved

    // LIST header for strl
    fwrite("LIST", 4, 1, f);
    uint32_t strl_size = 0;  // Placeholder
    fwrite(&strl_size, 4, 1, f);
    fwrite("strl", 4, 1, f);

    // LIST header for movi
    fwrite("LIST", 4, 1, f);
    uint32_t movi_size = 0;  // Placeholder
    fwrite(&movi_size, 4, 1, f);
    fwrite("movi", 4, 1, f);

    fclose(f);
    return 0;
}
