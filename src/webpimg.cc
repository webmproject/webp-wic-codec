/*===========================================================================*
 - Copyright 2010 Google Inc.
 -
 - This code is licensed under the same terms as WebM:
 - Software License Agreement:  http://www.webmproject.org/license/software/
 - Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
 *===========================================================================*/

/* A modified file from webpconv+leptonica package */

/*
 * Encoding/Decoding of WebP still image compression format.
 *
 * 1. WebPDecode: Takes an array of bytes (string) corresponding to the WebP
 *                encoded image and generates output in the YUV format with
 *                the color components U, V subsampled to 1/2 resolution along
 *                each dimension.
 *
 * 2. YUV420toRGBA: Converts from YUV (with color subsampling) such as produced
 *                  by the WebPDecode routine into 32 bits per pixel RGBA data
 *                  array. This data array can be directly used by the Leptonica
 *                  Pix in-memory image format.
 *
 * 3. WebPEncode: Takes a Y, U, V data buffers (with color components U and V
 *                subsampled to 1/2 resolution) and generates the WebP string
 *
 * 4. RGBAToYUV420: Generates Y, U, V data (with color subsampling) from 32 bits
 *                  per pixel RGBA data buffer. The resulting YUV data can be
 *                  directly fed into the WebPEncode routine.
 *
 * 5. AdjustColorspace:
 *
 * 6. AdjustColorspaceBack:
 */

#include "webpimg.h"

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;

/*---------------------------------------------------------------------*
 *                              color conversions                      *
 *---------------------------------------------------------------------*/

static inline int clip(float v, int a, int b) {
  return (v > b) ? b : (v < 0) ? 0 : (int)(v);
}
enum {
    COLOR_RED = 0,
    COLOR_GREEN = 1,
    COLOR_BLUE = 2,
    ALPHA_CHANNEL = 3
};

/* endian neutral extractions of RGBA from a 32 bit pixel */
static const uint32  RED_SHIFT =
       8 * (sizeof(uint32) - 1 - COLOR_RED);           /* 24 */
static const uint32  GREEN_SHIFT =
       8 * (sizeof(uint32) - 1 - COLOR_GREEN);         /* 16 */
static const uint32  BLUE_SHIFT =
       8 * (sizeof(uint32) - 1 - COLOR_BLUE);          /*  8 */
static const uint32  ALPHA_SHIFT =
       8 * (sizeof(uint32) - 1 - ALPHA_CHANNEL);       /*  0 */

static inline int GetRed(const uint32* rgba) {
  return (int)((*rgba >> RED_SHIFT) & 0xff);
}

static inline int GetGreen(const uint32* rgba) {
  return (int)((*rgba >> GREEN_SHIFT) & 0xff);
}

static inline int GetBlue(const uint32* rgba) {
  return (int)((*rgba >> BLUE_SHIFT) & 0xff);
}

enum { YUV_FRAC = 16 };

static inline int clip_uv(int v) {
   v = (v + (257 << (YUV_FRAC + 2 - 1))) >> (YUV_FRAC + 2);
   return ((v & ~0xff) == 0) ? v : v < 0 ? 0u : 255u;
}


/* YUV <-----> RGB conversions */
/* The exact naming is Y'CbCr, following the ITU-R BT.601 standard.
 * More information at: http://en.wikipedia.org/wiki/YCbCr
 */
static inline int GetLumaY(int r, int g, int b) {
  const int kRound = (1 << (YUV_FRAC - 1)) + (16 << YUV_FRAC);
  // Y = 0.2569 * R + 0.5044 * G + 0.0979 * B + 16
  const int luma = 16839 * r + 33059 * g + 6420 * b;
  return (luma + kRound) >> YUV_FRAC;
}

static inline int GetLumaYfromPtr(uint32* rgba) {
  const int r = GetRed(rgba);
  const int g = GetGreen(rgba);
  const int b = GetBlue(rgba);
  return GetLumaY(r, g, b);
}

static inline int GetChromaU(int r, int g, int b) {
  // U = -0.1483 * R - 0.2911 * G + 0.4394 * B + 128
  return clip_uv(-9719 * r - 19081 * g + 28800 * b);
}

static inline int GetChromaV(int r, int g, int b) {
  // V = 0.4394 * R - 0.3679 * G - 0.0715 * B + 128
  return clip_uv(+28800 * r - 24116 * g - 4684 * b);
}

/* Converts YUV to RGB and writes into a 32 bit pixel in endian
 * neutral fashion
 */
enum { RGB_FRAC = 16, RGB_HALF = (1 << RGB_FRAC) / 2,
       RGB_RANGE_MIN = -227, RGB_RANGE_MAX = 256 + 226 };

static int init_done = 0;
static int16_t kVToR[256], kUToB[256];
static int32_t kVToG[256], kUToG[256];
static uint8_t kClip[RGB_RANGE_MAX - RGB_RANGE_MIN];

void WebpInitTables() {
  int i;
  for (i = 0; i < 256; ++i) {
    kVToR[i] = (89858 * (i - 128) + RGB_HALF) >> RGB_FRAC;
    kUToG[i] = -22014 * (i - 128) + RGB_HALF;
    kVToG[i] = -45773 * (i - 128);
    kUToB[i] = (113618 * (i - 128) + RGB_HALF) >> RGB_FRAC;
  }
  for (i = RGB_RANGE_MIN; i < RGB_RANGE_MAX; ++i) {
    const int j = ((i - 16) * 76283 + RGB_HALF) >> RGB_FRAC;
    kClip[i - RGB_RANGE_MIN] = (j < 0) ? 0 : (j > 255) ? 255 : j;
  }

  init_done = 1;
}

void WebpToRGB(int y, int u, int v, uint32* const dst) {
  const int r_off = kVToR[v];
  const int g_off = (kVToG[v] + kUToG[u]) >> RGB_FRAC;
  const int b_off = kUToB[u];
  const int r = kClip[y + r_off - RGB_RANGE_MIN];
  const int g = kClip[y + g_off - RGB_RANGE_MIN];
  const int b = kClip[y + b_off - RGB_RANGE_MIN];
  *dst = (r << RED_SHIFT) | (g << GREEN_SHIFT) | (b << BLUE_SHIFT);
}

void WebpToRGB24(int y, int u, int v, uint8* const dst) {
  const int r_off = kVToR[v];
  const int g_off = (kVToG[v] + kUToG[u]) >> RGB_FRAC;
  const int b_off = kUToB[u];
  dst[0] = kClip[y + b_off - RGB_RANGE_MIN];
  dst[1] = kClip[y + g_off - RGB_RANGE_MIN];
  dst[2] = kClip[y + r_off - RGB_RANGE_MIN];
//  *dst = (r << RED_SHIFT) | (g << GREEN_SHIFT) | (b << BLUE_SHIFT);
}
