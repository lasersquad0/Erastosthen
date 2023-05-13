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

uint32_t EratosthenesSieve::getArraySize()
{
    uint64_t maxPrimeToLoad = (uint64_t)(sqrt(real_start + real_length) + 1ULL); // max простое число нужное нам для расчетов переданного диапазона
    uint64_t numPrimesLessThan = maxPrimeToLoad/log(maxPrimeToLoad); // примерное кол-во простых чисел нужное нам для расчетов переданного диапазона
    
    printf("Calculation: there are %llu primes less than %llu.\n", numPrimesLessThan, maxPrimeToLoad);

    return (uint32_t)(numPrimesLessThan * 1.2); // даем запас 20% для массива так как расчет не точный
    
}

string EratosthenesSieve::getOutputFilename()
{
    char buf[100];
    sprintf_s(buf, 100, "primes - %llu-%llu%c", real_start / FACTOR, (real_start + real_length) / FACTOR, symFACTOR);
    
    string fname = buf;
    switch (OUTPUT_FILE_TYPE)
    {
    case 1: fname += ".txt";        break;
    case 2: fname += ".diff.txt";   break;
    case 3: fname += ".bin";        break;
    case 4: fname += ".diff.bin";   break;
    case 5: fname += ".diffvar.bin";break;
    }

    return fname;

}

void EratosthenesSieve::Calculate()
{
    if (SIMPLE_MODE)
        CalculateSimple();
    else
        CalculateOptimum();

}

void EratosthenesSieve::CalculateOptimum()
{
    auto start1 = chrono::high_resolution_clock::now();

    uint64_t START = (real_start) / 2ULL; // Переводим в индекс в уплотненном массиве. Не храним четные элементы так как они делятся на 2 по умолчанию.
    uint64_t LENGTH = (real_length) / 2ULL; // переводим в индекс

    printf("Calculating prime numbers with Eratosfen algorithm.\n");
    cout << "Range: " << real_start << "..." << real_start + real_length << endl;

    auto sarr = new SegmentedArray(START, LENGTH);
    //System.out.format("Big array capacity %,d\n", sarr.capacity());

    uint32_t arrSize = getArraySize();
    cout << "Calculated array size for primes to be read from a file: " << arrSize << endl;

    uint64_t* preloadedPrimes = new uint64_t[arrSize];

    uint32_t readPrimes = LoadPrimesFromTXTFile(Pre_Loaded_Primes_Filename, preloadedPrimes, arrSize);
    cout << "Number of primes actually read from file: " << readPrimes << endl;

    //if (preloadedPrimes[readPrimes - 1] < maxPrimeToLoad)
    if(readPrimes < arrSize)
        printf("WARNING: There might be not enough primes in a file '%s' for calculation new primes!\n", Pre_Loaded_Primes_Filename);

    auto stop = chrono::high_resolution_clock::now();
    cout << "Primes loaded in " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start1).count()) << endl;

    auto start2 = chrono::high_resolution_clock::now();

    uint32_t delta = readPrimes / 100;
    uint32_t percentCounter = 0;//delta;
    uint32_t percent = 0;

    printf("Calculation started.\n");

    for (uint32_t i = 1; i < readPrimes; ++i)  //%%%%% пропускаем число 2 начинаем с 3
    {
        if (i > percentCounter)
        {
            printf("\rProgress...%u%%", percent++);
            percentCounter += delta;
        }

        uint64_t currPrime = preloadedPrimes[i];

        uint64_t begin = real_start % currPrime;
        //long begin = (REAL_START/currPrime)*currPrime;
        //if(begin < REAL_START) begin += currPrime;
        if (begin > 0)
            begin = real_start - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
        else
            begin = real_start;

        if (begin % 2 == 0) begin += currPrime; // может быстрее (begin&0x01)==0 четные числа выброшены из массива поэтому берем следующее кратное currPrime, оно будет точно нечетное

        uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
        begin = max(begin, j);

        j = (begin - 1) / 2; //нечетное переводим в индекс уплотненного массива

        uint64_t len = sarr->size();
        while (j < len)
        {
            sarr->set(j, true);
            j += currPrime; // здесь должен быть именно currPrime а не primeIndex
        }
    }

    if (real_start == 0) sarr->set(0, true); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые

    cout << endl;

    stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    printf("Saving file...\n");

    start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0; 
    string outputFilename = getOutputFilename();

    switch (OUTPUT_FILE_TYPE)
    {
    case 1: primesSaved = saveAsTXT(START, sarr, outputFilename, SIMPLE_MODE, false);break;
    case 2: primesSaved = saveAsTXT(START, sarr, outputFilename, SIMPLE_MODE, true); break;
    case 3: primesSaved = saveAsBIN(START, sarr, outputFilename, SIMPLE_MODE, false);break;
    case 4: primesSaved = saveAsBIN(START, sarr, outputFilename, SIMPLE_MODE, true); break;
    case 5: primesSaved = saveAsBINVar(START, sarr, outputFilename, SIMPLE_MODE);    break;
    }
    
    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    delete[] preloadedPrimes;
    delete sarr;
}

