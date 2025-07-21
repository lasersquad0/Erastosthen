#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <mutex>
//#include "CriticalSection.h"

class ProgressPrinter
{
private:
	uint64_t m_p100 = 0;
	uint64_t m_delta = 0;
	uint64_t m_current = 0;
	std::streamsize m_defPrecision = 0;
	std::streamsize m_defWidth = 0;
protected:
	//CriticalSection m_mutex;
	std::mutex m_mutex;
public:
	inline void startProgress(uint64_t fullhouse)
	{
		m_p100 = fullhouse;
		m_delta = m_p100 / 100;
		m_current = 1;
		
		m_defPrecision = std::cout.precision();
		m_defWidth = std::cout.width();

		std::cout << std::setw(7) << std::setprecision(4);
	}

	inline void finishProgress()
	{
		std::cout << std::setprecision(m_defPrecision) << std::setw(m_defWidth);
		//cout << "\r                                \r"; // cleaning up progress line of text
		std::cout << std::endl;
	}

	inline void updateProgress(uint64_t progress)
	{
		if (progress > m_current)
		{
			//TODO use mutex guard here?
			m_mutex.lock();
			std::cout << "\rProgress ... " << (float)(100 * m_current) / (float)m_p100 << "%      ";
			m_current += m_delta;
			m_mutex.unlock();
		}
	}
};

