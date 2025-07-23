
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <format>
#include "EratosthenesSieve.h"
#include "string_utils.h"
#include "Utils.h"


using namespace std;

/*
EratosthenesSieve::EratosthenesSieve(bool mode, PRIMES_FILE_FORMATS oFileType, uint64_t start, uint64_t len, const string& primesInputFile): EratosthenesSieve()
{
    OPTIMUM_MODE = mode;
    OUTPUT_FILE_TYPE = oFileType;
    m_realStart = start;
    m_realLength = len;
    m_primesInputFile = primesInputFile;
}
*/
EratosthenesSieve::EratosthenesSieve() // initialize to DEFAULT values
{
    OUTPUT_FILE_TYPE = txt;
    m_mode = CALC_MODE::optimized;
    m_realStart = DEFAULT_START;
    m_realLength = DEFAULT_LENGTH;
    m_bitArray = nullptr;
    m_readPrimes = 0;
    m_prldPrimes = nullptr;
    m_primesInputFile = Pre_Loaded_Primes_Filename;
};


string EratosthenesSieve::getOutputFilename()
{
    //char buf[100];
    //sprintf_s(buf, 100, "primes - %llu%c-%llu%c", m_realStart/FACTORstart, symFACTORstart, m_realLength/FACTORlen, symFACTORlen);
    
    string fname = std::format("primes - {}{}-{}{}", m_realStart / FACTORstart, symFACTORstart, m_realLength / FACTORlen, symFACTORlen);
    
    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       fname += ".txt";        break;
    case txtdiff:   fname += ".diff.txt";   break;
    case bin:       fname += ".bin";        break;
    case bindiff:   fname += ".diff.bin";   break;
    case bindiffvar:fname += ".diffvar.bin";break;
    }

    return fname;
}

void EratosthenesSieve::Calculate()
{
    switch (m_mode)
    {
    case CALC_MODE::simple   : CalculateSimple(); break;
    case CALC_MODE::optimized: CalculateOptimum();break;
    case CALC_MODE::n6n1     : Calculate6n1();    break;
    case CALC_MODE::n30x     : Calculate30nx();   break;
    default: throw std::invalid_argument(std::format("Incorrect calc mode: {}", (uint32_t)m_mode).c_str());
    }
}

#define NO_MORE_PRIMES (-1LL)

//TODO implements only one method, need to add more methods here - 6n1 30nx...
void FindPrimesThread(EratosthenesSieve* es)
{
    es->m_mutex.lock();
    uint32_t id = es->m_threadIDs.fetch_add(1);
    cout << "\rFindPrimesThread#" << id << " BEGIN" << endl;
    es->m_mutex.unlock();

    uint64_t NPrime, currPrime, rem, j;
    //uint64_t len = es->m_bitArray->end();
    uint64_t realEnd = es->m_realStart + es->m_realLength;

    while ((currPrime = es->nextPrime()) != NO_MORE_PRIMES)
    {
        //uint64_t currPrime = prldPrimes[i];

        /*uint64_t begin = es->m_realStart % currPrime;
        if (begin > 0)
            begin = es->m_realStart - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
        else
            begin = es->m_realStart;

        if ((begin & 0x01ULL) == 0) begin += currPrime; // четные числа выброшены из массива поэтому берем следующее кратное currPrime, оно будет точно нечетное

        uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
        begin = max(begin, j);

        j = (begin - 1) / 2; //нечетное переводим в индекс уплотненного массива
        */

        NPrime = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
        if (NPrime < es->m_realStart)
        {
            NPrime = es->m_realStart;
            rem = es->m_realStart % currPrime;
            if (rem > 0)
                NPrime = es->m_realStart - rem + currPrime; // calc first number inside specified range that divisible to currPrime

            if ((NPrime & 0x01ULL) == 0) NPrime += currPrime; // we do not store even numbers in bit array therefore we take next number for NPrime which is odd  
        }

        j = (NPrime - 1) / 2;
        auto range = es->getRange(j);
        if (range != nullptr)
            range->first->lock();

        while (NPrime < realEnd)
        {
            es->m_bitArray->setTrue(j);
            NPrime += 2*currPrime; // NPrime + currPrime makes NPrime even. we have to bypass cases when NPrime is even. That is why we do NPrime + 2*currPrime
            j = (NPrime - 1) / 2;

            if (j > range->second.second)
            {
                range->first->unlock();
                range = es->getRange(j);
                if(range != nullptr) // range can be null one time when j becomes bigger than len in this loop.  
                    range->first->lock(); // we jumped into next range. lock it and continue filtering primes in this range
            }
        }
    }

    es->m_mutex.lock();
    cout << endl << "CheckPrimeThread#" << id << " END"; // << endl;
    es->m_mutex.unlock();
}

uint64_t EratosthenesSieve::nextPrime()
{
    static atomic_uint index(1); // TODO may be for simple mode we need 0 here

    updateProgress(index);

    if (index < m_readPrimes)
        return m_prldPrimes[index.fetch_add(1)];

    //cout << "\r                                \r"; // cleaning up progress line of text

    return NO_MORE_PRIMES;
}

