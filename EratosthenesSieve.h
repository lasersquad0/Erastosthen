#pragma once

#include <string>
#include "SegmentedArray.h"


#define Pre_Loaded_Primes_Filename "primes - 0-1G 2.txt"

class EratosthenesSieve
{
private:
	//static const string Pre_Loaded_Primes_Filename; //= "primes - 0-1G.txt";
	const string fSymbols = "BMGTP";
	static const uint64_t FACTOR_B = 1ULL; // B means "bytes". so the factor = 1.
	static const uint64_t FACTOR_M = 1'000'000ULL; // 'megabytes'
	static const uint64_t FACTOR_G = 1'000'000'000ULL; // 'gigabytes'
	static const uint64_t FACTOR_T = 1'000'000'000'000ULL; // terabytes
	static const uint64_t FACTOR_P = 1'000'000'000'000'000ULL; // petabytes
	char symFACTOR = 'G'; // символьное предстваление Factor дл€ формировани€ имени output файла.

	uint64_t FACTOR = FACTOR_G; // G по умолчанию по потом перепишетс€ исход€ из переданного диапазона.
	bool SIMPLE_MODE = false;
	uint32_t OUTPUT_FILE_TYPE = 1; // 1-just txt file no diff.  2-txt file with diff. 3 - bin file no diff. 4-bin file with diff. 5-bin file with variable diff

	string primesInFile;
	uint64_t real_start;  
	uint64_t real_length; // ƒлина промежутка на котором ищем простые

	void CalculateOptimum();
	void CalculateSimple();

	uint64_t saveAsTXT(uint64_t START, SegmentedArray *sarr, string outputFilename, bool simpleMode, bool diffMode);
	uint64_t saveAsBIN(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode, bool diffMode);
	uint64_t saveAsBINVar(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode);
	uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint64_t stopPrime);
	uint32_t LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint32_t len);
	
	uint32_t getArraySize();
	string getOutputFilename();
	uint64_t parseOption(string s);
	void parseMode(string s);
	void printTime(string msg);
	string millisecToStr(long long ms);

public:
	static const uint64_t DEFAULT_START = 20ULL * FACTOR_T; // это реальные primes откуда стартуем если не передано в параметрах command line
	static const uint64_t DEFAULT_LENGTH = 10'000'000'000ULL; // ƒлина промежутка на котором ищем простые. «десь всегда фиксирована (hardcoded)

	EratosthenesSieve(uint64_t strt = DEFAULT_START, uint64_t len = DEFAULT_LENGTH, string primesInputFile = Pre_Loaded_Primes_Filename) :real_start(strt), real_length(len), primesInFile(primesInputFile)
	{

	}

	void parseCmdLine(int argc, char* argv[]);
	void Calculate();

};


