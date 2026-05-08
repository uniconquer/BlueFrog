#pragma once
#include "Surface.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ImageLoader
{
    // Loads an RGBA image file from disk using WIC.
    // path is relative to the working directory.
    // Throws std::runtime_error on failure.
    Surface LoadSurfaceFromFile(const std::wstring& path);

    // Decodes an in-memory image (PNG / JPEG / etc., anything WIC supports
    // out of the box) into an RGBA Surface. Used for embedded glTF images
    // (data: URI base64 payloads OR bufferView-stored binaries) where the
    // bytes never live on disk. Throws std::runtime_error on failure.
    Surface LoadSurfaceFromMemory(const std::uint8_t* data, std::size_t size);
}