void EratosthenesSieve::FindPrimesInThreads(int numThreads)
{
    //m_mutex.lock(); // do not run threads untill all threads are created.
    
    vector<thread*> thr;
    for (size_t i = 0; i < numThreads; i++)
    {
        thread* t = new thread(FindPrimesThread, this);
        thr.push_back(t);
    }

    //m_mutex.unlock();

    for (size_t i = 0; i < numThreads; i++)
    {
        thr.at(i)->join();
    }

    for (size_t i = 0; i < numThreads; i++)
    {
        delete thr.at(i);
    }
}

void EratosthenesSieve::LoadPrimes()
{
    auto start1 = chrono::high_resolution_clock::now();

    uint64_t maxPrimeToLoad = (uint64_t)(sqrt(m_realStart + m_realLength)); // max prime num in a specified range 
    uint64_t numPrimesLessThan = (uint64_t)((double)maxPrimeToLoad / log(maxPrimeToLoad)); // approx number of prime numbers in a range 
    cout << "There are *approximately* " << numPrimesLessThan << " primes less than " << maxPrimeToLoad << " = sqrt(" << m_realStart + m_realLength << ") we need for calculations." << endl;

    uint32_t prldPrimesSize = (uint32_t)(numPrimesLessThan * 1.2); // give extra 20% for primes array size since formular above is not presize

    //cout << "Array size for primes to be read from a file: " << prldPrimesSize << endl;

    m_prldPrimes = new uint64_t[prldPrimesSize];

    // loading predefined primes that we will use for new primes calculations
    m_readPrimes = LoadPrimesFromBINDiffVar(Parameters::PRIMES_FILENAME, m_prldPrimes, prldPrimesSize, maxPrimeToLoad);
    std::cout << "Number of primes actually read from file: " << m_readPrimes << endl;

    if (m_prldPrimes[m_readPrimes - 1] < maxPrimeToLoad) // not enought primes in file, show warning 
        std::cout << "WARNING: There might be not enough primes in a file '" << Parameters::PRIMES_FILENAME << "' for calculation new primes!" << endl;

    auto stop = chrono::high_resolution_clock::now();
    cout << "Primes loaded in " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;
}

// stores bits only for 30N+X numbers which covers all prime numbers. where X is one pf the following remainders 1, 7, 11, 13, 17, 19, 23, 29
// if current number is 30N+X format then set bit in bit field to mark this number as compound
// if current number is NOT 30N+X format then do nothing and go to the next number 
// Calculate30nx works slightly faster than Optimum method AND requires 1.5 times less memory than Optimum
void EratosthenesSieve::Calculate30nx()
{
    static int BIT_TO_REM[] = { 1, 7, 11, 13, 17, 19, 23, 29 };

    static int REM_TO_BIT[] = {
        -1, 0,
            -1, -1, -1, -1, -1, 1,
            -1, -1, -1, 2,
            -1, 3,
            -1, -1, -1, 4,
            -1, 5,
            -1, -1, -1, 6,
            -1, -1, -1, -1, -1, 7,
        };

    const uint64_t START = (m_realStart) / 30ULL;
    const uint64_t LENGTH = (m_realLength + 29) / 30ULL;

    printTime("Calculating prime numbers with Eratosthenes algorithm.");
    cout << "Start: " << m_realStart << endl;
    cout << "Length: " << m_realLength << endl;

    LoadPrimes(); // loading primes from file, result - in m_prldPrimes array. 

    uint8_t* bitArray = (uint8_t*)calloc(LENGTH, sizeof(uint8_t));

    auto start1 = chrono::high_resolution_clock::now();

    cout << "Calculation started" << std::endl;

    startProgress(m_readPrimes);

    if (Parameters::THREADS > 1)
    {
        FindPrimesInThreads(Parameters::THREADS - 1); // waits until all threads are finished
    }
    else
    {
        //uint64_t len = m_bitArray->end();
        uint64_t realEnd = m_realStart + m_realLength;
        std::lldiv_t divres;
        uint64_t NPrime, rem, currPrime;
        for (uint32_t i = 1; i < m_readPrimes; ++i)  //%%%%% bypass num 2, start from 3
        {
            updateProgress(i);

            currPrime = m_prldPrimes[i];
            
            NPrime = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
            if (NPrime < m_realStart)
            {
                NPrime = m_realStart;
                rem = m_realStart % currPrime;
                if (rem > 0)
                    NPrime = m_realStart - rem + currPrime; // calc first number inside specified range that divisible to currPrime

                if ((NPrime & 0x01ULL) == 0) NPrime += currPrime; // we do not store even numbers in bit array therefore we take next number for NPrime which is odd  
            }
            
            while (NPrime < realEnd)
            {
                divres = std::div(NPrime, 30ll);

                assert(divres.quot >= START);

                int bit = REM_TO_BIT[divres.rem];
                
                if (bit >= 0)
                {
                    bitArray[divres.quot - START] |= (1ll << bit);
                }

                NPrime += 2*currPrime;  // NPrime + currPrime makes NPrime even. we have to bypass cases when NPrime is even. That is why we do NPrime + 2*currPrime
            }
        }
    }

    if (m_realStart == 0) m_bitArray->setTrue(0); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые
    if (m_realStart == 1) m_bitArray->setTrue(0); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые

    finishProgress();

    auto stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;
    cout << "Saving file..." << endl;

    auto start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0;
    string outputFilename = getOutputFilename();

    ///////////////////////////////////////////////////////////////////////////

    size_t chunk = 10'000'000;
/*
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    string s;
    s.reserve(chunk); // store to file by 10M chunks

    uint64_t cntPrimes = 0;
    uint64_t prime = 0;

    if ((START == 0) || (START == 1))
        s.append("2,"); // заглушка для сжатого массива если генерим числа начиная с 0.

    for (uint64_t i = 0; i < LENGTH; ++i)
    {
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (!(bitArray[i] & (1ll << bit)))
            {
                prime = (i + START) * 30 + BIT_TO_REM[bit];

                //sometimes prime may be out of requested range
                // add to file only primes that fit into range
                if ((prime >= m_realStart) && (prime < m_realStart + m_realLength))
                {
                    cntPrimes++;
                    s.append(to_string(prime));
                    s.append(",");
                }

                if (s.size() > chunk - 20) // when we are close to capacity but еще НЕ перепрыгнули ее
                {
                    f.write(s.c_str(), s.length());
                    s.clear(); // очищаем строку но оставляем capacity
                }
            }
        }
    }

    f.write(s.c_str(), s.length());
    f.close();
    */
    /*
    PrimeFromIndexPred pred6n1 = [](const uint64_t index)
        {
            if ((index & 0x01) == 0)
                return (index) * 3 + 1; // index is even, full formular (index/2)*6+1
            else
                return (index - 1) * 3 + 5; // index is odd, full formular ((index-1)/2)*6+5
        };

    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       primesSaved = saveAsTXTOptimumMode(START, m_bitArray, outputFilename, pred6n1); break;
    case txtdiff:   primesSaved = saveAsTXTDiffOptimumMode(START, m_bitArray, outputFilename); break;
    case bin:       primesSaved = saveAsBINOptimumMode(START, m_bitArray, outputFilename); break;
    case bindiff:   primesSaved = saveAsBINDiffOptimumMode(START, m_bitArray, outputFilename); break;
    case bindiffvar:primesSaved = saveAsBINDiffVarOptimumMode(START, m_bitArray, outputFilename); break;
    }
    */
    //primesSaved = cntPrimes;
    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    cout << "Total: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    delete[] m_prldPrimes;
    free(bitArray);
}

