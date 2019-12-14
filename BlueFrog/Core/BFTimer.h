#pragma once
#include <chrono>

class BFTimer
{
public:
	BFTimer();
	float Mark();
	float Peek() const;
private:
	std::chrono::steady_clock::time_point last;
};