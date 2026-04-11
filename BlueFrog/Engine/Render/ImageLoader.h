#pragma once
#include "Surface.h"
#include <string>

namespace ImageLoader
{
    // Loads an RGBA image file from disk using WIC.
    // path is relative to the working directory.
    // Throws std::runtime_error on failure.
    Surface LoadSurfaceFromFile(const std::wstring& path);
}
