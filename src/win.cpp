#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

void usleep(unsigned int ms)
{
	ms /= 1000;
	if (ms)
		::Sleep(ms);
}
