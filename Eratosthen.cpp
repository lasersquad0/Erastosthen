// Erastofen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <iomanip>
#include "DefaultParser.h"
#include "HelpFormatter.h"
#include "EratosthenesSieve.h"
#include "Parameters.h"


using namespace std;

struct MyGroupSeparator : numpunct<char>
{
    char do_thousands_sep() const override { return ' '; } // ����������� �����
    string do_grouping() const override { return "\3"; } // ����������� �� 3
};

static void defineOptions(COptionsList& options)
{
    options.AddOption("s", "simple", "generate primes using simple Eratosthenes sieve mode", 3);
    options.AddOption("o", "optimum", "generate primes using optimized Eratosthenes sieve mode", 3);
    options.AddOption("p", "primesfile", string("file with primes to preload, if not specified '") + Pre_Loaded_Primes_Filename + "' file is used", 1);
    options.AddOption("t", "threads", "use specified number of threads during primes checking", 1);
    options.AddOption("h", "help", "show help", 0);
}

int main(int argc, char* argv[])
{
    //cout << "Erastofen START" << endl;
    
    cout.imbue(locale(cout.getloc(), new MyGroupSeparator));

    CDefaultParser defaultParser;
    CCommandLine cmd;
    COptionsList options;

    defineOptions(options);

    if (argc < 2)
    {
        cout << "Error: No command line arguments found." << endl;
        cout << CHelpFormatter::Format(APP_NAME, &options) << endl;

        return 1;
    }

    if (!defaultParser.Parse(&options, &cmd, argv, argc))
    {
        cout << defaultParser.GetLastError() << endl;
        cout << CHelpFormatter::Format(APP_NAME, &options) << endl;

        return 1;
    }

    if (cmd.HasOption("p"))
    {
        Parameters::PRIMES_FILENAME = cmd.GetOptionValue("p", 0, Pre_Loaded_Primes_Filename);
    }

    if (cmd.HasOption("t"))
    {
        try
        {
            Parameters::THREADS = stoi(cmd.GetOptionValue("t"));
            if ( (Parameters::THREADS == 0) || (Parameters::THREADS > 25) )
            {
                cout << "Number of threads is out of bounds (1...25). Using default value 1." << endl;
                Parameters::THREADS = 1;
            }
        }
        catch (...) 
        {
            cout << "Incorrect number of threads specified '" << cmd.GetOptionValue("t") <<"'. Using default value 1." << endl;
            Parameters::THREADS = 1;
        }
    }

    if (cmd.HasOption("h"))
    {
        cout << CHelpFormatter::Format(APP_NAME, &options) << endl;
    }

    if (cmd.HasOption("o") && cmd.HasOption("s")) // incorrect case when two options present in command line
    {
        cout << "Error: Only one of these two options -c, -s is allowed in command line." << endl;
        cout << CHelpFormatter::Format(APP_NAME, &options) << endl;

        return 1;
    }

  
    if (cmd.HasOption("o") || cmd.HasOption("s"))
    {
        try
        {
            string opt = cmd.HasOption("o") ? "o" : "s";
            
            string ftype = cmd.GetOptionValue(opt, 0);
            string start = cmd.GetOptionValue(opt, 1);
            string length = cmd.GetOptionValue(opt, 2);

            EratosthenesSieve sieve;
            sieve.parseParams(opt, ftype, start, length);
            sieve.Calculate();

        }
        catch (invalid_cmd_option& e)
        {
            cout << endl << e.what() << endl << endl;
            cout << CHelpFormatter::Format(APP_NAME, &options) << endl;
            return 1;
        }
        catch (ios_base::failure& e)
        {
            cout << endl;
            cout << e.what() << endl << endl;
            cout << "Error code: " << e.code() << endl;

        }
        catch (exception& e)
        {
            cout << endl << e.what() << endl << endl;
            cout << CHelpFormatter::Format(APP_NAME, &options) << endl;
            return 1;
        }
        catch (...)
        {
            cout << "UNKNOWN exception has thrown" << endl;
            return 1;
        }
    }
    //cout << "Finished\n";

    return 0;
}
