#pragma once

#include <string>


#define Pre_Loaded_Primes_Filename "primes - 0-1G.diffvar.bin"
#define APP_NAME "Eratosthen"
#define APP_EXE_NAME APP_NAME ".exe"
#define PERCENT_FACTOR 100

class Parameters
{
public:
	static int THREADS;
	static std::string PRIMES_FILENAME;// = Pre_Loaded_Primes_Filename;
};




