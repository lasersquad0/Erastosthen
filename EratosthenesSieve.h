#pragma once

#include <string>
#include <windows.h>
#include <synchapi.h>
#include "PrimesFIO.h"
#include "SegmentedArray.h"
#include "Parameters.h"


class invalid_cmd_option: public invalid_argument
{
public:
	invalid_cmd_option(string str) :invalid_argument(str)
	{
	}
};

class EratosthenesSieve;
void FindPrimesThread(EratosthenesSieve* es); // forward declaration

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

class EratosthenesSieve
{
private:
	friend void FindPrimesThread(EratosthenesSieve* es);

	const string fSymbols = "BMGTP";
	static const uint64_t FACTOR_B  = 1ULL; // B means "bytes". so the factor = 1.
	static const uint64_t FACTOR_M  = 1'000'000ULL; // 'megabytes'
	static const uint64_t FACTOR_G  = 1'000'000'000ULL; // 'gigabytes'
	static const uint64_t FACTOR_T  = 1'000'000'000'000ULL; // terabytes
	static const uint64_t FACTOR_P  = 1'000'000'000'000'000ULL; // petabytes
	static const uint64_t MAX_START_M = 18400'000'000'000ULL; // 18400P
	static const uint64_t MAX_START_G = 18400'000'000ULL; // 18400P
	static const uint64_t MAX_START_T = 18400'000ULL; // 18400P
	static const uint64_t MAX_START_P = 18400ULL; // 18400P
	static const uint64_t MAX_LEN   = 1'000'000'000'000ULL; // terabyte

	char symFACTORstart = 'G'; // символьное представление Factor дл€ значени€ m_realStart дл€ формировани€ имени output файла.
	char symFACTORlen = 'G'; // символьное представление Factor дл€ значени€ m_realLength дл€ формировани€ имени output файла.

	uint64_t FACTORstart = FACTOR_G; // G по умолчанию по потом перепишетс€ исход€ из переданного диапазона.
	uint64_t FACTORlen = FACTOR_G;
	bool OPTIMUM_MODE = true;
	PRIMES_FILE_FORMATS OUTPUT_FILE_TYPE; // 1-just txt file no diff.  2-txt file with diff. 3 - bin file no diff. 4-bin file with diff. 5-bin file with variable diff

	uint64_t m_realStart;  
	uint64_t m_realLength; // ƒлина промежутка на котором ищем простые

	SegmentedArray* m_bitArray;
	mutex m_mutex;
	atomic_uint m_threadIDs = 0;
	uint32_t m_readPrimes;
	uint64_t* m_prldPrimes;
	uint32_t m_delta;

	typedef pair<uint64_t, uint64_t> rangePair;
	//typedef CriticalSection SyncType;
	typedef mutex SyncType;

	typedef pair<SyncType*, rangePair> rangeItem;
	vector<rangeItem> m_ranges;
	

	void CalculateOptimum();
	void CalculateSimple();

	uint64_t saveAsTXTSimpleMode(uint64_t START, SegmentedArray *sarr, string outputFilename, bool diffMode);
	uint64_t saveAsTXTOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsTXTDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBINSimpleMode(uint64_t start, SegmentedArray* sarr, string outputFilename, bool diffMode);
	uint64_t saveAsBINOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBINDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBINDiffVarOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBINDiffVarSimpleMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	//uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint64_t stopPrime);
	uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint32_t len, uint64_t stopPrime);
	uint32_t LoadPrimesFromBINDiffVar(string filename, uint64_t* primes, size_t len, uint64_t stopPrime);
	
	string getOutputFilename();
	uint64_t parseOption(string s, char* symFactor, uint64_t* factor);
	void checkStartParam(string startValue);
	void defineRanges();
	rangeItem* getRange(uint64_t v);
	void printTime(string msg);
	string millisecToStr(long long ms);
	uint64_t nextPrime();
	void FindPrimesInThreads(int numThreads);

public:
	static const uint64_t DEFAULT_START = 20ULL * FACTOR_T; // это реальные primes откуда стартуем если не передано в параметрах command line
	static const uint64_t DEFAULT_LENGTH = 10'000'000'000ULL; // ƒлина промежутка на котором ищем простые. «десь всегда фиксирована (hardcoded)

	EratosthenesSieve(bool mode, PRIMES_FILE_FORMATS oFileType, uint64_t strt, uint64_t len, string primesInputFile = Pre_Loaded_Primes_Filename);
	EratosthenesSieve();

	//void parseCmdLine(int argc, char* argv[]);
	void parseParams(string mode, string oft, string start, string len);
	void printUsage();
	void Calculate();

};


