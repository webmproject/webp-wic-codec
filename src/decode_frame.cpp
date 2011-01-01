#include <windows.h>
#include "decode_frame.h"

#include <cassert>
#include <cstdlib>
#include <memory>

#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "webpimg.h"

const int kBytesPerPixel = 3;

YUVImage::~YUVImage() {
  // Note: only Y point to allocated memory. U and V are pointers inside Y.
  std::free(Y);
}

static void YUV420toRGB24Line(const BYTE* y_src,
                              const BYTE* u_src,
                              const BYTE* v_src,
                              int x_start,
                              int x_end,
                              BYTE* rgb_dst) {
  assert(x_start != x_end);
  y_src += x_start;
  u_src += x_start/2;
  v_src += x_start/2;
  if (x_start & 1) {
    WebpToRGB24(y_src[0], u_src[0], v_src[0], rgb_dst);
    rgb_dst += 3;
    ++u_src;
    ++v_src;
    ++y_src;
    x_start++;
  }
  int i;
  int len = (x_end - x_start)/2;
  for (i = 0; i < len; ++i) {
    const int U = u_src[0];
    const int V = v_src[0];
    WebpToRGB24(y_src[0], U, V, rgb_dst);
    WebpToRGB24(y_src[1], U, V, rgb_dst + 3);
    ++u_src;
    ++v_src;
    y_src += 2;
    rgb_dst += 6;
  }
  if (x_end & 1) {      /* Rightmost pixel */
    WebpToRGB24(y_src[0], (*u_src), (*v_src), rgb_dst);
  }
}

HRESULT DecodeFrame::CreateFromVP8Stream(BYTE* vp8_bitstream, DWORD stream_size, ComPtr<DecodeFrame>* ppOutput) {
  TRACE1("stream_size=%d\n", stream_size);
  HRESULT result = S_OK;
  ComPtr<DecodeFrame> output;

  (*ppOutput).reset(NULL);

  std::auto_ptr<YUVImage> pImage(new (std::nothrow) YUVImage());
  if (pImage.get() == NULL)
    return E_OUTOFMEMORY;

  // Note: according to the documentation, the actual decoding should happen
  // in CopyPixels. However, this would need to be implemented efficiently, as
  // e.g., the photo viewers load the image row by row, with multiple calls to
  // CopyPixels. Thus, for simplicity, we do all expect for YUV -> RGB
  // conversion here.
  WebPResult decode_result =
    VPXDecode(vp8_bitstream, stream_size, &pImage->Y,
              &pImage->U, &pImage->V, &pImage->width,
              &pImage->height);

  if (decode_result != webp_success || pImage->width < 0 || pImage->height < 0) {
    // We don't know what was the problem, but we assume it's a problem with
    // the content.
    // TODO: get better error codes.
    // Note: Win7 jpeg codec seems to prefer to return WINCODEC_ERR_BADHEADER
    // even for problems in the bitstream.
    TRACE("Couldn't decode VP8 stream.\n");
    return WINCODEC_ERR_BADIMAGE;
  }

  output.reset(new (std::nothrow) DecodeFrame(pImage.release()));
  if (output.get() == NULL)
    return E_OUTOFMEMORY;

  (*ppOutput).reset(output.new_ref());
  return S_OK;
}

HRESULT DecodeFrame::QueryInterface(REFIID riid, void **ppvObject) {
  TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

  if (ppvObject == NULL)
    return E_INVALIDARG;
  *ppvObject = NULL;

  if (!IsEqualGUID(riid, IID_IUnknown) &&
      !IsEqualGUID(riid, IID_IWICBitmapFrameDecode) &&
      !IsEqualGUID(riid, IID_IWICBitmapSource))
    return E_NOINTERFACE;
  this->AddRef();
  *ppvObject = static_cast<IWICBitmapFrameDecode*>(this);
  return S_OK;
}

