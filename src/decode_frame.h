// Copyright 2010 Google Inc.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// Interface to the VP8 frame.
//
// Author: Mikolaj Zalewski (mikolajz@google.com)

#ifndef WEBPWICCODEC_DECODE_FRAME_H
#define WEBPWICCODEC_DECODE_FRAME_H

#include <wincodec.h>
#include "main.h"
#include "utils.h"

struct YUVImage {
  YUVImage() :Y(NULL), U(NULL), V(NULL), width(0), height(0) {}
  ~YUVImage();

  BYTE* Y;
  BYTE* U;
  BYTE* V;
  int width;
  int height;
  int yStride;
  int uvStride;
};

// The single frame available in a WebP image file in a decoded form.
// Thread-safety: immutable.
class DecodeFrame : public ComObjectBase<IWICBitmapFrameDecode> {
public:
  DecodeFrame(YUVImage* image) :image_(image) { }
  ~DecodeFrame() { delete image_; }

  static HRESULT CreateFromVP8Stream(BYTE* vp8_bitstream, DWORD stream_size, ComPtr<DecodeFrame>* ppOutput);

  // IUnknown:
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
  ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
  ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
  // IWICBitmapSource:
  virtual HRESULT STDMETHODCALLTYPE GetSize(UINT *puiWidth, UINT *puiHeight);
  virtual HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID *pPixelFormat);
  virtual HRESULT STDMETHODCALLTYPE GetResolution(double *pDpiX, double *pDpiY);
  virtual HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette);
  virtual HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer);
  // IWICBitmapFrameDecode:
  virtual HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader);
  virtual HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount);
  virtual HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail);
private:
  const YUVImage * const image_;
};


// A dummy frame returned by an DecodeContainer that has not been initialized.
class DummyFrame : public ComObjectBase<IWICBitmapFrameDecode> {
public:
  DummyFrame() { }
  // IUnknown:
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
  ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
  ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
  // IWICBitmapSource:
  virtual HRESULT STDMETHODCALLTYPE GetSize(UINT *puiWidth, UINT *puiHeight) {
    // Win7 JPEG codec returns S_OK here (without writing anything to
    // puiWidth/puiHeight). Let's hope it's only not for some compatibility and
    // return an error code.
    return WINCODEC_ERR_WRONGSTATE;
  }

  virtual HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID *pPixelFormat) {
    // Win7 JPEG codec returns S_OK here (without writing anything to
    // pPixelFormat). Let's hope it's only not for some compatibility and
    // return an error code.
    return WINCODEC_ERR_WRONGSTATE;
  }

  virtual HRESULT STDMETHODCALLTYPE GetResolution(double *pDpiX, double *pDpiY) {
    return WINCODEC_ERR_WRONGSTATE;
  }

  virtual HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette) {
    return WINCODEC_ERR_WRONGSTATE;
  }

  virtual HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer) {
    return E_INVALIDARG;
  }

  // IWICBitmapFrameDecode:
  virtual HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader) {
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
  }
  virtual HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount) {
    return WINCODEC_ERR_BADIMAGE;
  }
  virtual HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail) {
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
  }
};

#endif  // WEBPWICCODEC_DECODE_FRAME_H