// stores bits only for 6n+1 and 6n+5 numbers which covers all prime numbers
// if current number is 6n+1 or 6n+5 then set bit in bit field to mark this number as compound
// if current number is NOT 6n+1 or 6n+5 then do nothing and go to the next number 
// Calculate6n1 works about two times slower than Optimum method BUT requires 1.5 times less memory than Optimum
void EratosthenesSieve::Calculate6n1()
{
    uint64_t START = (m_realStart) / 3ULL; 
    uint64_t END = (m_realStart + m_realLength) / 3ULL + 1;

    printTime("Calculating prime numbers with Eratosthenes algorithm.");
    cout << "Start: " << m_realStart << endl;
    cout << "Length: " << m_realLength << endl;

    LoadPrimes(); // loading primes from file, result - in m_prldPrimes array. 

    m_bitArray = new SegmentedArray(START, END);

    auto start1 = chrono::high_resolution_clock::now();

    cout << "Calculation started" << std::endl;

    startProgress(m_readPrimes);

    if (Parameters::THREADS > 1)
    {
        FindPrimesInThreads(Parameters::THREADS); // waits until all threads are finished
    }
    else
    {
       // uint64_t len = m_bitArray->end();
        uint64_t NPrime, rem, currPrime;
        uint64_t realEnd = m_realStart + m_realLength;
        std::lldiv_t divres;
        for (uint32_t i = 1; i < m_readPrimes; ++i)  //%%%%% пропускаем число 2 начинаем с 3
        {
            updateProgress(i);

            currPrime = m_prldPrimes[i];

            NPrime = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
            if (NPrime < m_realStart)
            {
                NPrime = m_realStart;
                rem = m_realStart % currPrime;
                if (rem > 0)
                    NPrime = m_realStart - rem + currPrime; // calc first number inside specified range that divisible to currPrime

                if ((NPrime & 0x01ULL) == 0) NPrime += currPrime; // we do not store even numbers in bit array therefore we take next number for NPrime which is odd  
            }

            //NPrime must be odd before starting this loop
            while (NPrime < realEnd)
            {
                divres = std::div(NPrime, 6ll);
                if (divres.rem == 1)
                {
                    m_bitArray->setTrue(divres.quot << 1);
                }
                else if (divres.rem == 5)
                {
                    m_bitArray->setTrue((divres.quot << 1) + 1);
                }

                NPrime += 2*currPrime; // NPrime + currPrime makes NPrime even. we have to bypass cases when NPrime is even. That is why we do NPrime + 2*currPrime
            }
        }
    }

    if (m_realStart == 0) m_bitArray->setTrue(0); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые
    if (m_realStart == 1) m_bitArray->setTrue(0); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые

    finishProgress();

    auto stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;
    cout << "Saving file..." << endl;

    auto start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0;
    string outputFilename = getOutputFilename();

    PrimeFromIndexPred pred6n1 = [](const uint64_t index)
        {
            if ((index & 0x01) == 0)
                return (index) * 3 + 1; // index is even, full formular (index/2)*6+1
            else
                return (index - 1) * 3 + 5; // index is odd, full formular ((index-1)/2)*6+5
        };

    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       primesSaved = saveAsTXTOptimumMode(START, m_bitArray, outputFilename, pred6n1); break;
    case txtdiff:   primesSaved = saveAsTXTDiffOptimumMode(START, m_bitArray, outputFilename, pred6n1); break;
    case bin:       primesSaved = saveAsBINOptimumMode(START, m_bitArray, outputFilename, pred6n1); break;
    case bindiff:   primesSaved = saveAsBINDiffOptimumMode(START, m_bitArray, outputFilename, pred6n1); break;
    case bindiffvar:primesSaved = saveAsBINDiffVarOptimumMode(START, m_bitArray, outputFilename, pred6n1); break;
    }

    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    cout << "Total: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    delete[] m_prldPrimes;
    delete m_bitArray;
}

