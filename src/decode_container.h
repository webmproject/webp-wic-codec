#ifndef WEBPWICCODEC_DECODE_CONTAINER_H
#define WEBPWICCODEC_DECODE_CONTAINER_H

#include <wincodec.h>
#include "decode_frame.h"
#include "main.h"
#include "utils.h"

// Base decoder class that can decode the WebP container and delegated the
// decoding of the VP8 stream to DecodeFrame.
// Thread-safety: protected by critical section cs_.
class DecodeContainer : public ComObjectBase<IWICBitmapDecoder> {
public:
  DecodeContainer() { InitializeCriticalSection(&cs_); }
  ~DecodeContainer() { }
  // IUnknown:
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
  ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
  ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
  // IWICBitmapDecoder:
  virtual HRESULT STDMETHODCALLTYPE QueryCapability(IStream *pIStream, DWORD *pdwCapability);
  virtual HRESULT STDMETHODCALLTYPE Initialize(IStream *pIStream, WICDecodeOptions cacheOptions);
  virtual HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID *pguidContainerFormat);
  virtual HRESULT STDMETHODCALLTYPE GetDecoderInfo(IWICBitmapDecoderInfo **ppIDecoderInfo);
  virtual HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette);
  virtual HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader);
  virtual HRESULT STDMETHODCALLTYPE GetPreview(IWICBitmapSource **ppIBitmapSource);
  virtual HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount);
  virtual HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail);
  virtual HRESULT STDMETHODCALLTYPE GetFrameCount(UINT *pCount);
  virtual HRESULT STDMETHODCALLTYPE GetFrame(UINT index, IWICBitmapFrameDecode **ppIBitmapFrame);
private:
  ComPtr<DecodeFrame> frame_;
  ComPtr<IWICImagingFactory> factory_;
  CRITICAL_SECTION cs_;
};

#endif  // WEBPWICCODEC_DECODE_CONTAINER_H