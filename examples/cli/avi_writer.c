/**
 * @file avi_writer.c
 * @brief AVI video writer — uncompressed RGB (DIB) frames with RIFF size patching
 */

#include "avi_writer.h"
#include <stdlib.h>
#include <string.h>

static uint8_t padding_zero[4] = {0, 0, 0, 0};

int create_mjpg_avi_from_rgb_frames(const char* filename,
                                    const uint8_t** frames,
                                    int num_frames,
                                    int width,
                                    int height,
                                    int fps,
                                    int quality) {
    (void)quality;  /* raw RGB — quality unused */

    if (num_frames == 0 || !frames || !filename) return -1;

    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    uint32_t frame_data_size = (uint32_t)(width * height * 3);
    uint32_t microsec_per_frame = (fps > 0) ? (1000000 / fps) : 125000;

    /* strh payload = 56 bytes, strf payload = 40 bytes */
    uint32_t strh_payload = 56;
    uint32_t strf_payload = 40;
    /* strl LIST payload = "strl"(4) + "strh"(4)+4+56 + "strf"(4)+4+40 = 116 */
    uint32_t strl_payload = 4 + (8 + strh_payload) + (8 + strf_payload);

    /* hdrl LIST payload = "hdrl"(4) + "avih"(4)+4+56 + LIST strl(8+strl_payload) */
    uint32_t hdrl_payload = 4 + (8 + 56) + (8 + strl_payload);

    /* movi LIST payload = "movi"(4) + num_frames * (8 + frame_data_size) */
    uint32_t movi_payload = 4 + (uint32_t)num_frames * (8 + frame_data_size);

    /* RIFF payload = "AVI "(4) + LIST hdrl(8+hdrl_payload) + LIST movi(8+movi_payload) */
    uint32_t riff_payload = 4 + (8 + hdrl_payload) + (8 + movi_payload);

    /* ---- RIFF header ---- */
    fwrite("RIFF", 1, 4, f);
    long riff_size_pos = ftell(f);
    write_u32_le(f, riff_payload);
    fwrite("AVI ", 1, 4, f);

    /* ---- LIST hdrl ---- */
    fwrite("LIST", 1, 4, f);
    write_u32_le(f, hdrl_payload);
    fwrite("hdrl", 1, 4, f);

    /* avih chunk */
    fwrite("avih", 1, 4, f);
    write_u32_le(f, 56);
    write_u32_le(f, microsec_per_frame);          /* MicroSecPerFrame */
    write_u32_le(f, frame_data_size * fps);       /* MaxBytesPerSec */
    write_u32_le(f, 0);                           /* PaddingGranularity */
    write_u32_le(f, 0x10);                        /* Flags: AVIF_HASINDEX */
    write_u32_le(f, (uint32_t)num_frames);        /* TotalFrames */
    write_u32_le(f, 0);                           /* InitialFrames */
    write_u32_le(f, 1);                           /* Streams */
    write_u32_le(f, frame_data_size);             /* SuggestedBufferSize */
    write_u32_le(f, (uint32_t)width);             /* Width */
    write_u32_le(f, (uint32_t)height);            /* Height */
    fwrite(padding_zero, 1, 4, f);               /* Reserved[0] */
    fwrite(padding_zero, 1, 4, f);               /* Reserved[1] */
    fwrite(padding_zero, 1, 4, f);               /* Reserved[2] */
    fwrite(padding_zero, 1, 4, f);               /* Reserved[3] */

    /* LIST strl */
    fwrite("LIST", 1, 4, f);
    write_u32_le(f, strl_payload);
    fwrite("strl", 1, 4, f);

    /* strh chunk — video stream header */
    fwrite("strh", 1, 4, f);
    write_u32_le(f, strh_payload);
    fwrite("vids", 1, 4, f);                     /* fccType */
    fwrite("DIB ", 1, 4, f);                     /* fccHandler: uncompressed DIB */
    write_u32_le(f, 0);                           /* Flags */
    write_u16_le(f, 0);                           /* Priority */
    write_u16_le(f, 0);                           /* Language */
    write_u32_le(f, 0);                           /* InitialFrames */
    write_u32_le(f, 1);                           /* Scale */
    write_u32_le(f, (uint32_t)fps);               /* Rate (fps) */
    write_u32_le(f, 0);                           /* Start */
    write_u32_le(f, (uint32_t)num_frames);        /* Length */
    write_u32_le(f, frame_data_size);             /* SuggestedBufferSize */
    write_u32_le(f, 0xFFFFFFFF);                  /* Quality */
    write_u32_le(f, 0);                           /* SampleSize */
    write_u16_le(f, 0); write_u16_le(f, 0);      /* rcFrame left, top */
    write_u16_le(f, (uint16_t)width);             /* rcFrame right */
    write_u16_le(f, (uint16_t)height);            /* rcFrame bottom */

    /* strf chunk — BITMAPINFOHEADER */
    fwrite("strf", 1, 4, f);
    write_u32_le(f, strf_payload);
    write_u32_le(f, 40);                          /* biSize */
    write_u32_le(f, (uint32_t)width);             /* biWidth */
    write_u32_le(f, (uint32_t)height);            /* biHeight (positive = bottom-up) */
    write_u16_le(f, 1);                           /* biPlanes */
    write_u16_le(f, 24);                          /* biBitCount: 24-bit RGB */
    write_u32_le(f, 0);                           /* biCompression: BI_RGB */
    write_u32_le(f, frame_data_size);             /* biSizeImage */
    write_u32_le(f, 0);                           /* biXPelsPerMeter */
    write_u32_le(f, 0);                           /* biYPelsPerMeter */
    write_u32_le(f, 0);                           /* biClrUsed */
    write_u32_le(f, 0);                           /* biClrImportant */

    /* ---- LIST movi ---- */
    fwrite("LIST", 1, 4, f);
    long movi_size_pos = ftell(f);
    write_u32_le(f, movi_payload);
    fwrite("movi", 1, 4, f);

    /* Write each frame as a "00dc" chunk (uncompressed video) */
    for (int i = 0; i < num_frames; i++) {
        fwrite("00dc", 1, 4, f);
        write_u32_le(f, frame_data_size);
        fwrite(frames[i], 1, frame_data_size, f);
    }

    /* Patch RIFF and movi sizes after writing all frames */
    long end_pos = ftell(f);
    fseek(f, riff_size_pos, SEEK_SET);
    write_u32_le(f, (uint32_t)(end_pos - 8));  /* RIFF payload = total file - 8 */
    fseek(f, movi_size_pos, SEEK_SET);
    write_u32_le(f, movi_payload);
    fseek(f, end_pos, SEEK_SET);

    fclose(f);
    return 0;
}
