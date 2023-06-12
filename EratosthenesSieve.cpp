#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include "EratosthenesSieve.h"


using namespace std;

size_t var_len_encode(uint8_t buf[9], uint64_t num)
{
    if (num > UINT64_MAX / 2)
        return 0;

    size_t i = 0;

    while (num >= 0x80)
    {
        buf[i++] = (uint8_t)(num) | 0x80;
        num >>= 7;
    }

    buf[i++] = (uint8_t)(num);

    return i;
}

size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t* num)
{
    if (size_max == 0)
        return 0;

    if (size_max > 9)
        size_max = 9;

    *num = buf[0] & 0x7F;
    size_t i = 0;

    while (buf[i++] & 0x80)
    {
        if (i >= size_max || buf[i] == 0x00)
            return 0;

        *num |= (uint64_t)(buf[i] & 0x7F) << (i * 7);
    }

    return i;
}

void trimStr(string& str)
{
    // remove any leading and traling spaces, just in case.
    size_t strBegin = str.find_first_not_of(' ');
    size_t strEnd = str.find_last_not_of(' ');
    str.erase(strEnd + 1, str.size() - strEnd);
    str.erase(0, strBegin);
}

void trimAndUpper(string& str)
{
    // remove any leading and traling spaces, just in case.
    size_t strBegin = str.find_first_not_of(' ');
    size_t strEnd = str.find_last_not_of(' ');
    str.erase(strEnd + 1, str.size() - strEnd);
    str.erase(0, strBegin);

    // to uppercase
    transform(str.begin(), str.end(), str.begin(), ::toupper);
}

EratosthenesSieve::EratosthenesSieve(bool mode, PRIMES_FILE_FORMATS oFileType, uint64_t strt, uint64_t len, string primesInputFile): EratosthenesSieve()
{
    OPTIMUM_MODE = mode;
    OUTPUT_FILE_TYPE = oFileType;
    m_realStart = strt;
    m_realLength = len;
}

EratosthenesSieve::EratosthenesSieve() // initialize to DEFAULT values
{
    OPTIMUM_MODE = true;
    OUTPUT_FILE_TYPE = txt;
    m_realStart = DEFAULT_START;
    m_realLength = DEFAULT_LENGTH;
    m_bitArray = nullptr;
    m_delta = 0;
    m_readPrimes = 0;
    m_prldPrimes = nullptr;
};


string EratosthenesSieve::getOutputFilename()
{
    char buf[100];
    sprintf_s(buf, 100, "primes - %llu%c-%llu%c", m_realStart/FACTORstart, symFACTORstart, m_realLength/FACTORlen, symFACTORlen);
    
    string fname = buf;
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
    if (OPTIMUM_MODE)
        CalculateOptimum();
    else
        CalculateSimple();
}

#define NO_MORE_PRIMES (-1)

void FindPrimesThread(EratosthenesSieve* es)
{
    es->m_mutex.lock();
    uint32_t id = es->m_threadIDs.fetch_add(1);
    cout << "FindPrimesThread#" << id << " BEGIN" << endl;
    es->m_mutex.unlock();

    uint64_t currPrime;
    uint64_t len = es->m_bitArray->size();

    while ((currPrime = es->nextPrime()) != NO_MORE_PRIMES)
    {
        /*if (i > percentCounter)
        {
            printf("\rProgress...%u%%", percent++);
            percentCounter += delta;
        }*/

        //uint64_t currPrime = prldPrimes[i];

        uint64_t begin = es->m_realStart % currPrime;
        //long begin = (REAL_START/currPrime)*currPrime;
        //if(begin < REAL_START) begin += currPrime;
        if (begin > 0)
            begin = es->m_realStart - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
        else
            begin = es->m_realStart;

        if (begin % 2 == 0) begin += currPrime; // TODO может быстрее (begin&0x01)==0 четные числа выброшены из массива поэтому берем следующее кратное currPrime, оно будет точно нечетное

        uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
        begin = max(begin, j);

        j = (begin - 1) / 2; //нечетное переводим в индекс уплотненного массива

        auto range = es->getRange(j);
        if (range != nullptr)
            range->first->lock();

        while (j < len)
        {
            es->m_bitArray->set(j, true);
            j += currPrime; // здесь должен быть именно currPrime а не primeIndex
            if (j > range->second.second)
            {
                range->first->unlock();
                range = es->getRange(j);
                if(range != nullptr) // range can be null one time when j becomes bigger than len in this loop.  
                    range->first->lock();
            }
        }
    }

    es->m_mutex.lock();
    cout << "CheckPrimeThread#" << id << " END" << endl;
    es->m_mutex.unlock();
}

