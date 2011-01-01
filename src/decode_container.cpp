#include <windows.h>
#include "decode_container.h"

#include <new>
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

HRESULT ParseHeader(IStream* pIStream, DWORD* container_size, DWORD* vp8_size) {
  HRESULT ret;
  BYTE header[20];
  ULONG read;

  if (FAILED(ret = pIStream->Read(header, 20, &read)))
    return ret;
  if (read < 20 || memcmp(&header[0], "RIFF", 4) != 0 ||
      memcmp(&header[8], "WEBPVP8 ", 8) != 0) {
    TRACE("Bad magic\n");
    return WINCODEC_ERR_BADHEADER;
  }

  *container_size = get_le32(&header[4]);
  *vp8_size = get_le32(&header[16]);
  // Check that VP8 data fits in the container.
  if (*vp8_size + 12 > *container_size) {
    TRACE2("Wrong sizes (container: %d, vp8: %d)\n", container_size, vp8_size);
    return WINCODEC_ERR_BADHEADER;
  }

  *container_size -= 12;  // Take into account the header we've already read.
  return S_OK;
}

HRESULT DecodeContainer::QueryCapability(IStream* pIStream, DWORD* pdwCapability) {
  TRACE2("(%p, %x)\n", pIStream, pdwCapability);
  if (pdwCapability == NULL)
    return E_INVALIDARG;

  DWORD tmp1, tmp2;
  HRESULT ret = ParseHeader(pIStream, &tmp1, &tmp2);
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
  DWORD container_size, vp8_size;
  ULONG read;

  ret = ParseHeader(pIStream, &container_size, &vp8_size);
  if (FAILED(ret))
    return ret;

  scoped_buffer vp8_data(vp8_size);
  if (vp8_data.alloc_failed()) {
    TRACE1("Couldn't allocate %d for VP8 data\n", vp8_size);
    return E_OUTOFMEMORY;
  }
  if (FAILED(ret = pIStream->Read(vp8_data.get(), vp8_size, &read)))
    return ret;
  if (read < vp8_size) {
    TRACE2("Premature end of file (read %d instead of %d)\n", read, vp8_size);
    return WINCODEC_ERR_BADHEADER;
  }

  ComPtr<DecodeFrame> tmp_frame;
  ret = DecodeFrame::CreateFromVP8Stream(vp8_data.get(), vp8_size, &tmp_frame);
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
