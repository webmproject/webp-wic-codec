#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#define COBJMACROS
#define STANDALONE
#define inline

#include <shlwapi.h>
#include <windows.h>
#include <wincodec.h>
#include "wine/test.h"
#include "uuid.h"

#define expect_eq_x(expected, actual) \
  do { \
   int value = (actual); \
   ok((expected) == value, "Expected " #actual " to be 0x%x (" #expected ")  is 0x%x\n", \
       (expected), value); \
  } while (0)
#define expect_eq_p(expected, actual) \
  do { \
  void *value = (actual); \
  ok((expected) == value, "Expected " #actual " to be %p (" #expected ") is %p\n", \
      (expected), value); \
  } while (0)
#define expect_eq_ws(expected, actual) \
  do { \
    LPCWSTR value = (actual); \
    ok(winetest_strcmpW((expected), value) == 0, "Expected " #actual " to be %s (" #expected ") is %s\n", \
        wine_dbgstr_w(expected), wine_dbgstr_w(value)); \
  } while (0)
#define expect_true(actual) \
  do { \
    BOOL value = (actual); \
    ok(value, "Expected " #actual " to be true, but it isn't\n"); \
  } while (0)

LPWSTR guid_to_string(GUID guid) {
  static WCHAR buffer[16][80];
  static int current_buffer = 0;
  current_buffer %= 16;
  StringFromGUID2(&guid, buffer[current_buffer],
      sizeof(buffer[0])/sizeof(buffer[0][0]));
  return buffer[current_buffer++];
}

#define CODEC_JPEG 0
#define CODEC_WEBP 1
#define CODEC_BMP 2
#define CODEC_COUNT 3

BYTE invalid_file[] = "dacnasdipouaiosduvbasioudvbioasdubvioasudfo";

// A valid JPEG HEADER without thumbnail
#define JPEG_HEADER \
    "\xff\xd8"  /* start of information */ \
    "\xff\xe0"  /* APP0 segment */ \
    "\x00\x10"  /* length: 16 bytes */ \
    "JFIF\x00"  /* magic */ \
    "\x01\x02"   /* specification 1.02 */ \
    "\x00"  /* no dpi, only aspect ratio */ \
    "\x00\x64"  /* horizontal weight */ \
    "\x00\x64"  /* vertical weight */ \
    "\x00\x00"  /* 0x0 thumbnail */ \

// A valid JPEG file.
char valid_jpeg[1024] =
    "\xFF\xD8\xFF\xE0\x00\x10\x4A\x46\x49\x46\x00\x01\x01\x01\x00\x60\x00\x60"
    "\x00\x00\xFF\xDB\x00\x43\x00\x02\x01\x01\x02\x01\x01\x02\x02\x02\x02\x02"
    "\x02\x02\x02\x03\x05\x03\x03\x03\x03\x03\x06\x04\x04\x03\x05\x07\x06\x07"
    "\x07\x07\x06\x07\x07\x08\x09\x0B\x09\x08\x08\x0A\x08\x07\x07\x0A\x0D\x0A"
    "\x0A\x0B\x0C\x0C\x0C\x0C\x07\x09\x0E\x0F\x0D\x0C\x0E\x0B\x0C\x0C\x0C\xFF"
    "\xDB\x00\x43\x01\x02\x02\x02\x03\x03\x03\x06\x03\x03\x06\x0C\x08\x07\x08"
    "\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C"
    "\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C"
    "\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\xFF\xC0\x00\x11"
    "\x08\x00\x10\x00\x10\x03\x01\x22\x00\x02\x11\x01\x03\x11\x01\xFF\xC4\x00"
    "\x1F\x00\x00\x01\x05\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\xFF\xC4\x00\xB5\x10\x00"
    "\x02\x01\x03\x03\x02\x04\x03\x05\x05\x04\x04\x00\x00\x01\x7D\x01\x02\x03"
    "\x00\x04\x11\x05\x12\x21\x31\x41\x06\x13\x51\x61\x07\x22\x71\x14\x32\x81"
    "\x91\xA1\x08\x23\x42\xB1\xC1\x15\x52\xD1\xF0\x24\x33\x62\x72\x82\x09\x0A"
    "\x16\x17\x18\x19\x1A\x25\x26\x27\x28\x29\x2A\x34\x35\x36\x37\x38\x39\x3A"
    "\x43\x44\x45\x46\x47\x48\x49\x4A\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64"
    "\x65\x66\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A\x83\x84\x85\x86"
    "\x87\x88\x89\x8A\x92\x93\x94\x95\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6"
    "\xA7\xA8\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xC2\xC3\xC4\xC5\xC6"
    "\xC7\xC8\xC9\xCA\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xE1\xE2\xE3\xE4\xE5"
    "\xE6\xE7\xE8\xE9\xEA\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFF\xC4\x00"
    "\x1F\x01\x00\x03\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00"
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\xFF\xC4\x00\xB5\x11\x00"
    "\x02\x01\x02\x04\x04\x03\x04\x07\x05\x04\x04\x00\x01\x02\x77\x00\x01\x02"
    "\x03\x11\x04\x05\x21\x31\x06\x12\x41\x51\x07\x61\x71\x13\x22\x32\x81\x08"
    "\x14\x42\x91\xA1\xB1\xC1\x09\x23\x33\x52\xF0\x15\x62\x72\xD1\x0A\x16\x24"
    "\x34\xE1\x25\xF1\x17\x18\x19\x1A\x26\x27\x28\x29\x2A\x35\x36\x37\x38\x39"
    "\x3A\x43\x44\x45\x46\x47\x48\x49\x4A\x53\x54\x55\x56\x57\x58\x59\x5A\x63"
    "\x64\x65\x66\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A\x82\x83\x84"
    "\x85\x86\x87\x88\x89\x8A\x92\x93\x94\x95\x96\x97\x98\x99\x9A\xA2\xA3\xA4"
    "\xA5\xA6\xA7\xA8\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xC2\xC3\xC4"
    "\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xE2\xE3\xE4"
    "\xE5\xE6\xE7\xE8\xE9\xEA\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFF\xDA\x00"
    "\x0C\x03\x01\x00\x02\x11\x03\x11\x00\x3F\x00\xCB\xAF\x9F\xEB\xF7\xF2\xBF"
    "\x1F\xEB\xF0\x9F\xA2\xE5\x4F\xF8\x8D\x1F\xDA\x7F\xF3\x03\xF5\x1F\x63\xFF"
    "\x00\x4F\xB9\xFD\xB7\xB5\xFF\x00\xAF\x5C\xBC\xBE\xCB\xFB\xD7\xE6\xE9\x6D"
    "\x7C\xBE\x0E\xF1\x73\xFE\x25\x9B\xDB\x7F\xB2\x7F\x6A\x7F\x6A\x72\xFF\x00"
    "\xCB\xCF\xAB\x7B\x2F\xAB\x73\x7F\x73\x11\xCF\xCF\xF5\x8F\xEE\x72\xF2\x7D"
    "\xAE\x6F\x77\xFF\xD9";

// A valid WebP file:
char valid_webp[1024] =
    "\x52\x49\x46\x46\x4A\x00\x00\x00\x57\x45\x42\x50\x56\x50\x38\x20\x3E\x00"
    "\x00\x00\x30\x02\x00\x9D\x01\x2A\x10\x00\x10\x00\x03\x07\x08\x85\x85\x88"
    "\x85\x84\x88\xA0\x02\x02\x74\xF4\x6F\xC0\xC0\x17\x00\x00\x01\x00\x00\x01"
    "\x00\x00\xFD\xFF\xFE\xDF\xFD\xD7\xFF\xFD\x83\x0A\x31\x61\xFF\xCF\x8F\x1F"
    "\xF7\x47\xFA\xDB\x26\xD8\xE0\x00\x00\x00";

// A valid BMP file:
char valid_bmp[1024] = 
    "\x42\x4D\xF6\x00\x00\x00\x00\x00\x00\x00\x76\x00\x00\x00\x28\x00\x00\x00"
    "\x10\x00\x00\x00\x10\x00\x00\x00\x01\x00\x04\x00\x00\x00\x00\x00\x80\x00"
    "\x00\x00\xC4\x0E\x00\x00\xC4\x0E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x80\x00\x00\x80\x00\x00\x00\x80\x80\x00\x80\x00"
    "\x00\x00\x80\x00\x80\x00\x80\x80\x00\x00\x80\x80\x80\x00\xC0\xC0\xC0\x00"
    "\x00\x00\xFF\x00\x00\xFF\x00\x00\x00\xFF\xFF\x00\xFF\x00\x00\x00\xFF\x00"
    "\xFF\x00\xFF\xFF\x00\x00\xFF\xFF\xFF\x00\xBB\xBB\xBB\xBB\xEE\xEE\xEE\xEE"
    "\xBB\xBB\xBB\xBB\xEE\xEE\xEE\xEE\xBB\xBB\xBB\xBB\xEE\xEE\xEE\xEE\xBB\xBB"
    "\xBB\xBB\xEE\xEE\xEE\xEE\xBB\xBB\xBB\xBB\xEE\xEE\xEE\xEE\xBB\xBB\xBB\xBB"
    "\xEE\xEE\xEE\xEE\xBB\xBB\xBB\xBB\xEE\xEE\xEE\xEE\xBB\xBB\xBB\xBB\xEE\xEE"
    "\xEE\xEE\x66\x66\x66\x66\x99\x99\x99\x99\x66\x66\x66\x66\x99\x99\x99\x99"
    "\x66\x66\x66\x66\x99\x99\x99\x99\x66\x66\x66\x66\x99\x99\x99\x99\x66\x66"
    "\x66\x66\x99\x99\x99\x99\x66\x66\x66\x66\x99\x99\x99\x99\x66\x66\x66\x66"
    "\x99\x99\x99\x99\x66\x66\x66\x66\x99\x99\x99\x99";

BYTE* codec_valid_file[CODEC_COUNT] = {
  valid_jpeg,
  valid_webp,
  valid_bmp
};

BYTE codec_good_header_bad_data[CODEC_COUNT][1024] = {
  JPEG_HEADER "\xff\xdb\x19" "as;ldkjf;alskdj f;lask dj" "\xfd\xd9",
  "RIFF\x00\x01\x00\x00WEBPVP8 \xf4\x00\x00\x00xfa;sdkjf;laskdjf;laskdjf;laskdjfaiushviou",
  ""  // Not used.
};

const GUID *codec_decoder_clsid[CODEC_COUNT] = {
  &CLSID_WICJpegDecoder,
  &CLSID_WebpWICDecoder,
  &CLSID_WICBmpDecoder
};
const GUID *codec_container_guid[CODEC_COUNT] = {
  &GUID_ContainerFormatJpeg,
  &GUID_ContainerFormatWebp,
  &GUID_ContainerFormatBmp
};

#define webp_different(webp_result, jpeg_result) (codec_id == CODEC_WEBP ? (webp_result) : (jpeg_result))

void test_uninitialized(int codec_id) {
  IWICMetadataQueryReader *metadata_reader = NULL;
  IWICBitmapDecoderInfo *decoder_info = NULL;
  IWICBitmapFrameDecode *frame = NULL;
  IWICBitmapSource *bitmap_source = NULL;
  IWICBitmapDecoder *decoder = NULL;
  IWICColorContext *contexts[4] = {NULL, NULL, NULL, NULL};
  WICRect rect = {0, 0, 16, 16};
  UINT tmp_uint, tmp_uint2;
  double tmp_double, tmp_double2;
  GUID guid;
  INT tmp_int;

  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(S_OK, IWICBitmapDecoder_GetContainerFormat(decoder, &guid));
  expect_eq_x(E_INVALIDARG, IWICBitmapDecoder_GetContainerFormat(decoder, NULL));
  expect_eq_ws(guid_to_string(*codec_container_guid[codec_id]), guid_to_string(guid));

  // Note: at least on Win7, before the first IWICImagingFactory is constructed,
  // Jpeg's GetDecoderInfo fails. We don't replicate this behavior.
  expect_eq_x(webp_different(S_OK, WINCODEC_ERR_COMPONENTNOTFOUND),
              IWICBitmapDecoder_GetDecoderInfo(decoder, &decoder_info));
  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetMetadataQueryReader(decoder, &metadata_reader));

  expect_eq_x(WINCODEC_ERR_PALETTEUNAVAILABLE, IWICBitmapDecoder_CopyPalette(decoder, (IWICPalette*)0xdeadbeef));
  expect_eq_x(WINCODEC_ERR_PALETTEUNAVAILABLE, IWICBitmapDecoder_CopyPalette(decoder, NULL));
  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetColorContexts(decoder, 4, contexts, &tmp_int));
  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetPreview(decoder, &bitmap_source));
  expect_eq_x(WINCODEC_ERR_CODECNOTHUMBNAIL, IWICBitmapDecoder_GetThumbnail(decoder, &bitmap_source));

  expect_eq_x(S_OK, IWICBitmapDecoder_GetFrameCount(decoder, &tmp_uint));
  expect_eq_x(1, tmp_uint);
  expect_eq_x(WINCODEC_ERR_FRAMEMISSING, IWICBitmapDecoder_GetFrame(decoder, 17, &frame));
  expect_eq_x(S_OK, IWICBitmapDecoder_GetFrame(decoder, 0, &frame));

  expect_eq_x(WINCODEC_ERR_WRONGSTATE, IWICBitmapFrameDecode_GetResolution(frame, &tmp_double, &tmp_double2));
  expect_eq_x(WINCODEC_ERR_WRONGSTATE, IWICBitmapFrameDecode_CopyPalette(frame, (IWICPalette*)0xdeadbeef));
  expect_eq_x(WINCODEC_ERR_CODECNOTHUMBNAIL, IWICBitmapFrameDecode_GetThumbnail(frame, &bitmap_source));
  expect_eq_x(WINCODEC_ERR_BADIMAGE, IWICBitmapFrameDecode_GetColorContexts(frame, 4, contexts, &tmp_int));
  // Should have probably returned WINCODEC_ERR_WRONGSTATE:
  tmp_uint = 0x12345;
  tmp_uint2 = 0x23456;
  expect_eq_x(webp_different(WINCODEC_ERR_WRONGSTATE, S_OK), IWICBitmapFrameDecode_GetSize(frame, &tmp_uint, &tmp_uint2));
  expect_eq_x(0x12345, tmp_uint);
  expect_eq_x(0x23456, tmp_uint2);
  guid = CLSID_WICBmpEncoder;
  expect_eq_x(webp_different(WINCODEC_ERR_WRONGSTATE, S_OK), IWICBitmapFrameDecode_GetPixelFormat(frame, &guid));
  expect_eq_ws(guid_to_string(CLSID_WICBmpEncoder), guid_to_string(guid));

  expect_eq_x(E_INVALIDARG, IWICBitmapFrameDecode_CopyPixels(frame, &rect, 1024, 1048576, (BYTE*)0xdeadbeef));

  // Works:
  expect_eq_x(webp_different(WINCODEC_ERR_UNSUPPORTEDOPERATION, S_OK), IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &metadata_reader));

  if (metadata_reader)
    IWICMetadataQueryReader_Release(metadata_reader);
  IWICComponentInfo_Release(frame);
  IWICBitmapDecoder_Release(decoder);
}