// stores only odd numbers in memory because all even numbers are not primes by default
// uses two times less memory than Simple method
void EratosthenesSieve::CalculateOptimum()
{
    //because bit array contains only odd numbers m_realStart must be odd too
    if ((m_realStart & 0x01ULL) == 0) m_realStart++; // making m_realStart odd
    uint64_t START = (m_realStart - 1ULL) / 2ULL; // Переводим в индекс в уплотненном массиве. Не храним четные элементы так как они делятся на 2 по умолчанию.
    uint64_t END = (m_realStart + m_realLength) / 2ULL; // переводим в индекс

    printTime("Calculating prime numbers with Eratosthenes algorithm.");
    cout << "Start: " << m_realStart << endl;
    cout << "Length: " << m_realLength << endl;

    LoadPrimes(); // loading primes from file, result - in m_prldPrimes array. 

    m_bitArray = new SegmentedArray(START, END);

    auto start1 = chrono::high_resolution_clock::now();

    cout << "Calculation started" << std::endl;

    startProgress(m_readPrimes);

    if (Parameters::THREADS > 1)
    {
        FindPrimesInThreads(Parameters::THREADS - 1); // waits until all threads are finished
    }
    else
    {
        //uint64_t len = m_bitArray->end();
        uint64_t begin;
        uint64_t rem;
        uint64_t realEnd = m_realStart + m_realLength;
        for (uint32_t i = 1; i < m_readPrimes; ++i)  //%%%%% bypass number 2 and start from 3
        {
            updateProgress(i);

            uint64_t currPrime = m_prldPrimes[i];

            begin = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс            
            if (begin < m_realStart)
            {
                begin = m_realStart;
                rem = m_realStart % currPrime;
                if (rem > 0)
                    begin = m_realStart - rem + currPrime; // calc first number inside specified range that divisible to currPrime
              
                if ((begin & 0x01ULL) == 0) begin += currPrime; // четные числа выброшены из массива поэтому берем следующее кратное currPrime, оно будет точно нечетное            
            }
            
            
            while (begin < realEnd)
            {
                //if ((begin & 0x01ull)) // bypass even numbers in begin
                //{
                    // begin is always odd, transfer it to index of "packed" array
                    m_bitArray->setTrue((begin - 1) / 2);
                //}
                begin += 2*currPrime; // begin + currPrime makes begin even. we have to bypass cases when even begin is even. That is why we do begin + 2*currPrime
            }
        }
    }

    if (m_realStart == 0) m_bitArray->setTrue(0); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые
    if (m_realStart == 1) m_bitArray->setTrue(0); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые

    finishProgress();

    auto stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;
    cout << "Saving file..." << endl;

    auto start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0; 
    string outputFilename = getOutputFilename();

    PrimeFromIndexPred pred = [](const uint64_t index)
        {
            return index * 2 + 1;
        };

    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       primesSaved = saveAsTXTOptimumMode(START, m_bitArray, outputFilename, pred);break;
    case txtdiff:   primesSaved = saveAsTXTDiffOptimumMode(START, m_bitArray, outputFilename, pred);break;
    case bin:       primesSaved = saveAsBINOptimumMode(START, m_bitArray, outputFilename, pred);break;
    case bindiff:   primesSaved = saveAsBINDiffOptimumMode(START, m_bitArray, outputFilename, pred);break;
    case bindiffvar:primesSaved = saveAsBINDiffVarOptimumMode(START, m_bitArray, outputFilename, pred);break;
    }
    
    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    cout << "Total: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    delete[] m_prldPrimes;
    delete m_bitArray;
}

