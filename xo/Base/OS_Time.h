#pragma once

namespace xo {

XO_API void   SleepMS(int milliseconds);
XO_API double TimeAccurateSeconds(); // Arbitrary epoch, but accurate (QueryPerformanceCounter, CLOCK_MONOTONIC)
XO_API int64_t MilliTicks();         // Returns TimeAccurateSeconds * 1000
XO_API int64_t NanoTicks();          // Returns TimeAccurateSeconds * 1000,0000,000

// Truncate milliticks to 32-bit, for storage. Comparisons are still meaningful because of 2's complement subtraction
inline uint32_t MilliTicks_32(int64_t ms64) { return (uint32_t)((uint64_t) ms64); }

// Used to measure elapsed time of a piece of code
class XO_API CodeTimer {
public:
	double Start;
	CodeTimer() { Start = TimeAccurateSeconds(); }
	double Measure() const { return TimeAccurateSeconds() - Start; }
	double MeasureAndRestart() {
		double t = Measure();
		Start    = TimeAccurateSeconds();
		return t;
	}
};
}