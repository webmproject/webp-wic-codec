// Copyright 2010 Google Inc.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// Interface to the RIFF container and creating a VP8 frame object.
//
// Author: Mikolaj Zalewski (mikolajz@google.com)

#include <windows.h>
#include "decode_container.h"

#include <new>
#include "webp/decode.h"  // for WebPGetInfo
#include "decode_frame.h"
#include "uuid.h"

inline DWORD get_le32(const BYTE* data) {
  // All architectures we support are little-endian and don't care about
  // alignment:
  return *(DWORD *)data;
}

HRESULT DecodeContainer::QueryInterface(REFIID riid, void** ppvObject) {
  TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

  if (ppvObject == NULL)
    return E_INVALIDARG;
  *ppvObject = NULL;

  if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IWICBitmapDecoder))
    return E_NOINTERFACE;
  this->AddRef();
  *ppvObject = static_cast<IWICBitmapDecoder*>(this);
  return S_OK;
}

// Parse the RIFF header and return the total file size in '*file_size'.
// pIStream's seek pointer is left unchanged on success.
HRESULT ParseHeader(IStream* pIStream, DWORD* file_size) {
  // read enough so WebPGetInfo can do basic validation.
  const int kHeaderSize = 32;
  BYTE header[kHeaderSize];
  HRESULT ret;
  ULONG read;

  if (FAILED(ret = pIStream->Read(header, sizeof(header), &read)))
    return ret;
  if (read < kHeaderSize) {
    TRACE("Read error\n");
    return E_UNEXPECTED;
  }

  if (!WebPGetInfo(header, kHeaderSize, NULL, NULL)) {
    TRACE("WebPGetInfo failed\n");
    return WINCODEC_ERR_BADIMAGE;
  }

  const DWORD riff_size = get_le32(&header[4]);
  *file_size = riff_size + 8;

  // reset pIStream
  LARGE_INTEGER offset;
  offset.QuadPart = -kHeaderSize;
  ret = pIStream->Seek(offset, STREAM_SEEK_CUR, NULL);
  return ret;
}

HRESULT DecodeContainer::QueryCapability(IStream* pIStream, DWORD* pdwCapability) {
  TRACE2("(%p, %x)\n", pIStream, pdwCapability);
  if (pdwCapability == NULL)
    return E_INVALIDARG;

  DWORD tmp1;
  HRESULT ret = ParseHeader(pIStream, &tmp1);
  if (ret == WINCODEC_ERR_BADHEADER)
    return WINCODEC_ERR_WRONGSTATE;  // That's what Win7 jpeg codec returns.
  if (ret == S_OK)
    // TODO: should we check if we really can decode the VP8 bitstream?
    *pdwCapability = WICBitmapDecoderCapabilityCanDecodeSomeImages;
  return ret;
}

HRESULT DecodeContainer::Initialize(IStream* pIStream, WICDecodeOptions cacheOptions) {
  TRACE2("(%p, %x)\n", pIStream, cacheOptions);

  if (pIStream == NULL)
    return E_INVALIDARG;

  {
    SectionLock l(&cs_);
    if (frame_.get() != NULL)
      return WINCODEC_ERR_WRONGSTATE;
    // This may change, so we will check it again, after loading the file.
  }

  HRESULT ret;
  DWORD file_size;

  ret = ParseHeader(pIStream, &file_size);
  if (FAILED(ret))
    return ret;

  scoped_buffer file_data(file_size);
  if (file_data.alloc_failed()) {
    TRACE1("Couldn't allocate %u for file data\n", file_size);
    return E_OUTOFMEMORY;
  }

  ULONG read;
  if (FAILED(ret = pIStream->Read(file_data.get(), file_size, &read)))
    return ret;
  if (read < file_size) {
    TRACE2("Premature end of file (read %u instead of %u)\n", read, file_size);
    return WINCODEC_ERR_BADHEADER;
  }

  ComPtr<DecodeFrame> tmp_frame;
  ret = DecodeFrame::CreateFromVP8Stream(file_data.get(), file_size,
                                         &tmp_frame);
  if (FAILED(ret))
    return ret;

  {
    SectionLock l(&cs_);
    if (frame_.get() != NULL)
      return WINCODEC_ERR_WRONGSTATE;
    frame_.reset(tmp_frame.new_ref());
  }

  return S_OK;
}

HRESULT DecodeContainer::GetContainerFormat(GUID* pguidContainerFormat) {
  TRACE1("(%p)\n", pguidContainerFormat);
  if (pguidContainerFormat == NULL)
    return E_INVALIDARG;
  *pguidContainerFormat = GUID_ContainerFormatWebp;
  return S_OK;
}

HRESULT DecodeContainer::GetDecoderInfo(IWICBitmapDecoderInfo** ppIDecoderInfo) {
  TRACE1("(%p)\n", ppIDecoderInfo);
  HRESULT result;
  ComPtr<IWICImagingFactory> factory;

  {
    SectionLock l(&cs_);
    if (factory_.get() == NULL) {
      result = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory_.get_out_storage());
      if (FAILED(result))
        return result;
    }
    factory.reset(factory_.new_ref());
  }

  ComPtr<IWICComponentInfo> compInfo;
  result = factory->CreateComponentInfo(CLSID_WebpWICDecoder, compInfo.get_out_storage());
  if (FAILED(result))
    return result;

  result = compInfo->QueryInterface(IID_IWICBitmapDecoderInfo, (void**)ppIDecoderInfo);
  if (FAILED(result))
    return result;

  return S_OK;
}

HRESULT DecodeContainer::CopyPalette(IWICPalette* pIPalette) {
  TRACE1("(%p)\n", pIPalette);
  return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

HRESULT DecodeContainer::GetMetadataQueryReader(IWICMetadataQueryReader** ppIMetadataQueryReader) {
  TRACE1("(%p)\n", ppIMetadataQueryReader);
  return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT DecodeContainer::GetPreview(IWICBitmapSource** ppIBitmapSource) {
  TRACE1("(%p)\n", ppIBitmapSource);
  return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT DecodeContainer::GetColorContexts(UINT cCount, IWICColorContext** ppIColorContexts, UINT* pcActualCount) {
  TRACE3("(%d, %p, %p)\n", cCount, ppIColorContexts, pcActualCount);
  return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT DecodeContainer::GetThumbnail(IWICBitmapSource** ppIThumbnail) {
  TRACE1("(%p)\n", ppIThumbnail);
  return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

HRESULT DecodeContainer::GetFrameCount(UINT* pCount) {
  TRACE1("(%p)\n", pCount);
  if (pCount == NULL)
    return E_INVALIDARG;
  *pCount = 1;
  return S_OK;
}

HRESULT DecodeContainer::GetFrame(UINT index, IWICBitmapFrameDecode** ppIBitmapFrame) {
  TRACE2("(%d, %p)\n", index, ppIBitmapFrame);
  if (ppIBitmapFrame == NULL)
    return E_INVALIDARG;
  if (index != 0)
    return WINCODEC_ERR_FRAMEMISSING;

  SectionLock l(&cs_);

  if (frame_.get() == NULL) {
    TRACE("No initialize - returning dummy frame\n");
    *ppIBitmapFrame = new (std::nothrow) DummyFrame();
    if (!*ppIBitmapFrame)
      return E_OUTOFMEMORY;
  } else {
    *ppIBitmapFrame = frame_.new_ref();
  }
  return S_OK;
}