void EratosthenesSieve::CalculateSimple()
{
    auto start1 = chrono::high_resolution_clock::now();

    printTime("Calculating prime numbers with Eratosthenes algorithm.");
    cout << "Start: " << m_realStart << endl;
    cout << "Length: " << m_realLength << endl;

    LoadPrimes(); // loading primes from file, result - in m_prldPrimes array. 

    m_bitArray = new SegmentedArray(m_realStart, m_realStart + m_realLength);

    start1 = chrono::high_resolution_clock::now();

    std::cout << "Calculation started." << std::endl;

    startProgress(m_readPrimes);

    uint64_t len = m_bitArray->end();
    for (uint32_t i = 0; i < m_readPrimes; ++i) 
       {
           updateProgress(i);

           uint64_t currPrime = m_prldPrimes[i];

           uint64_t begin = m_realStart % currPrime;
           if (begin > 0)
               begin = m_realStart - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
           else
               begin = m_realStart;

           uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
           j = max(begin, j);

           while (j < len)
           {
               m_bitArray->setTrue(j);
               j += currPrime; 
           }
       }
       

    if (m_realStart == 0) m_bitArray->setTrue(0), m_bitArray->setTrue(1); // заглушка если генерим числа начиная с 0. что бы '0' не попало в простые
    if (m_realStart == 1) m_bitArray->setTrue(1); // заглушка если генерим числа начиная с 1. что бы '1' не попало в простые
    
    finishProgress();
    //cout << endl;

    auto stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;
    cout << "Saving file..." << endl;

    auto start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0;
    string outputFilename = getOutputFilename();

    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       primesSaved = saveAsTXTSimpleMode(m_realStart, m_bitArray, outputFilename, false);break;
    case txtdiff:   primesSaved = saveAsTXTSimpleMode(m_realStart, m_bitArray, outputFilename, true); break;
    case bin:       primesSaved = saveAsBINSimpleMode(m_realStart, m_bitArray, outputFilename, false);break;
    case bindiff:   primesSaved = saveAsBINSimpleMode(m_realStart, m_bitArray, outputFilename, true); break;
    case bindiffvar:primesSaved = saveAsBINDiffVarSimpleMode(m_realStart, m_bitArray, outputFilename);break;
    }

    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved in: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;


    cout << "Total: " << MillisecToStr<string>(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    delete[] m_prldPrimes;
    delete m_bitArray;
}

