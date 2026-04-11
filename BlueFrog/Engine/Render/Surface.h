#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

class Surface final
{
public:
	struct RGBA
	{
		std::uint8_t r;
		std::uint8_t g;
		std::uint8_t b;
		std::uint8_t a;
	};

	Surface() = default;
	Surface(unsigned int width, unsigned int height)
		:
		width(width),
		height(height),
		pixels(width * height * 4u)
	{
	}

	static Surface MakeCheckerboard(
		unsigned int width,
		unsigned int height,
		unsigned int cellSize = 8u,
		RGBA colorA = { 230u, 230u, 230u, 255u },
		RGBA colorB = { 60u, 60u, 60u, 255u })
	{
		Surface surface(width, height);
		surface.FillCheckerboard(cellSize, colorA, colorB);
		return surface;
	}

	unsigned int GetWidth() const noexcept
	{
		return width;
	}

	unsigned int GetHeight() const noexcept
	{
		return height;
	}

	unsigned int GetPitch() const noexcept
	{
		return width * 4u;
	}

	const std::uint8_t* GetPixels() const noexcept
	{
		return pixels.data();
	}

	std::uint8_t* GetPixels() noexcept
	{
		return pixels.data();
	}

	const void* GetData() const noexcept
	{
		return pixels.data();
	}

private:
	void FillCheckerboard(unsigned int cellSize, RGBA colorA, RGBA colorB) noexcept
	{
		cellSize = std::max(1u, cellSize);

		for (unsigned int y = 0; y < height; ++y)
		{
			for (unsigned int x = 0; x < width; ++x)
			{
				const bool useColorA = (((x / cellSize) + (y / cellSize)) & 1u) == 0u;
				const RGBA color = useColorA ? colorA : colorB;
				const unsigned int index = (y * width + x) * 4u;
				pixels[index + 0u] = color.r;
				pixels[index + 1u] = color.g;
				pixels[index + 2u] = color.b;
				pixels[index + 3u] = color.a;
			}
		}
	}

private:
	unsigned int width = 0u;
	unsigned int height = 0u;
	std::vector<std::uint8_t> pixels;
};