void test_invalid(int codec_id) {
  IWICBitmapFrameDecode *frame = NULL;
  IWICBitmapDecoder *decoder = NULL;
  IWICColorContext *contexts[4] = {NULL, NULL, NULL, NULL};
  IStream *stream;
  BYTE damaged_file[1024];

  stream = SHCreateMemStream(invalid_file, sizeof(invalid_file));
  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(WINCODEC_ERR_BADHEADER, IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);
  IWICBitmapDecoder_Release(decoder);

  stream = SHCreateMemStream(codec_good_header_bad_data[codec_id],
                             sizeof(codec_good_header_bad_data[codec_id]));
  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(webp_different(WINCODEC_ERR_BADIMAGE, WINCODEC_ERR_BADHEADER),
              IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);
  IWICBitmapDecoder_Release(decoder);

  memcpy(damaged_file, codec_valid_file[codec_id], sizeof(damaged_file));
  if (codec_id == CODEC_JPEG)
    damaged_file[0xb8] = 'a';  // Damage a Huffman table definition.
  else
    damaged_file[0x2f] = 'a';  // Damage the VP8 bitstream.
  stream = SHCreateMemStream(damaged_file,
                             sizeof(damaged_file));
  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(webp_different(WINCODEC_ERR_BADIMAGE, WINCODEC_ERR_BADHEADER),
              IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);
  IWICBitmapDecoder_Release(decoder);

  // Intialize with valid data...
  stream = SHCreateMemStream(codec_valid_file[codec_id], 1024);
  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(S_OK, IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);
  // ... and try to initialize the same object again.
  stream = SHCreateMemStream(codec_valid_file[codec_id], 1024);
  expect_eq_x(WINCODEC_ERR_WRONGSTATE, IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);

  IWICBitmapDecoder_Release(decoder);

}