uint64_t EratosthenesSieve::saveAsTXTSimpleMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, bool diffMode)
{
    uint32_t chunk = 10'000'000;

    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);
    
    string s;
    s.reserve(chunk);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    //uint64_t prime = 0;

    for (uint64_t i = start; i < sarr->end(); ++i)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;
            
            if (diffMode)
            {
                s.append(to_string(i - lastPrime));
                lastPrime = i;
            }
            else
            {
                s.append(to_string(i));
            }
           
            s.append(",");

            if (s.size() > chunk - 20) // when we are close to capacity but еще НЕ перепрыгнули ее
            {
                f.write(s.c_str(), s.length()); 
               // f.flush();
                s.clear(); // очищаем строку но оставляем capacity
            }
        }
    }

    f.write(s.c_str(), s.length());
    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsTXTDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, PrimeFromIndexPred pred)
{
    uint32_t chunk = 10'000'000;

    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    string s;
    s.reserve(chunk);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    uint64_t prime = 0;

    if( ( start == 0) || (start == 1) )
    {
        s.append("2,"); // заглушка для сжатого массива если генерим числа начиная с 0.
        lastPrime = 2;
    }

    for (uint64_t i = start; i < sarr->end(); ++i)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;
            prime = pred(i); // 2 * i + 1;

            s.append(to_string(prime - lastPrime));
            s.append(",");
            lastPrime = prime;

            if (s.size() > chunk - 20) // when we are close to capacity but еще НЕ перепрыгнули ее
            {
                f.write(s.c_str(), s.length());
                // f.flush();
                s.clear(); // очищаем строку но оставляем capacity
            }
        }
    }

    f.write(s.c_str(), s.length());
    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsTXTOptimumMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, PrimeFromIndexPred pred)
{
    size_t chunk = 10'000'000;

    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    string s;
    s.reserve(chunk); // store to file by 10M chunks

    uint64_t cntPrimes = 0;
    uint64_t prime = 0;

    if ((start == 0) || (start == 1))
        s.append("2,"); // заглушка для сжатого массива если генерим числа начиная с 0.
 
    for (uint64_t i = start; i < sarr->end(); ++i)
    {
        if (!sarr->get(i))
        {
            prime = pred(i); //2 * i + 1;

            //sometimes prime may be out of requested range
            // add to file only primes that fit into range
            if ((prime >= m_realStart) && (prime < m_realStart + m_realLength))
            {
                cntPrimes++;
                s.append(to_string(prime));
                s.append(",");
            }

            if (s.size() > chunk - 20) // when we are close to capacity but еще НЕ перепрыгнули ее
            {
                f.write(s.c_str(), s.length());
                // f.flush();
                s.clear(); // очищаем строку но оставляем capacity
            }
        }
    }

    f.write(s.c_str(), s.length());
    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINSimpleMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, bool diffMode)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t prime;
    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;

    for (uint64_t i = start; i < sarr->end(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;

            prime = i;

            if (diffMode)
            {
                if (lastPrime == 0)
                {
                    f.write((char*)&prime, sizeof uint64_t); // save as 8 byte value (full number)
                }
                else
                {
                    uint16_t diff = (uint16_t)(prime - lastPrime);
                    f.write((char*)&diff, sizeof(uint16_t));  // save a 2 bytes value (difference)
                }

                lastPrime = prime;
            }
            else
                f.write((char*)&prime, sizeof(uint64_t)); // save as 8 byte value (full number)
        }
    }

    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINOptimumMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, PrimeFromIndexPred pred)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t prime;
    uint64_t cntPrimes = 0;

    if ((start == 0) || (start == 1))
        prime = 2, f.write((char*)&prime, sizeof(uint64_t)); // заглушка для сжатого массива если генерим числа начиная с 0.

    for (uint64_t i = start; i < sarr->end(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;            
            prime = pred(i); // 2 * i + 1;
            f.write((char*)&prime, sizeof(uint64_t)); 
        }
    }

    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, PrimeFromIndexPred pred)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t prime;
    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;

    if ((start == 0) || (start == 1))
        prime = 2, lastPrime = 2, f.write((char*)&prime, sizeof(uint64_t)); // заглушка для сжатого массива если генерим числа начиная с 0.


    for (uint64_t i = start; i < sarr->end(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;

            prime = pred(i); //2 * i + 1;

            if (lastPrime == 0)
            {
                f.write((char*)&prime, sizeof(uint64_t)); // save as 8 byte value (full number)
            }
            else
            {
                uint16_t diff = (uint16_t)(prime - lastPrime);
                f.write((char*)&diff, sizeof(uint16_t));  // save a 2 bytes value (difference)
            }

            lastPrime = prime;
        }
    }

    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINDiffVarOptimumMode(uint64_t start, SegmentedArray* sarr, string& outputFilename, PrimeFromIndexPred pred)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t lastPrime = 0;
    uint64_t prime;

    const uint64_t BUF_LEN = 100'000'000; // записываем в файл блоками по 100М
    uint8_t* buf = new uint8_t[BUF_LEN];

    if ( (start == 0) || (start == 1) ) // заглушка для сжатого массива если генерим числа начиная с 0.
    {
        prime = 2;
        size_t len = var_len_encode(buf, prime);
        f.write((char*)buf, len);
        lastPrime = prime;
    }

    uint64_t cntPrimes = 0;
    uint64_t offset = 0;

    for (uint64_t i = start; i < sarr->end(); i++)
    {
        if (!sarr->get(i))
        {
            prime = pred(i); // (i << 1) + 1; //2 * i + 1;

            // sometimes prime may be out of requested range
            // save to file only primes that fit into range
            if ((prime < m_realStart) || (prime >= m_realStart + m_realLength))
                continue;

            cntPrimes++;

            uint64_t diff;
            if (lastPrime == 0)
            {
                diff = prime;
            }
            else
            {
                diff = prime - lastPrime;
                assert((diff & 0x01ULL) == 0ULL); // check that diff is even

                if (lastPrime > 2) diff >>= 1; // разница всегда четная. поэтому можем хранить половину значения. больше значений уместится в 1 байт.
            }

            size_t len = var_len_encode(buf + offset, diff);            
            assert(len > 0);
            
            offset += len;

            if (offset > BUF_LEN - 3) // encoded len always less than 2^24 (max number that fits into 3 bytes)
            {
                f.write((char*)buf, offset);
                offset = 0;
            }

            lastPrime = prime;
        }
    }

    f.write((char*)buf, offset);

    f.close();

    delete[] buf;

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINDiffVarSimpleMode(uint64_t start, SegmentedArray* sarr, string& outputFilename)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    uint8_t buf[9];

    for (uint64_t i = start; i < sarr->end(); i++)
    {
        if (!sarr->get(i))
        {        
            cntPrimes++;

            uint64_t diff = (i - lastPrime); // разница всегда четная. поэтому можем хранить половину значения. больше значений уместится в 1 байт.
            assert((diff & 0x01ULL) == 0ULL); // check that diff is even

            if (lastPrime > 2) diff /= 2;

            size_t len = var_len_encode(buf, diff);
            f.write((char*)buf, len);

            lastPrime = i;
        }
    }

    f.close();

    return cntPrimes;
}

//Загружаем либо весь файл, либо up to len, либо до stopPrime простых чисел из файла 
uint32_t EratosthenesSieve::LoadPrimesFromBINDiffVar(string& filename, uint64_t* primes, size_t len, uint64_t stopPrime)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.open(filename, ios::in | ios::binary);
    f.exceptions(ifstream::badbit);

    uint64_t diff;
    uint64_t lastPrime = 0;
    uint8_t buf[9];
    size_t maxSize;

    uint32_t cnt = 0;
    while (cnt < len)
    {
        maxSize = 0;

        while(true)
        {
            f.read((char*)(buf + maxSize), 1);
            if (f.eof()) break;
            if ((buf[maxSize++] & 0x80) == 0) break;
        }

        if (f.eof()) break;

        size_t res = var_len_decode(buf, maxSize, &diff);

        assert(res > 0);

        if (lastPrime > 2) diff *= 2; // умножаем на 2 все: кроме первого числа(lastPrime==0), и когда lastPrime==2

        primes[cnt] = diff + lastPrime;
        lastPrime = primes[cnt++];

        if (lastPrime > stopPrime) break;
    }

    return cnt;
}