uint64_t EratosthenesSieve::nextPrime()
{
    //const lock_guard<mutex> lock(m_mutex);
    static uint64_t percent(0);
    static atomic_uint index(1);

    if (index > percent)
    {
        //cout << "\rProgress ... " << setw(7) << setprecision(4) << (float)(100 * percent) / (float)m_readPrimes /*100 * (float)index / m_loadedPrimes*/ << "%";
        cout << "\rProgress ... " << percent/m_readPrimes << "%      ";
        percent += m_delta;
    }

    if (index < m_readPrimes)
        return m_prldPrimes[index.fetch_add(1)];
    

    cout << "\r                                \r"; // cleaning up progress line of text

    return NO_MORE_PRIMES;
}

void EratosthenesSieve::FindPrimesInThreads(int numThreads)
{
    vector<thread*> thr;

    for (size_t i = 0; i < numThreads; i++)
    {
        thread* t = new thread(FindPrimesThread, this);
        thr.push_back(t);
    }

    for (size_t i = 0; i < numThreads; i++)
    {
        thr.at(i)->join();
    }

    for (size_t i = 0; i < numThreads; i++)
    {
        delete thr.at(i);
    }
}

void EratosthenesSieve::CalculateOptimum()
{
    auto start1 = chrono::high_resolution_clock::now();

    uint64_t START = (m_realStart) / 2ULL; // Переводим в индекс в уплотненном массиве. Не храним четные элементы так как они делятся на 2 по умолчанию.
    uint64_t LENGTH = (m_realLength) / 2ULL; // переводим в индекс

    printf("Calculating prime numbers with Eratosfen algorithm.\n");
    cout << "Start: " << m_realStart << endl;
    cout << "Length: " << m_realLength << endl;

    m_bitArray = new SegmentedArray(START, LENGTH);

    uint64_t maxPrimeToLoad = (uint64_t)(sqrt(m_realStart + m_realLength)); // max простое число нужное нам для расчетов переданного диапазона
    uint64_t numPrimesLessThan = maxPrimeToLoad / log(maxPrimeToLoad); // примерное кол-во простых чисел нужное нам для расчетов переданного диапазона
    cout << "There are *approximately* " << numPrimesLessThan << " primes less than " << maxPrimeToLoad << "." << endl;

    uint32_t prldPrimesSize = (uint32_t)(numPrimesLessThan * 1.2); // даем запас 20% для массива так как расчет не точный
    
    cout << "Array size for primes to be read from a file: " << prldPrimesSize << endl;

    m_prldPrimes = new uint64_t[prldPrimesSize];

    m_readPrimes = LoadPrimesFromBINDiffVar(Parameters::PRIMES_FILENAME, m_prldPrimes, prldPrimesSize, maxPrimeToLoad);
    cout << "Number of primes actually read from file: " << m_readPrimes << endl;

    if (m_prldPrimes[m_readPrimes - 1] < maxPrimeToLoad) // считали недостаточно primes. либо в файле с primes недостаточно primes либо размер массива посчитали неправилино (маловероятно)
    //if(readPrimes < prldPrimesSize)
        cout << "WARNING: There might be not enough primes in a file '" << Parameters::PRIMES_FILENAME << "' for calculation new primes!" << endl;

    auto stop = chrono::high_resolution_clock::now();
    cout << "Primes loaded in " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    auto start2 = chrono::high_resolution_clock::now();

    m_delta = m_readPrimes / 100;
    uint32_t percentCounter = 0;//delta;
    uint32_t percent = 0;

    printf("Calculation started.\n");

    if (Parameters::THREADS > 1)
    {
        FindPrimesInThreads(Parameters::THREADS - 1); // waits untill all threads are finished
    }
    else
    {
        uint64_t len = m_bitArray->size();

        for (uint32_t i = 1; i < m_readPrimes; ++i)  //%%%%% пропускаем число 2 начинаем с 3
        {
            if (i > percentCounter)
            {
                printf("\rProgress...%u%%", percent++);
                percentCounter += m_delta;
            }

            uint64_t currPrime = m_prldPrimes[i];

            uint64_t begin = m_realStart % currPrime;
            //long begin = (REAL_START/currPrime)*currPrime;
            //if(begin < REAL_START) begin += currPrime;
            if (begin > 0)
                begin = m_realStart - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
            else
                begin = m_realStart;

            if (begin % 2 == 0) begin += currPrime; // может быстрее (begin&0x01)==0 четные числа выброшены из массива поэтому берем следующее кратное currPrime, оно будет точно нечетное

            uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
            begin = max(begin, j);

            j = (begin - 1) / 2; //нечетное переводим в индекс уплотненного массива

            while (j < len)
            {
                m_bitArray->set(j, true);
                j += currPrime; // здесь должен быть именно currPrime а не primeIndex
            }
        }
    }

    if (m_realStart == 0) m_bitArray->set(0, true); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые
    if (m_realStart == 1) m_bitArray->set(0, true); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые

    cout << endl;

    stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    printf("Saving file...\n");

    start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0; 
    string outputFilename = getOutputFilename();

    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       primesSaved = saveAsTXTOptimumMode(START, m_bitArray, outputFilename);break;
    case txtdiff:   primesSaved = saveAsTXTDiffOptimumMode(START, m_bitArray, outputFilename);break;
    case bin:       primesSaved = saveAsBINOptimumMode(START, m_bitArray, outputFilename);break;
    case bindiff:   primesSaved = saveAsBINDiffOptimumMode(START, m_bitArray, outputFilename);break;
    case bindiffvar:primesSaved = saveAsBINDiffVarOptimumMode(START, m_bitArray, outputFilename);break;
    }
    
    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    delete[] m_prldPrimes;
    delete m_bitArray;
}

