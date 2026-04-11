#include "ImageLoader.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <stdexcept>

#pragma comment(lib, "windowscodecs.lib")

namespace ImageLoader
{
    Surface LoadSurfaceFromFile(const std::wstring& path)
    {
        // Ignore S_FALSE (already initialized) and RPC_E_CHANGED_MODE (different apartment).
        // Do NOT CoUninitialize - COM lifetime is owned by the process (D3D11 may already hold it).
        const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE)
        {
            throw std::runtime_error("ImageLoader: CoInitializeEx failed");
        }

        Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        {
            throw std::runtime_error("ImageLoader: failed to create WIC factory");
        }

        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromFilename(
                path.c_str(), nullptr, GENERIC_READ,
                WICDecodeMetadataCacheOnLoad, &decoder)))
        {
            throw std::runtime_error("ImageLoader: failed to open file: " +
                                     std::string(path.begin(), path.end()));
        }

        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0u, &frame)))
        {
            throw std::runtime_error("ImageLoader: failed to get frame");
        }

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        if (FAILED(factory->CreateFormatConverter(&converter)))
        {
            throw std::runtime_error("ImageLoader: failed to create format converter");
        }

        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                         WICBitmapDitherTypeNone, nullptr, 0.0,
                                         WICBitmapPaletteTypeCustom)))
        {
            throw std::runtime_error("ImageLoader: failed to convert to RGBA");
        }

        UINT width = 0u;
        UINT height = 0u;
        converter->GetSize(&width, &height);

        Surface surface(width, height);
        converter->CopyPixels(nullptr, surface.GetPitch(),
                              surface.GetPitch() * height,
                              surface.GetPixels());
        return surface;
    }
}
