#pragma once

#include <windows.h>


class CriticalSection
{
public:
	CriticalSection()
	{
		InitializeCriticalSection(&cs);
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&cs);
	}

	void lock()
	{
		EnterCriticalSection(&cs);
	}

	void unlock()
	{
		LeaveCriticalSection(&cs);
	}

private:
	CriticalSection(const CriticalSection&) = delete;
	void operator=(const CriticalSection&) = delete;

	CRITICAL_SECTION cs;
};