//Загружаем либо весь файл, либо up to len, либо до stopPrime простых чисел из файла 
uint32_t EratosthenesSieve::LoadPrimesFromTXTFile(string& filename, uint64_t* primes, uint32_t len, uint64_t stopPrime)
{
    fstream f;
    f.exceptions(/*ifstream::failbit |*/ ifstream::badbit);
    f.open(filename, ios::in);

    string line;

    uint32_t cnt = 0;
    while (cnt < len)
    {
        getline(f, line, ',');
        if (f.eof()) break;

        primes[cnt] = atoll(line.c_str());

        if (primes[cnt] > stopPrime) break;
        cnt++;
    }

    f.close();

    return cnt;
}

void EratosthenesSieve::printUsage()
{
    cout << endl << "Usage:" << endl;
    cout << APP_EXE_NAME << " -<option> <outputformat> <start> <length>" << endl;
    cout << "   <option> - either symbol 'c' (means compressed alg and needs less memory) or 's' ( simple and needs more memory)" << endl;
    cout << "   <outputformat> - String that specifies output file format with generated prime numbers. Possible values: txt, txtdiff, bin, bindiff, bindiffvar" << endl;
    cout << "   <start> - starting value for generating primes. Accepts modificators B (bytes), K (kilo), M (mega), G (giga), T (tera), P (peta) for big values" << endl;
    cout << "   <length> - length of a range. Primes are generated in a range: start...start+length. Accepts modificators B,K,M,G,T,P" << endl;
    cout << "All three arguments are required." << endl;

    cout << endl<< "Examples:" << endl;
    cout << APP_EXE_NAME << " -s txt 0g 10g" << endl;
    cout << APP_EXE_NAME << " -o bin 900G 100G" << endl;
    cout << APP_EXE_NAME << " -o txtdiff 100T 10G" << endl;
    cout << APP_EXE_NAME << " -s bindiff 5T 50G" << endl;
    cout << APP_EXE_NAME << " -o bindiffvar 20P 1G" << endl;
    cout << APP_EXE_NAME << " -t 5 -o txt 1000P 100M" << endl;
    cout << APP_EXE_NAME << " -o 1000B 1G" << endl;
    cout << APP_EXE_NAME << " -o 0M 100M" << endl;
}


void EratosthenesSieve::parseParams(string& mode, string& oft, string& start, string& len)
{
    TrimAndUpper(mode);

    m_mode = STR_TO_CALCMODE(mode);
    if (m_mode == CALC_MODE::none)
        throw invalid_argument("Error: Unsupported calculation mode specified.");

    checkStartParam(start); // generates an exception is anything wrong with start param value
    m_realStart = parseOption(start, symFACTORstart, FACTORstart);

    // no need to call checkStartParam() for len, see below
    m_realLength = parseOption(len, symFACTORlen, FACTORlen); 
    
    if (m_realLength > MAX_LEN)
        throw invalid_argument("Length argument is out of bounds (1...1000G)");

    TrimAndUpper(oft);

    OUTPUT_FILE_TYPE = STR_TO_FORMAT(oft);
    if (OUTPUT_FILE_TYPE == none)
        throw invalid_argument("Error: Unsupported output file type specified.");

    defineRanges();
}

// opt passed by value intentionally
// used only for checking validity of START parameter
void EratosthenesSieve::checkStartParam(string start)
{
    const string ErrStr = std::format("Error: Start parameter value should be a number with factor symbol at the end (factor symbol is a single letter from list: {}). Current incorrect Start value: {}\n", fSymbols, start);
    
    // + remove any leading and traling spaces, just in case.
    TrimAndUpper(start);
    
    char sFactor = start.at(start.length() - 1); 

    if (fSymbols.find(sFactor) == string::npos)
        throw invalid_argument(ErrStr);

    start = start.substr(0, start.length() - 1); // remove letter G at the end. Or T, or P, or M or K or B.

    uint64_t dStart;
    try
    {
        dStart = stoull(start);
    }
    catch (...)
    {
        throw invalid_argument("Error: Start parameter value should be a number: '" + start + "'.\n");
    }

    // used below in switch()
    invalid_argument e("Start parameter value is out of bounds (0..18'400P)");
   
    switch (sFactor)
    {
    case 'B': break;
    case 'K': if (dStart > MAX_START_K) throw e; break;
    case 'M': if (dStart > MAX_START_M) throw e; break;
    case 'G': if (dStart > MAX_START_G) throw e; break;
    case 'T': if (dStart > MAX_START_T) throw e; break;
    case 'P': if (dStart > MAX_START_P) throw e; break;
    default:
        throw invalid_argument(ErrStr);
    }
}