void test_initialized(int codec_id) {
  IWICMetadataQueryReader *metadata_reader = NULL;
  IWICBitmapDecoderInfo *decoder_info = NULL;
  IWICBitmapFrameDecode *frame = NULL;
  IWICBitmapSource *bitmap_source = NULL;
  IWICBitmapDecoder *decoder = NULL;
  IWICColorContext *contexts[4] = {NULL, NULL, NULL, NULL};
  IStream *stream;
  WICRect rect = {5, 5, 4, 4};
  double tmp_double, tmp_double2;
  UINT tmp_uint, tmp_uint2;
  BYTE buf[20 * 128];
  BYTE buf2[20 * 128];
  GUID guid;
  INT tmp_int;
  int i, j;

  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(S_OK, IWICBitmapDecoder_GetContainerFormat(decoder, &guid));
  expect_eq_ws(guid_to_string(*codec_container_guid[codec_id]), guid_to_string(guid));

  // Intialize with valid data...
  stream = SHCreateMemStream(codec_valid_file[codec_id], 1024);
  expect_eq_x(S_OK, IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);

  // All decoder-level methods work the same as in uninitialized test:
  expect_eq_x(webp_different(S_OK, WINCODEC_ERR_COMPONENTNOTFOUND),
              IWICBitmapDecoder_GetDecoderInfo(decoder, &decoder_info));
  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetMetadataQueryReader(decoder, &metadata_reader));

  expect_eq_x(WINCODEC_ERR_PALETTEUNAVAILABLE, IWICBitmapDecoder_CopyPalette(decoder, (IWICPalette*)0xdeadbeef));
  expect_eq_x(WINCODEC_ERR_PALETTEUNAVAILABLE, IWICBitmapDecoder_CopyPalette(decoder, NULL));
  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetColorContexts(decoder, 4, contexts, &tmp_int));
  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetPreview(decoder, &bitmap_source));
  expect_eq_x(WINCODEC_ERR_CODECNOTHUMBNAIL, IWICBitmapDecoder_GetThumbnail(decoder, &bitmap_source));

  expect_eq_x(S_OK, IWICBitmapDecoder_GetFrameCount(decoder, &tmp_uint));
  expect_eq_x(1, tmp_uint);
  expect_eq_x(WINCODEC_ERR_FRAMEMISSING, IWICBitmapDecoder_GetFrame(decoder, 17, &frame));
  expect_eq_x(S_OK, IWICBitmapDecoder_GetFrame(decoder, 0, &frame));

  // For frame-level data:
  expect_eq_x(WINCODEC_ERR_PALETTEUNAVAILABLE, IWICBitmapFrameDecode_CopyPalette(frame, (IWICPalette*)0xdeadbeef));
  expect_eq_x(WINCODEC_ERR_CODECNOTHUMBNAIL, IWICBitmapFrameDecode_GetThumbnail(frame, &bitmap_source));
  expect_eq_x(S_OK, IWICBitmapFrameDecode_GetResolution(frame, &tmp_double, &tmp_double2));
  ok(tmp_double == 96.0, "X resolution mismatch - %f vs %f\n", tmp_double, 96.0);
  ok(tmp_double2 == 96.0, "Y resolution mismatch - %f vs %f\n", tmp_double2, 96.0);
  expect_eq_x(S_OK, IWICBitmapFrameDecode_GetColorContexts(frame, 4, NULL, &tmp_int));
  expect_eq_x(0, tmp_int);
  expect_eq_x(S_OK, IWICBitmapFrameDecode_GetColorContexts(frame, 4, contexts, &tmp_int));
  expect_eq_x(0, tmp_int);
  expect_eq_x(S_OK, IWICBitmapFrameDecode_GetSize(frame, &tmp_uint, &tmp_uint2));
  expect_eq_x(16, tmp_uint);
  expect_eq_x(16, tmp_uint2);
  expect_eq_x(S_OK, IWICBitmapFrameDecode_GetPixelFormat(frame, &guid));
  expect_eq_ws(guid_to_string(GUID_WICPixelFormat24bppBGR), guid_to_string(guid));

  // Initialize buffer to known values:
  for (i = 0; i < sizeof(buf); i++)
    buf[i] = i%227;

  // Stride too small for a single row.
  expect_eq_x(E_INVALIDARG, IWICBitmapFrameDecode_CopyPixels(frame, NULL, 14, sizeof(buf), (BYTE*)buf));
  // Buffer too small for all rows.
  expect_eq_x(WINCODEC_ERR_INSUFFICIENTBUFFER, IWICBitmapFrameDecode_CopyPixels(frame, NULL, 128, 128*10, (BYTE*)buf));
  // Buffer OK.
  expect_eq_x(S_OK, IWICBitmapFrameDecode_CopyPixels(frame, NULL, 128, sizeof(buf), (BYTE*)buf));

  // Check bytes outside of image not overwritten.
  for (i = 0; i < sizeof(buf); i++) {
    int col = i % 128;
    if (col >= 48)
      ok(buf[i] == i%227, "Byte %d overwritten, but shouldn't (%d vs %d)\n", i, buf[i], i);
  }

  // Check image.
  for (i = 0; i < 16; i++)
    for (j = 0; j < 16; j++) {
      DWORD expected;
      DWORD pixel;
      int r_diff, g_diff, b_diff;
      if (i < 8) {
        if (j < 8)
          expected = 0x20b020;
        else
          expected = 0xe02020;
      } else {
        if (j < 8)
          expected = 0xfff000;
        else
          expected = 0x00a0e0;
      }
      pixel = (*(DWORD*)&buf[i*128+3*j])&0xffffff;
      r_diff = abs((expected >> 16) - (pixel >> 16));
      g_diff = abs((expected >> 16) - (pixel >> 16));
      b_diff = abs((expected >> 16) - (pixel >> 16));
      ok(r_diff < 0x58 && g_diff < 0x58 && b_diff < 0x58,
         "Pixel (%d,%d) too different - expected %x got %x\n",
         i, j, expected, pixel);
      //trace("px[%d,%d] = %x\n", i, j, pixel);
    }

  // Copy subrect.
  expect_eq_x(S_OK, IWICBitmapFrameDecode_CopyPixels(frame, &rect, 4*3, 4*4*3, (BYTE*)buf2));
  // Check same thing copied:
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4*3; j++) {
      ok(buf2[4*3*i + j] == buf[128*(i+5) + j+3*5],
         "mismatch for pixel (%d,%d), channel %d (%x vs %x)\n", i+5, j/3+5, j%3,
         buf2[4*3*i + j], buf[128*(i+5) + j+3*5]);
    }

  rect.Height = 15;  // Rect too big.
  expect_eq_x(E_INVALIDARG, IWICBitmapFrameDecode_CopyPixels(frame, &rect, 128, sizeof(buf2), (BYTE*)buf2));


  expect_eq_x(webp_different(WINCODEC_ERR_UNSUPPORTEDOPERATION, S_OK), IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &metadata_reader));

  if (metadata_reader)
    IWICMetadataQueryReader_Release(metadata_reader);
  IWICComponentInfo_Release(frame);
  IWICBitmapDecoder_Release(decoder);
}

