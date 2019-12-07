#pragma once
#include "BFException.h"
#include <sstream>


BFException::BFException(int line, const char* file) noexcept
	:
	line(line),
	file(file)
{}

const char* BFException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* BFException::GetType() const noexcept
{
	return "BlueFrog Exception";
}

int BFException::GetLine() const noexcept
{
	return line;
}

const std::string& BFException::GetFile() const noexcept
{
	return file;
}

std::string BFException::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << std::endl
		<< "[Line] " << line;
	return oss.str();
}