void EratosthenesSieve::CalculateSimple()
{
    auto start1 = chrono::high_resolution_clock::now();

    printf("Calculating prime numbers with Eratosfen algorithm.\n");
    cout << "Range: " << real_start << "..." << real_start + real_length << endl;

    auto sarr = new SegmentedArray(real_start, real_length);
    //System.out.format("Big array capacity %,d\n", sarr.capacity());

    uint32_t arrSize = getArraySize();
    cout << "Calculated array size for primes to be read from a file: " << arrSize << endl;

    uint64_t* preloadedPrimes = new uint64_t[arrSize];

    uint32_t readPrimes = LoadPrimesFromTXTFile(Pre_Loaded_Primes_Filename, preloadedPrimes, arrSize);
    cout << "Number of primes actually read from file: " << readPrimes << endl;

    //if (preloadedPrimes[readPrimes - 1] < maxPrimeToLoad)
    if (readPrimes < arrSize)
        printf("WARNING: There might be not enough primes in a file '%s' for calculation new primes!\n", Pre_Loaded_Primes_Filename);

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

           uint64_t begin = real_start % currPrime;
           if (begin > 0)
               begin = real_start - begin + currPrime; // находим первое число внутри диапазона которое делится на currPrime
           else
               begin = real_start;

           uint64_t j = currPrime * currPrime; // начинаем вычеркивать составные с p^2 потому что все варианты вида p*3, p*5, p*7 уже перебрали ранее в предыдущих итерациях. формула это p^2 пересчитанное в спец индекс
           j = max(begin, j);

           uint64_t len = sarr->size();
           while (j < len)
           {
               sarr->set(j, true);
               j += currPrime; 
           }
       }
       

 //   if (real_start == 1) sarr->set(0, true); // заглушка если генерим числа начиная с 0. что бы '1' не попало в простые

    cout << endl;

    stop = chrono::high_resolution_clock::now();
    cout << "Calculations done: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    printf("Saving file...\n");

    start2 = chrono::high_resolution_clock::now();
    uint64_t primesSaved = 0;
    string outputFilename = getOutputFilename();

    switch (OUTPUT_FILE_TYPE)
    {
    case 1: primesSaved = saveAsTXT(real_start, sarr, outputFilename, SIMPLE_MODE, false);break;
    case 2: primesSaved = saveAsTXT(real_start, sarr, outputFilename, SIMPLE_MODE, true); break;
    case 3: primesSaved = saveAsBIN(real_start, sarr, outputFilename, SIMPLE_MODE, false);break;
    case 4: primesSaved = saveAsBIN(real_start, sarr, outputFilename, SIMPLE_MODE, true); break;
    case 5: primesSaved = saveAsBINVar(real_start, sarr, outputFilename, SIMPLE_MODE);    break;
    }

    cout << "Primes " << primesSaved << " were saved to file '" << outputFilename << "'." << endl;

    stop = chrono::high_resolution_clock::now();

    cout << "File saved in: " << millisecToStr(chrono::duration_cast<chrono::milliseconds>(stop - start2).count()) << endl;

    delete[] preloadedPrimes;
    delete sarr;
}