void EratosthenesSieve::CalculateSimple()
{
    auto start1 = chrono::high_resolution_clock::now();

    printf("Calculating prime numbers with Eratosfen algorithm.\n");
    cout << "Start: " << m_realStart << endl;
    cout << "Length: " << m_realLength << endl;

    auto sarr = new SegmentedArray(m_realStart, m_realLength);
    //System.out.format("Big array capacity %,d\n", sarr.capacity());

    uint64_t maxPrimeToLoad = (uint64_t)(sqrt(m_realStart + m_realLength)); // max простое число нужное нам для расчетов переданного диапазона
    uint64_t numPrimesLessThan = maxPrimeToLoad / log(maxPrimeToLoad); // примерное кол-во простых чисел нужное нам для расчетов переданного диапазона
    cout << "There are " << numPrimesLessThan << " primes less than " << maxPrimeToLoad << "." << endl;

    uint32_t prldPrimesSize = (uint32_t)(numPrimesLessThan * 1.2); // даем запас 20% для массива так как расчет не точный

    cout << "Calculated array size for primes to be read from a file: " << prldPrimesSize << endl;

    uint64_t* preloadedPrimes = new uint64_t[prldPrimesSize];

    uint32_t readPrimes = LoadPrimesFromBINDiffVar(Parameters::PRIMES_FILENAME, preloadedPrimes, prldPrimesSize, maxPrimeToLoad);
    cout << "Number of primes actually read from file: " << readPrimes << endl;

    if (preloadedPrimes[readPrimes - 1] < maxPrimeToLoad)
    //if (readPrimes < arrSize)
        cout << "WARNING: There might be not enough primes in a file '" << Parameters::PRIMES_FILENAME << "' for calculation new primes!" << endl;

    auto stop = chrono::high_resolution_clock::now();
    cout << "Primes loaded in " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    auto start2 = chrono::high_resolution_clock::now();

    uint32_t delta = readPrimes / 100;
    uint32_t percentCounter = 0;//delta;
    uint32_t percent = 0;

    printf("Calculation started.\n");

    for (uint32_t i = 0; i < readPrimes; ++i) 
       {
           if (i > percentCounter)
           {
               printf("\rProgress...%d%%", percent++);
               percentCounter += delta;
           }

           uint64_t currPrime = preloadedPrimes[i];

           uint64_t begin = m_realStart % currPrime;
           if (begin > 0)
               begin = m_realStart - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
           else
               begin = m_realStart;

           uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
           j = max(begin, j);

           uint64_t len = sarr->size();
           while (j < len)
           {
               sarr->set(j, true);
               j += currPrime; 
           }
       }
       

    if (m_realStart == 0) 
        sarr->set(0, true), sarr->set(1, true); // заглушка если генерим числа начиная с 0. что бы '0' не попало в простые
    if (m_realStart == 1) 
        sarr->set(1, true); // заглушка если генерим числа начиная с 1. что бы '1' не попало в простые

    cout << endl;

    stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    printf("Saving file...\n");

    start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0;
    string outputFilename = getOutputFilename();

    switch (OUTPUT_FILE_TYPE)
    {
    case txt:       primesSaved = saveAsTXTSimpleMode(m_realStart, sarr, outputFilename, false);break;
    case txtdiff:   primesSaved = saveAsTXTSimpleMode(m_realStart, sarr, outputFilename, true); break;
    case bin:       primesSaved = saveAsBINSimpleMode(m_realStart, sarr, outputFilename, false);break;
    case bindiff:   primesSaved = saveAsBINSimpleMode(m_realStart, sarr, outputFilename, true); break;
    case bindiffvar:primesSaved = saveAsBINDiffVarSimpleMode(m_realStart, sarr, outputFilename);break;
    }

    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved in: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    delete[] preloadedPrimes;
    delete sarr;
}


