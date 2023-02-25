#pragma once
#include <string>
#include <chrono>

#define TIMING 1

#if TIMING
#define TIMER(name) Timer name
#define STOP_LOG(name) { name.Stop(false); std::cout << #name << " took: "; name.Log(); }
#define TIME_SCOPE(name) ScopedTimer name(#name)
#else
#define TIMER(name)
#define STOP_LOG(name)
#define TIME_SCOPE(name)
#endif

class Timer
{
	using TimePoint = std::chrono::system_clock::time_point;
	using Duration = std::chrono::system_clock::duration;

public:
	Timer(bool start = true)
		: duration(0)
	{
		if (start) Start();
	}

	void Start()
	{
		using namespace std::chrono;
		start = system_clock::now();
	}
	void Stop(bool log = true)
	{
		using namespace std::chrono;
		TimePoint end = system_clock::now();
		duration += (end - start);

		if (log) Log();
	}

	void Log()
	{
		using namespace std::chrono;
		static constexpr auto res = duration_cast<Duration>(1s).count();

		auto count = duration.count();
		double millis = (double)count / res * 1000.0;

		if (millis < 1.0) std::cout << millis * 1000 << "us\n";
		else std::cout << millis << "ms\n";
	}
	Duration GetDuration() { return duration; }

protected:
	TimePoint start;
	Duration duration;
};

class ScopedTimer : Timer
{
public:
	ScopedTimer(std::string_view name_) : Timer(false), name(name_) { Start(); }
	~ScopedTimer() { Stop(false); std::cout << name << " took: "; Log(); }

protected:
	std::string name;
};