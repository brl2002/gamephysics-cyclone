#ifndef PROFILER_H
#define PROFILER_H

class Profiler
{
public:
	Profiler();

	void StartTally();
	void EndTally();

protected:
    unsigned frameNumber;

    unsigned lastFrameTimestamp;

    unsigned lastFrameDuration;

    unsigned long lastFrameClockstamp;

    unsigned long lastFrameClockTicks;

	unsigned GetSystemTime();


};

#endif