void test_capability(int codec_id) {
  IWICBitmapDecoder *decoder = NULL;
  IStream *stream = NULL;
  DWORD capability;

  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));

  stream = SHCreateMemStream(codec_valid_file[codec_id], 1024);
  expect_eq_x(S_OK, IWICBitmapDecoder_QueryCapability(decoder, stream, &capability));
  expect_eq_x(webp_different(
    WICBitmapDecoderCapabilityCanDecodeSomeImages,
    WICBitmapDecoderCapabilityCanDecodeAllImages|WICBitmapDecoderCapabilityCanEnumerateMetadata),
    capability);
  IStream_Release(stream);

  stream = SHCreateMemStream(invalid_file, 1024);
  expect_eq_x(WINCODEC_ERR_WRONGSTATE, IWICBitmapDecoder_QueryCapability(decoder, stream, &capability));
  IStream_Release(stream);

  IWICBitmapDecoder_Release(decoder);
}

// Tests an object created by the WIC factory.
void test_from_factory(int codec_id) {
  IWICBitmapDecoderInfo *decoder_info = NULL;
  IWICImagingFactory* factory = NULL;
  IWICBitmapDecoder* decoder = NULL;
  IStream *stream;
  GUID guid;

  expect_eq_x(S_OK, CoCreateInstance(&CLSID_WICImagingFactory, NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&factory));

  stream = SHCreateMemStream(codec_valid_file[codec_id], 1024);
  expect_eq_x(S_OK, IWICImagingFactory_CreateDecoderFromStream(factory,
      stream, NULL, WICDecodeMetadataCacheOnDemand, &decoder));

  expect_eq_x(S_OK, IWICBitmapDecoder_GetContainerFormat(decoder, &guid));
  expect_eq_ws(guid_to_string(*codec_container_guid[codec_id]), guid_to_string(guid));

  // GetDecoderInfo now works.
  expect_eq_x(S_OK, IWICBitmapDecoder_GetDecoderInfo(decoder, &decoder_info));
  if (decoder_info) {
    BOOL tmp_bool;
    GUID pixel_formats[10];
    UINT formats_count;
    expect_eq_x(S_OK, IWICBitmapDecoderInfo_DoesSupportLossless(decoder_info, &tmp_bool));
    expect_eq_x(FALSE, tmp_bool);
    expect_eq_x(S_OK, IWICBitmapCodecInfo_GetPixelFormats(decoder_info, 10, pixel_formats, &formats_count));
    ok(formats_count > 0, "Expected >0 pixel formats (for %d)\n", formats_count);
  }

  IStream_Release(stream);
  if (decoder_info)
    IWICBitmapDecoderInfo_Release(decoder_info);
  IWICBitmapDecoder_Release(decoder);
  IWICImagingFactory_Release(factory);
}