uint64_t EratosthenesSieve::saveAsTXT(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode, bool diffMode)
{
    uint32_t chunk = 10'000'000;

    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);

    //if(diffMode)
    //    outputFilename += ".diff.txt";
    //else
    //    outputFilename += ".txt";

    f.open(outputFilename, ios::out | ios::binary);
    
    string s;
    s.reserve(chunk);

    if(!simpleMode && (start == 0)) 
        s.append("2,"); // заглушка для сжатого массива если генерим числа начиная с 0.

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    for (uint64_t i = start; i < sarr->size(); ++i)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;
            
            if (simpleMode)
            {
                s.append(to_string(i - lastPrime));
                if (diffMode) lastPrime = i;
            }
            else
            {
                uint64_t ii = 2 * i + 1;
                s.append(to_string(ii - lastPrime));
                if (diffMode) lastPrime = ii;
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

uint64_t EratosthenesSieve::saveAsBIN(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode, bool diffMode)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);

    //if(diffMode)
    //    outputFilename += ".diffvar.bin";
    //else
    //    outputFilename += ".bin";

    f.open(outputFilename, ios::out | ios::binary);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;

    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {
            cntPrimes++;
            uint64_t prime;
            
            if (simpleMode)
                prime = i;                
            else
                prime = 2 * i + 1;

            uint64_t diff = prime - lastPrime;

            if(lastPrime == 0)
                f.write((char*)&prime, sizeof(uint64_t)); // save as 8 byte value (full number)
            else
                f.write((char*)&diff, sizeof(uint16_t));  // sace a 2 bytes value (difference)

            if (diffMode) lastPrime = prime;
        }
    }

    f.close();

    return cntPrimes;
}

uint64_t EratosthenesSieve::saveAsBINVar(uint64_t start, SegmentedArray* sarr, string outputFilename, bool simpleMode)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.sync_with_stdio(false);

    f.open(outputFilename, ios::out | ios::binary);

    uint64_t cntPrimes = 0;
    uint64_t lastPrime = 0;
    uint8_t buf[9];

    for (uint64_t i = start; i < sarr->size(); i++)
    {
        if (!sarr->get(i))
        {
            uint64_t prime;
            cntPrimes++;
            if (simpleMode)
                prime = i;
            else
                prime = 2 * i + 1;

            uint64_t diff = prime - lastPrime;

            size_t len = var_len_encode(buf, diff);
            f.write((char*)buf, len);

            lastPrime = prime;
        }
    }

    f.close();

    return cntPrimes;
}



//Загружаем простые числа из файла пока не загрузим число большее либо равное stopPrime
uint32_t EratosthenesSieve::LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint64_t stopPrime)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.open(filename, ios::in);

    string line;

    uint64_t lastPrime = 0;
    uint32_t cnt = 0;
    while ((lastPrime < stopPrime) && !f.eof())
    {
        getline(f, line, ',');
        lastPrime = atoll(line.c_str());
        primes[cnt++] = lastPrime;
    }

    f.close();

    return cnt;
}

//Загружаем либо весь файл либо up to len простых чисел из файла 
uint32_t EratosthenesSieve::LoadPrimesFromTXTFile(string filename, uint64_t* primes, uint32_t len)
{
    fstream f;
    f.exceptions(ifstream::failbit | ifstream::badbit);
    f.open(filename, ios::in);

    string line;

    uint32_t cnt = 0;
    while ((cnt < len) && !f.eof())
    {
        getline(f, line, ',');
        primes[cnt++] = atoll(line.c_str());
    }

    f.close();

    return cnt;
}

