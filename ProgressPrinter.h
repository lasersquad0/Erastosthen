#pragma once
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <mutex>
#include "CriticalSection.h"

class ProgressPrinter
{
private:
	uint64_t m_p100 = 0;
	uint64_t m_delta = 0;
	uint64_t m_current = 0;
	streamsize m_defPrecision = 0;
	streamsize m_defWidth = 0;
protected:
	CriticalSection m_mutex;
	//mutex  m_mutex;
public:
	inline void startProgress(uint64_t fullhouse)
	{
		m_p100 = fullhouse;
		m_delta = m_p100 / 100;
		m_current = 1;
		
		m_defPrecision = std::cout.precision();
		m_defWidth = cout.width();

		cout << setw(7) << setprecision(4);
	}

	inline void finishProgress()
	{
		cout << setprecision(m_defPrecision) << setw(m_defWidth);
		//cout << "\r                                \r"; // cleaning up progress line of text
		cout << endl;
	}

	inline void updateProgress(uint64_t progress)
	{
		if (progress > m_current)
		{
			m_mutex.lock();
			cout << "\rProgress ... " /* << setw(7) << setprecision(4) */ << (float)(100 * m_current) / (float)m_p100 << "%      ";
			m_current += m_delta;
			m_mutex.unlock();
		}
	}
};