void test_with_bmp(int codec_id) {
  // We (currently) don't support metadata, like BMP, so let's check its return value.
  IWICMetadataQueryReader *metadata_reader = NULL;
  IWICBitmapFrameDecode *frame = NULL;
  IWICBitmapDecoder *decoder = NULL;
  IStream *stream;
  UINT tmp_uint, tmp_uint2;
  GUID guid;

  expect_eq_x(S_OK, CoCreateInstance(codec_decoder_clsid[codec_id], NULL,
      CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&decoder));
  expect_eq_x(S_OK, IWICBitmapDecoder_GetContainerFormat(decoder, &guid));
  expect_eq_ws(guid_to_string(*codec_container_guid[codec_id]), guid_to_string(guid));

  // Intialize with valid data...
  stream = SHCreateMemStream(codec_valid_file[codec_id], 1024);
  expect_eq_x(S_OK, IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand));
  IStream_Release(stream);

  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapDecoder_GetMetadataQueryReader(decoder, &metadata_reader));

  expect_eq_x(S_OK, IWICBitmapDecoder_GetFrameCount(decoder, &tmp_uint));
  expect_eq_x(1, tmp_uint);
  expect_eq_x(S_OK, IWICBitmapDecoder_GetFrame(decoder, 0, &frame));

  expect_eq_x(WINCODEC_ERR_UNSUPPORTEDOPERATION, IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &metadata_reader));

  IWICComponentInfo_Release(frame);
  IWICBitmapDecoder_Release(decoder);
}

START_TEST(info) {
  int codec_id;
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

  // Main test executed for Jpeg and WebP only.
  for (codec_id = 0; codec_id < 2; codec_id++) {
    trace("Testing codec %d\n", codec_id);
    test_uninitialized(codec_id);
    test_invalid(codec_id);
    test_initialized(codec_id);
    test_from_factory(codec_id);
    test_capability(codec_id);
  }

  // Some tests executed for WebP and Bmp.
  for (codec_id = 1; codec_id < 3; codec_id++) {
    trace("Metadata test: testing codec %d\n", codec_id);
    test_with_bmp(codec_id);
  }

  CoUninitialize();
  printf("Press any key...");
  getchar();
}