uint64_t EratosthenesSieve::saveAsTXTSimpleMode(uint64_t start, SegmentedArray* sarr, string outputFilename, bool diffMode)
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

    for (uint64_t i = start; i < sarr->size(); ++i)
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

uint64_t EratosthenesSieve::saveAsTXTDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename)
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

    for (uint64_t i = start; i < sarr->size(); ++i)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;
            prime = 2 * i + 1;

            s.append(to_string(prime - lastPrime));
            lastPrime = prime;
 
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

uint64_t EratosthenesSieve::saveAsTXTOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename)
{
    uint32_t chunk = 10'000'000;

    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    string s;
    s.reserve(chunk);

    uint64_t cntPrimes = 0;
    uint64_t prime = 0;

    if ((start == 0) || (start == 1))
        s.append("2,"); // заглушка для сжатого массива если генерим числа начиная с 0.
 
    for (uint64_t i = start; i < sarr->size(); ++i)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;
            prime = 2 * i + 1;

            s.append(to_string(prime));
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

uint64_t EratosthenesSieve::saveAsBINSimpleMode(uint64_t start, SegmentedArray* sarr, string outputFilename, bool diffMode)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t prime;
    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;

    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;

            prime = i;

            if (diffMode)
            {
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
            else
                f.write((char*)&prime, sizeof(uint64_t)); // save as 8 byte value (full number)
        }
    }

    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t prime;
    uint64_t cntPrimes = 0;

    if ((start == 0) || (start == 1))
        prime = 2, f.write((char*)&prime, sizeof(uint64_t)); // заглушка для сжатого массива если генерим числа начиная с 0.


    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;            
            prime = 2 * i + 1;
            f.write((char*)&prime, sizeof(uint64_t)); 
        }
    }

    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINDiffOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename)
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


    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;

            prime = 2 * i + 1;

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

uint64_t EratosthenesSieve::saveAsBINDiffVarOptimumMode(uint64_t start, SegmentedArray* sarr, string outputFilename)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    uint8_t buf[9];
    uint64_t prime;

    if ( (start == 0) || (start == 1) ) // заглушка для сжатого массива если генерим числа начиная с 0.
    {
        prime = 2;
        size_t len = var_len_encode(buf, prime);
        f.write((char*)buf, len);
        lastPrime = prime;
    }

    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;

            prime = 2 * i + 1;

            uint64_t diff = (prime - lastPrime); // разница всегда четная. поэтому можем хранить половину значения. больше значений уместится в 1 байт.

            if (lastPrime > 2) diff /= 2;

            size_t len = var_len_encode(buf, diff);
            f.write((char*)buf, len);

            lastPrime = prime;
        }
    }

    f.close();

    return cntPrimes;
}


uint64_t EratosthenesSieve::saveAsBINDiffVarSimpleMode(uint64_t start, SegmentedArray* sarr, string outputFilename)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);
    f.open(outputFilename, ios::out | ios::binary);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    uint8_t buf[9];
    //uint64_t prime;

    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {        
            cntPrimes++;

            uint64_t diff = (i - lastPrime); // разница всегда четная. поэтому можем хранить половину значения. больше значений уместится в 1 байт.

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
uint32_t EratosthenesSieve::LoadPrimesFromBINDiffVar(string filename, uint64_t* primes, size_t len, uint64_t stopPrime)
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
uint32_t EratosthenesSieve::LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint32_t len, uint64_t stopPrime)
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
    cout << "   <start> - starting value for generating primes. Accepts modificators B (bytes), M (mega), G (giga), T (tera), P (peta) for big values" << endl;
    cout << "   <length> - length of a range. Primes are generated in a range: start...start+length. Accepts modificators B,M,G,T,P" << endl;
    cout << "All three arguments are required." << endl;

    cout << endl<< "Examples:" << endl;
    cout << APP_EXE_NAME << " -s txt 0g 10g" << endl;
    cout << APP_EXE_NAME << " -o bin 900G 100G" << endl;
    cout << APP_EXE_NAME << " -o txtdiff 100T 10G" << endl;
    cout << APP_EXE_NAME << " -s bindiff 5T 50G" << endl;
    cout << APP_EXE_NAME << " -o bindiffvar 20P 1G" << endl;
    cout << APP_EXE_NAME << " -o 1000P 100M" << endl;
    cout << APP_EXE_NAME << " -o 1000B 1G" << endl;
    cout << APP_EXE_NAME << " -o 0M 100M" << endl;
}


