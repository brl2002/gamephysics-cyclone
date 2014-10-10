#include "Profiler.h"

#include <windows.h>
#include <mmsystem.h>

static double qpcFrequency;

static bool profilerQPCFlag;

unsigned Profiler::GetSystemTime()
{
    if(profilerQPCFlag)
    {
        static LONGLONG qpcMillisPerTick;
        QueryPerformanceCounter((LARGE_INTEGER*)&qpcMillisPerTick);
        return (unsigned)(qpcMillisPerTick * qpcFrequency);
    }
    else
    {
        return unsigned(timeGetTime());
    }
}