/*===========================================================================*
 - Copyright 2010 Google Inc.
 -
 - This code is licensed under the same terms as WebM:
 - Software License Agreement:  http://www.webmproject.org/license/software/
 - Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
 *===========================================================================*/

/* A modified version of the file from the webp+leptonica package*/

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

#ifndef THIRD_PARTY_VP8_VP8IMG_H_
#define THIRD_PARTY_VP8_VP8IMG_H_

typedef unsigned char uint8;
typedef unsigned int uint32;

void WebpInitTables();
void WebpToRGB(int y, int u, int v, uint32* const dst);
void WebpToRGB24(int y, int u, int v, uint8* const dst);

#endif  /* THIRD_PARTY_VP8_VP8IMG_H_ */
