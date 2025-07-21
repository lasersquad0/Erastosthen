#pragma once

#include <string>
//#include <windows.h>
//#include <synchapi.h>
#include "PrimesFIO.h"
#include "SegmentedArray.h"
#include "Parameters.h"
#include "ProgressPrinter.h"
//#include "CriticalSection.h"

class invalid_cmd_option: public std::invalid_argument
{
public:
	invalid_cmd_option(std::string str): invalid_argument(str)
	{
	}
};

class EratosthenesSieve;
void FindPrimesThread(EratosthenesSieve* es); // forward declaration


class EratosthenesSieve: public ProgressPrinter
{
private:
	friend void FindPrimesThread(EratosthenesSieve* es);

	const std::string fSymbols = "BKMGTP";
	static const uint64_t FACTOR_B  = 1ULL; // B means "bytes", so the factor = 1.
	static const uint64_t FACTOR_K  = 1'000ULL; // 'kilobytes'
	static const uint64_t FACTOR_M  = 1'000'000ULL; // 'megabytes'
	static const uint64_t FACTOR_G  = 1'000'000'000ULL; // 'gigabytes'
	static const uint64_t FACTOR_T  = 1'000'000'000'000ULL; // 'terabytes'
	static const uint64_t FACTOR_P  = 1'000'000'000'000'000ULL; // petabytes
	static const uint64_t MAX_START_K = 18400'000'000'000'000ULL; // 18400P
	static const uint64_t MAX_START_M = 18400'000'000'000ULL; // 18400P
	static const uint64_t MAX_START_G = 18400'000'000ULL; // 18400P
	static const uint64_t MAX_START_T = 18400'000ULL; // 18400P
	static const uint64_t MAX_START_P = 18400ULL; // 18400P
	static const uint64_t MAX_LEN   = 1'000'000'000'000ULL; // 1 terabyte (1T)

	char symFACTORstart = 'G'; // символьное представление Factor для значения m_realStart для формирования имени output файла.
	char symFACTORlen = 'G'; // символьное представление Factor для значения m_realLength для формирования имени output файла.

	uint64_t FACTORstart = FACTOR_G; // G по умолчанию по потом перепишется исходя из переданного диапазона.
	uint64_t FACTORlen = FACTOR_G;
	bool OPTIMUM_MODE = true;
	PRIMES_FILE_FORMATS OUTPUT_FILE_TYPE; // 1-just txt file no diff.  2-txt file with diff. 3 - bin file no diff. 4-bin file with diff. 5-bin file with variable diff

	uint64_t m_realStart;  
	uint64_t m_realLength; // Длина промежутка на котором ищем простые
	std::string m_primesInputFile;
	SegmentedArray* m_bitArray;
	
	std::atomic_uint m_threadIDs = 0;
	uint32_t m_readPrimes;
	uint64_t* m_prldPrimes;

	typedef std::pair<uint64_t, uint64_t> rangePair;
	//typedef CriticalSection SyncType;
	typedef std::mutex SyncType;

	typedef std::pair<SyncType*, rangePair> rangeItem;
	std::vector<rangeItem> m_ranges;

	void CalculateOptimum();
	void CalculateSimple();

	uint64_t saveAsTXTSimpleMode(uint64_t START, SegmentedArray *sarr, std::string& outputFilename, bool diffMode);
	uint64_t saveAsTXTOptimumMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename);
	uint64_t saveAsTXTDiffOptimumMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename);
	uint64_t saveAsBINSimpleMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename, bool diffMode);
	uint64_t saveAsBINOptimumMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename);
	uint64_t saveAsBINDiffOptimumMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename);
	uint64_t saveAsBINDiffVarOptimumMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename);
	uint64_t saveAsBINDiffVarSimpleMode(uint64_t start, SegmentedArray* sarr, std::string& outputFilename);
	//uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint64_t stopPrime);
	uint32_t LoadPrimesFromTXTFile(std::string& filename, uint64_t* primes, uint32_t len, uint64_t stopPrime);
	uint32_t LoadPrimesFromBINDiffVar(std::string& filename, uint64_t* primes, size_t len, uint64_t stopPrime);
	void LoadPrimes();

	std::string getOutputFilename();
	uint64_t parseOption(std::string s, char& symFactor, uint64_t& factor);
	void checkStartParam(std::string start);
	void defineRanges();
	rangeItem* getRange(uint64_t v);
	void printTime(const std::string& msg);
	//std::string millisecToStr(long long ms);
	uint64_t nextPrime();
	void FindPrimesInThreads(int numThreads);

public:
	static const uint64_t DEFAULT_START = 20ULL * FACTOR_T; // это реальные primes откуда стартуем если не передано в параметрах command line
	static const uint64_t DEFAULT_LENGTH = 10'000'000'000ULL; // длина промежутка на котором ищем простые если не передано в параметрах command line

	//EratosthenesSieve(bool mode, PRIMES_FILE_FORMATS oFileType, uint64_t start, uint64_t len, const std::string& primesInputFile = Pre_Loaded_Primes_Filename);
	EratosthenesSieve();

	void parseParams(std::string& mode, std::string& oft, std::string& start, std::string& len);
	void printUsage();
	void Calculate();

};