// первый параметр командной строки это режим и тип output файла
// режима 2: сжатый (по умолчанию) и simple (расжатый требует в 2 раза больше памяти)
// типов output файлов четыре: txt, txtdiff, bin, bindiff, bindiffvar.
// bindiffvar - значения diff записываются в формате variable length: либо в 1 байт (0..127) либо в 2 байта (значения 128..16383).
// формат cmd опции: [c,s][1,2,3,4,5]. 
// опция может состоять из 2х символов. первый это либо 'с' либо 's'. второй это одна цифра от 1 до 5.
// пропускать ничего нельзя, обязательно должна быть эта опция и обязательно одна буква, а за ней цифра (в таком порядке)
// примеры:
// Erastofen.exe c1 
// Erastofen.exe c2 900G
// Erastofen.exe c4 100G 10G
// Erastofen.exe s3 5T 50G
// Erastofen.exe s1 20000G 1G
// Erastofen.exe c2 100M 100M
// Erastofen.exe c5 1000B 1G
// Erastofen.exe c5 0G 1G
void EratosthenesSieve::parseCmdLine(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("No command line arguments, using default start value '%llu'\n", real_start);
        return;
    }

    if (argc > 1)
        parseMode(argv[1]); // обязательный параметр.

    if (argc > 2)
        real_start = parseOption(argv[2]); // если нету argv[2] то возьмется значение по умолчанию

    if (argc > 3)
        real_length = parseOption(argv[3]); // если нету argv[3] то возьмется значение по умолчанию

}

void EratosthenesSieve::parseMode(string s)
{
    // переводим в uppercase
    transform(s.begin(), s.end(), s.begin(), ::toupper);

    if (s.at(0) == 'C')
        SIMPLE_MODE = false;
    else if (s.at(0) == 'S')
        SIMPLE_MODE = true;
    else
        printf("Wrong mode option is specified ('%c'), using default value.\n", s.at(0));

    s = s.substr(1, s.length());
    uint32_t v = 0;
    try
    {
        v = stol(s);
    }
    catch(invalid_argument e)
    {
        // do nothing
    }

    if ((v > 0) && (v < 6))
        OUTPUT_FILE_TYPE = v;
    else
        printf("Wrong output file type is specified ('%c'), using default value.\n", s.at(0));
}

// парсит одно значение либо real_start либо real_length в формате с Factor модификаторами G, T, P
// примеры: 100G, 5T, 20000G, 400M, 0B
uint64_t EratosthenesSieve::parseOption(string opt)
{
    // remove any leading and traling spaces, just in case.
    size_t strBegin = opt.find_first_not_of(' ');
    size_t strEnd = opt.find_last_not_of(' ');
    opt.erase(strEnd + 1, opt.size() - strEnd);
    opt.erase(0, strBegin);

    // to uppercase
    transform(opt.begin(), opt.end(), opt.begin(), ::toupper);

    symFACTOR = opt.at(opt.length() - 1); // we need symFACTOR later for output filename

    if (fSymbols.find(symFACTOR) == string::npos)
    {
        const size_t BSIZE = 100;
        char buf[BSIZE];
        sprintf_s(buf, BSIZE, "Incorrect command line parameter '%s'.\n", opt.c_str());
        throw invalid_argument(buf);
    }

    opt = opt.substr(0, opt.length() - 1); // remove letter G at the end. Or T, or P, or M or B.

    uint64_t dStart = stoull(opt);

    switch (symFACTOR)
    {
    case 'B': FACTOR = FACTOR_B; break;
    case 'M': FACTOR = FACTOR_M; break;
    case 'G': FACTOR = FACTOR_G; break;
    case 'T': FACTOR = FACTOR_T; break;
    case 'P': FACTOR = FACTOR_P; break;
    }

    return dStart * FACTOR;
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