// parses one value either m_realStart or m_realLength in format with Factor multiplier: B, K, M, G, T, P
// examples: 100G, 5T, 20000G, 400M, 0B
// returns back symFactor and factor values, and real calculated value as uint64 as a result of function call 
uint64_t EratosthenesSieve::parseOption(string opt, char& symFactor, uint64_t& factor)
{
    // remove any leading and traling spaces, just in case.
    TrimAndUpper(opt);

    char sFactor = opt.at(opt.length() - 1); // we need symFACTOR later for output filename

    if (fSymbols.find(sFactor) == string::npos)
        throw invalid_argument("Error: Incorrect command line parameter '" + opt + "'.\n");

    opt = opt.substr(0, opt.length() - 1); // remove letter G at the end. Or T, or P, or M, or K or B.

    uint64_t dStart;
    try
    {
        dStart = stoull(opt);
    }
    catch (...)
    {
        throw invalid_argument("Error: Parameter value should be a number: '" + opt + "'.\n");
    }

    uint64_t ffactor = 0;
    switch (sFactor)
    {
    case 'B': ffactor = FACTOR_B; break;
    case 'K': ffactor = FACTOR_K; break;
    case 'M': ffactor = FACTOR_M; break;
    case 'G': ffactor = FACTOR_G; break;
    case 'T': ffactor = FACTOR_T; break;
    case 'P': ffactor = FACTOR_P; break;
    default:
        throw invalid_argument("Error: Param value should be a number with factor (one letter from list: BKMGTP). Factor is incorrect here: '" + opt + "'.\n");
    }

    symFactor = sFactor;
    factor = ffactor;

    return dStart * ffactor;
}

void EratosthenesSieve::defineRanges()
{
    //because bit array contains only odd numbers m_realStart must be odd too
    if ((m_realStart & 0x01ULL) == 0) m_realStart++; // making m_realStart odd
    uint64_t START = (m_realStart - 1ULL) / 2ULL; // Переводим в индекс в уплотненном массиве. Не храним четные элементы так как они делятся на 2 по умолчанию.
    uint64_t LENGTH = m_realLength / 2ULL; // переводим в индекс
    int workingThreads = Parameters::THREADS - 1;

    if (workingThreads > 1)
    { 
        uint64_t k = 1;
        uint64_t rangesCount = workingThreads * k;
        uint64_t rangeLen = LENGTH / rangesCount;

        m_ranges.push_back(rangeItem(new SyncType(), rangePair(START, START + rangeLen - 1))); //TODO this needs a call delete somewhere to free memory

        uint64_t currRange = rangeLen;

        for (size_t i = 1; i < rangesCount - 1; i++)
        {
            m_ranges.push_back(rangeItem(new SyncType(), rangePair(START + currRange, START + currRange + rangeLen - 1)));
            currRange += rangeLen;
        }

        m_ranges.push_back(rangeItem(new SyncType(), rangePair(START + currRange, START + LENGTH - 1)));
    }
}

EratosthenesSieve::rangeItem* EratosthenesSieve::getRange(uint64_t v)
{
    for (size_t i = 0; i < m_ranges.size(); i++)
    {
        rangePair& p = m_ranges[i].second;
        if ((v >= p.first) && (v <= p.second))
            return &m_ranges[i];
    }
    return nullptr;
}

static struct tm time_t_to_tm(const time_t t)
{
    struct tm ttm;
    localtime_s(&ttm, &t);
    return ttm;
}

static struct tm GetCurrDateTime()
{
    const std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return time_t_to_tm(tt);
}

#define DATETIME_BUF 100

// retrieves current datetime as string
std::string GetCurrDateTimeAsString(void)
{
    struct tm t = GetCurrDateTime();
    char ss[DATETIME_BUF];
    std::strftime(ss, DATETIME_BUF, "%c", &t);

    return ss;
}

void EratosthenesSieve::printTime(const string& msg)
{
    std::cout << GetCurrDateTimeAsString();
    std::cout << ' ' << msg << std::endl;
}
/*
void EratosthenesSieve::printTime(string msg)
{
    const auto now = chrono::system_clock::now();
    const time_t t_c = chrono::system_clock::to_time_t(now);

#pragma warning(suppress : 4996)
    cout << ctime(&t_c) << msg << endl;

    //time_t t = time(0);
    //tm* local = localtime(&t);
    //cout << put_time(local, "%F") << std::endl;
    //cout << put_time(local, "%T") << std::endl;
    

    //    LocalTime tm = LocalTime.now();
    //    System.out.format("Datetime: %tH:%tM:%tS:%tL\n", tm, tm, tm, tm);
    //    System.out.println(msg);
}

string EratosthenesSieve::millisecToStr(long long ms)
{
    int milliseconds = ms % 1000;
    int seconds = (ms / 1000) % 60;
    int minutes = (ms / 60000) % 60;
    int hours = (ms / 3600000) % 24;

    char buf[100];
    if (hours > 0)
        sprintf_s(buf, 100, "%u hours %u minutes %u seconds %u ms", hours, minutes, seconds, milliseconds);
    else if (minutes > 0)
        sprintf_s(buf, 100, "%u minutes %u seconds %u ms", minutes, seconds, milliseconds);
    else
        sprintf_s(buf, 100, "%u seconds %u ms", seconds, milliseconds);

    return buf;
}*/
