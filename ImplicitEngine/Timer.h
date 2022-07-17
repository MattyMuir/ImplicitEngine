#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
using namespace std::chrono;

#define TIMING 1

#if TIMING
#define TIMER(x) Timer x
#define STOP_LOG(x) x.StopLog(#x)
#else
#define TIMER(x)
#define STOP_LOG(x)
#endif

class Timer
{
public:
    Timer()
        : startTimePoint(high_resolution_clock::now()) {}

    void StopLog(std::string_view timerName = "")
    {
        duration x = high_resolution_clock::now() - startTimePoint;
        uint64_t microSeconds = duration_cast<microseconds>(x).count();

        std::stringstream ss;
        if (microSeconds < 1000)
            ss << timerName << " took: " << microSeconds << "us\n";
        else
            ss << timerName << " took: " << microSeconds * 0.001 << "ms\n";

        std::cout << ss.str() << std::flush;
    }
private:
    time_point<high_resolution_clock> startTimePoint;
};