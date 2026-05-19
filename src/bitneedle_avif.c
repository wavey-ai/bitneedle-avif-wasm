#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "avif/avif.h"

static uint8_t *g_output = NULL;
static size_t g_output_size = 0;
static char g_last_error[256] = "";

static void set_error(const char *message) {
    if (!message) {
        g_last_error[0] = '\0';
        return;
    }
    strncpy(g_last_error, message, sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = '\0';
}

static void clear_output(void) {
    if (g_output) {
        free(g_output);
        g_output = NULL;
    }
    g_output_size = 0;
}

void *bnavif_malloc(size_t size) {
    return malloc(size);
}

void bnavif_free(void *ptr) {
    free(ptr);
}

const char *bnavif_last_error_ptr(void) {
    return g_last_error;
}

const uint8_t *bnavif_output_ptr(void) {
    return g_output;
}

size_t bnavif_output_size(void) {
    return g_output_size;
}

int bnavif_encode_rgba(const uint8_t *rgba, int width, int height, int quantizer, int speed, int monochrome) {
    clear_output();
    set_error(NULL);

    if (!rgba) {
        set_error("missing RGBA input");
        return 0;
    }
    if (width <= 0 || height <= 0) {
        set_error("invalid image dimensions");
        return 0;
    }
    if (quantizer < AVIF_QUANTIZER_BEST_QUALITY || quantizer > AVIF_QUANTIZER_WORST_QUALITY) {
        set_error("quantizer must be in the range 0..63");
        return 0;
    }
    if (speed < AVIF_SPEED_SLOWEST || speed > AVIF_SPEED_FASTEST) {
        set_error("speed must be in the range 0..10");
        return 0;
    }

    avifPixelFormat pixel_format = monochrome ? AVIF_PIXEL_FORMAT_YUV400 : AVIF_PIXEL_FORMAT_YUV444;
    avifImage *image = avifImageCreate((uint32_t)width, (uint32_t)height, 8, pixel_format);
    if (!image) {
        set_error("failed to allocate avif image");
        return 0;
    }

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.depth = 8;
    rgb.pixels = (uint8_t *)rgba;
    rgb.rowBytes = (uint32_t)width * 4u;

    avifResult result = avifImageRGBToYUV(image, &rgb);
    if (result != AVIF_RESULT_OK) {
        set_error(avifResultToString(result));
        avifImageDestroy(image);
        return 0;
    }

    avifEncoder *encoder = avifEncoderCreate();
    if (!encoder) {
        set_error("failed to allocate avif encoder");
        avifImageDestroy(image);
        return 0;
    }

    encoder->codecChoice = AVIF_CODEC_CHOICE_AOM;
    encoder->speed = speed;
    encoder->minQuantizer = quantizer;
    encoder->maxQuantizer = quantizer;
    encoder->minQuantizerAlpha = AVIF_QUANTIZER_LOSSLESS;
    encoder->maxQuantizerAlpha = AVIF_QUANTIZER_LOSSLESS;
    encoder->timescale = 1;

    avifRWData output = AVIF_DATA_EMPTY;
    result = avifEncoderWrite(encoder, image, &output);
    avifEncoderDestroy(encoder);
    avifImageDestroy(image);

    if (result != AVIF_RESULT_OK) {
        set_error(avifResultToString(result));
        avifRWDataFree(&output);
        return 0;
    }

    g_output = (uint8_t *)malloc(output.size);
    if (!g_output) {
        set_error("failed to allocate output buffer");
        avifRWDataFree(&output);
        return 0;
    }
    memcpy(g_output, output.data, output.size);
    g_output_size = output.size;
    avifRWDataFree(&output);
    return 1;
}
