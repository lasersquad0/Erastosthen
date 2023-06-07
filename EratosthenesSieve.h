#pragma once

#include <string>
#include "PrimesFIO.h"
#include "SegmentedArray.h"


#define Pre_Loaded_Primes_Filename "primes - 0-1G.diffvar.bin"
#define APP_NAME "Eratosthen"
#define APP_EXE_NAME APP_NAME".exe"

class invalid_cmd_option: public invalid_argument
{
public:
	invalid_cmd_option(string str) :invalid_argument(str)
	{
	}
};

class EratosthenesSieve
{
private:
	const string fSymbols = "BMGTP";
	static const uint64_t FACTOR_B = 1ULL; // B means "bytes". so the factor = 1.
	static const uint64_t FACTOR_M = 1'000'000ULL; // 'megabytes'
	static const uint64_t FACTOR_G = 1'000'000'000ULL; // 'gigabytes'
	static const uint64_t FACTOR_T = 1'000'000'000'000ULL; // terabytes
	static const uint64_t FACTOR_P = 1'000'000'000'000'000ULL; // petabytes
	
	char symFACTORstart = 'G'; // символьное представление Factor дл€ значени€ real_start дл€ формировани€ имени output файла.
	char symFACTORlen = 'G'; // символьное представление Factor дл€ значени€ real_length дл€ формировани€ имени output файла.

	uint64_t FACTORstart = FACTOR_G; // G по умолчанию по потом перепишетс€ исход€ из переданного диапазона.
	uint64_t FACTORlen = FACTOR_G;
	bool OPTIMUM_MODE = true;
	PRIMES_FILE_FORMATS OUTPUT_FILE_TYPE; // 1-just txt file no diff.  2-txt file with diff. 3 - bin file no diff. 4-bin file with diff. 5-bin file with variable diff

	string primesInFile;
	uint64_t real_start;  
	uint64_t real_length; // ƒлина промежутка на котором ищем простые

	void CalculateOptimum();
	void CalculateSimple();

	uint64_t saveAsTXT(uint64_t START, SegmentedArray *sarr, string outputFilename, bool simpleMode, bool diffMode);
	uint64_t saveAsTXTOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsTXTDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBIN(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode, bool diffMode);
	uint64_t saveAsBINOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBINDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename);
	uint64_t saveAsBINDiffVar(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode);
	//uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint64_t stopPrime);
	uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint32_t len, uint64_t stopPrime);
	uint32_t LoadPrimesFromBINDiffVar(string filename, uint64_t* primes, size_t len, uint64_t stopPrime);
	
	//uint32_t getArraySize();
	string getOutputFilename();
	uint64_t parseOption(string s, char* symFactor, uint64_t* factor);
	//void parseMode(string s);
	void printTime(string msg);
	string millisecToStr(long long ms);

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


