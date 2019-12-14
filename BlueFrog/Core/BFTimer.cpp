#include "BFTimer.h"

using namespace std::chrono;

BFTimer::BFTimer()
{
	last = steady_clock::now();
}

float BFTimer::Mark()
{
	const auto old = last;
	last = steady_clock::now();
	const duration<float> frameTime = last - old;
	return frameTime.count();
}

float BFTimer::Peek() const
{
	return duration<float>(steady_clock::now() - last).count();
}
