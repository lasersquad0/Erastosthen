// Erastofen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <iomanip>
#include "EratosthenesSieve.h"

using namespace std;

struct MyGroupSeparator : numpunct<char>
{
    char do_thousands_sep() const override { return ' '; } // разделитель тыс€ч
    string do_grouping() const override { return "\3"; } // группировка по 3
};



int main(int argc, char* argv[])
{
    //cout << "Erastofen START" << endl;

    cout.imbue(locale(cout.getloc(), new MyGroupSeparator));

    try
    {
        EratosthenesSieve sieve;
        sieve.parseCmdLine(argc, argv);
        sieve.Calculate();

    }
    catch (invalid_cmd_option& e)
    {
        //cout << "Nothing" << endl;
    }
    catch(exception& e)
    {
        printf(e.what());
    }
    catch (...)
    {
        printf("Some exception thrown.\n");
    }
    
    //cout << "Finished\n";
}

uint64_t readLongLong(fstream& f)
{
    uint64_t res = 0;
    char b = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        f.read(&b, sizeof(char));
        res <<= 8;
        res |= (b & 0xFF);
    }
    return res;
}