HRESULT DecodeFrame::GetSize(UINT *puiWidth, UINT *puiHeight) {
  TRACE2("(%p, %p)\n", puiWidth, puiHeight);
  if (puiWidth == NULL || puiHeight == NULL)
    return E_INVALIDARG;
  *puiWidth = (UINT)image_->width;
  *puiHeight = (UINT)image_->height;
  TRACE2("ret: %u x %u\n", *puiWidth, *puiHeight);
  return S_OK;
}

HRESULT DecodeFrame::GetPixelFormat(WICPixelFormatGUID *pPixelFormat) {
  TRACE1("(%p)\n", pPixelFormat);
  if (pPixelFormat == NULL)
    return E_INVALIDARG;
  *pPixelFormat = GUID_WICPixelFormat24bppBGR;
  return S_OK;
}

HRESULT DecodeFrame::GetResolution(double *pDpiX, double *pDpiY) {
  TRACE2("(%p, %p)\n", pDpiX, pDpiY);
  // Let's assume square pixels. 96dpi seems to be a reasonable default.
  *pDpiX = 96;
  *pDpiY = 96;
  return S_OK;
}

HRESULT DecodeFrame::CopyPalette(IWICPalette *pIPalette) {
  TRACE1("(%p)\n", pIPalette);
  return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

HRESULT DecodeFrame::CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer) {
  TRACE4("(%p, %u, %u, %p)\n", prc, cbStride, cbBufferSize, pbBuffer);
  if (pbBuffer == NULL)
    return E_INVALIDARG;
  WICRect rect = {0, 0, image_->width, image_->height};
  if (prc)
    rect = *prc;

  if (rect.Width < 0 || rect.Height < 0 || rect.X < 0 || rect.Y < 0)
    return E_INVALIDARG;
  if (rect.X + rect.Width > image_->width ||
      rect.Y + rect.Height > image_->height)
    return E_INVALIDARG;

  // Divisions instead of multiplications to avoid integer overflows:
  if (cbStride / kBytesPerPixel < static_cast<UINT>(rect.Width))
    return E_INVALIDARG;
  if (cbBufferSize / cbStride < static_cast<UINT>(rect.Height))
    return WINCODEC_ERR_INSUFFICIENTBUFFER;

  if (rect.Width == 0 || rect.Height == 0)
    return S_OK;

  int y_stride = image_->width;
  int uv_stride = ((y_stride + 1) >> 1);
  /* note that the U, V upsampling in height is happening here as the U, V
   * buffers sent to successive odd-even pair of lines is same.
   */
  BYTE *dst_buffer = pbBuffer;
  for (int src_y = rect.Y; src_y < rect.Y + rect.Height; ++src_y) {
    YUV420toRGB24Line(image_->Y + src_y * y_stride,
                      image_->U + (src_y >> 1) * uv_stride,
                      image_->V + (src_y >> 1) * uv_stride,
                      rect.X,
                      rect.X + rect.Width,
                      dst_buffer);
    dst_buffer += cbStride;
  }

  return S_OK;
}

HRESULT DecodeFrame::GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader) {
  TRACE1("(%p)\n", ppIMetadataQueryReader);
  return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT DecodeFrame::GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount) {
  TRACE3("(%d, %p, %p)\n", cCount, ppIColorContexts, pcActualCount);
  if (pcActualCount == NULL)
    return E_INVALIDARG;
  *pcActualCount = 0;
  return S_OK;
}

HRESULT DecodeFrame::GetThumbnail(IWICBitmapSource **ppIThumbnail) {
  TRACE1("(%p)\n", ppIThumbnail);
  return WINCODEC_ERR_CODECNOTHUMBNAIL;
}


HRESULT DummyFrame::QueryInterface(REFIID riid, void **ppvObject) {
  TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

  if (ppvObject == NULL)
    return E_INVALIDARG;
  *ppvObject = NULL;

  if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IWICBitmapFrameDecode))
    return E_NOINTERFACE;
  this->AddRef();
  *ppvObject = static_cast<IWICBitmapFrameDecode*>(this);
  return S_OK;
}