void EratosthenesSieve::parseParams(string mode, string oft, string start, string len)
{
    trimStr(mode);
    OPTIMUM_MODE = (mode == "o");
    
    checkStartParam(start); // generates an exception is anything wrong with start param value
    m_realStart = parseOption(start, &symFACTORstart, &FACTORstart);

    // no need to call checkStartParam() for len, see below
    m_realLength = parseOption(len, &symFACTORlen, &FACTORlen); 
    
    if (m_realLength > MAX_LEN)
        throw invalid_argument("Length argument is out of bounds (1...1000G)");

    trimAndUpper(oft);

    OUTPUT_FILE_TYPE = STR_TO_FORMAT(oft);
    if(OUTPUT_FILE_TYPE == none)
        throw invalid_argument("Error: Unsupported output file type specified.");

    defineRanges();
}

void EratosthenesSieve::checkStartParam(string opt)
{
    // remove any leading and traling spaces, just in case.
    trimAndUpper(opt);
    
    char sFactor = opt.at(opt.length() - 1); 

    if (fSymbols.find(sFactor) == string::npos)
        throw invalid_argument("Error: Incorrect command line parameter '" + opt + "'.\n");

    opt = opt.substr(0, opt.length() - 1); // remove letter G at the end. Or T, or P, or M or B.

    uint64_t dStart;
    try
    {
        dStart = stoull(opt);
    }
    catch (...)
    {
        throw invalid_argument("Error: Parameter value should be a number: '" + opt + "'.\n");
    }

    invalid_argument e("Start value is out of bounds (0..18'400'000'000G)");

    switch (sFactor)
    {
    case 'B':  break;
    case 'M': if (dStart > MAX_START_M) throw e; break;
    case 'G': if (dStart > MAX_START_G) throw e; break;
    case 'T': if (dStart > MAX_START_T) throw e; break;
    case 'P': if (dStart > MAX_START_P) throw e; break;
    }

}

void EratosthenesSieve::defineRanges()
{
    uint64_t START = m_realStart / 2ULL; // Переводим в индекс в уплотненном массиве. Не храним четные элементы так как они делятся на 2 по умолчанию.
    uint64_t LENGTH = m_realLength / 2ULL; // переводим в индекс
    int workingThreads = Parameters::THREADS - 1;

    if (workingThreads > 1)
    { 
        uint64_t k = 1;
        uint64_t rangesCount = workingThreads * k;
        uint64_t rangeLen = LENGTH / rangesCount;
        
        mutex* m = new mutex();

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

// парсит одно значение либо m_realStart либо m_realLength в формате с Factor модификаторами G, T, P
// примеры: 100G, 5T, 20000G, 400M, 0B
uint64_t EratosthenesSieve::parseOption(string opt, char* symFactor, uint64_t* factor)
{
    // remove any leading and traling spaces, just in case.
    trimAndUpper(opt);

    char sFactor = opt.at(opt.length() - 1); // we need symFACTOR later for output filename

    if (fSymbols.find(sFactor) == string::npos)
        throw invalid_argument("Error: Incorrect command line parameter '" + opt + "'.\n");

    opt = opt.substr(0, opt.length() - 1); // remove letter G at the end. Or T, or P, or M or B.

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
    case 'M': ffactor = FACTOR_M; break;
    case 'G': ffactor = FACTOR_G; break;
    case 'T': ffactor = FACTOR_T; break;
    case 'P': ffactor = FACTOR_P; break;
    }

    *symFactor = sFactor;
    *factor = ffactor;

    return dStart * ffactor;
}

void EratosthenesSieve::printTime(string msg)
{
    const auto now = chrono::system_clock::now();
    const time_t t_c = chrono::system_clock::to_time_t(now);

#pragma warning(suppress : 4996)
    cout << ctime(&t_c) << msg << endl;

    /*time_t t = time(0);
    tm* local = localtime(&t);
    cout << put_time(local, "%F") << std::endl;
    cout << put_time(local, "%T") << std::endl;
    */

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
}
