# Eratosthen primes generating tool
This is a tool that implements optimized Eratosthen algorithm of generating prime numbers.
This implementation generates prime numbers in a range specified in command line.

## Command line arguments
```
Eratosthen usage:
-o, --optimum    <outfiletype> <start> <length> Generates primes using optimized Eratosthen sieve mode. Output file is stored in <outfiletype> format (see below).
-s, --simple     <outfiletype> <start> <length> Generates primes using simple Eratosthen sieve mode. Output file is stored in <outfiletype> format.
-p, --primesfile <filename>                  File with primes to preload, if not specified then default file 'primes - 0-1G.diffvar.bin' is used. File is looked in the same folder with Eratosthen.exe file.
-t, --threads    <threads>                   Use specified number of threads during primes checking. If not specified, single thread used. 
-h, --help                                   Show this help
```
Options 's' or 'o' are mandatory. All the others are optional.
All three arguments of options 's' and 'o' must be specified. There are no default values for these options.

## Prime generating modes
There are two primes generating modes: simple ('s') and optimized ('o').
Eratosthen algorithm requires much memory for execution.
Simple mode is a strainghtforward implementation of Eratosthen algorithm.
Optimized mode requires about two times less memory than simple mode because it excludes even numbers consideration.
Use symbol 's' to use simple mode of generation prime numbers.
Use symbol 'o' to use optimised mode of generating primes.

## Output file type
Generated primes numbers can be stored into 5 different file formats. 
Select file format that is appropriate for you.

Supported formats: txt, txtdiff, bin, bindiff, bindiffvar.
- **.txt** - standard txt format where all prime numbers are stored as comma separated decimal numbers and can be easily viewed in notepad. Files of this format have the biggest size.
- **.txtdiff** - this is also text format that can be viewed in notepad. Each prime number starting from second is written as difference with previous one. First prime number is written as is. 
That allows to make output file much smaller. It becomes especially important is you need to generate big primes number and in big range (let say 100G range or even more).
- **.bin** - numbers are stored in binary format. Each prime number in a file takes exacly 8 bytes (long long in C++). To work with this format you probably need to write your code to read this format. This is easy. 
Please pay attention that some other programming languages and/or platforms may use other sequence of bytes when saving/reading binary integers from file. 
- **.bindiff** - idea is the same as .txtdiff but in binry format. Only 2 bytes (short C++ type) instead of 8 are used for saving diff values. It makes bindiff file two times smaller than .bin file.
- **.bindiffvar** - main idea is the following: prime numbers usually stay not too far from each other. For even really big numbers (but numbers that fit into C++ long long type) maximum distance between two consequitive primes is always less than 800. 
Is most cases distance is even less than 255. It means that if distance is less than 255 we can code this difference with 1 byte, and only for rare cases when the difference is larger than 255 we can code such value with 2 bytes. 
In addition to above distance between two primes numbers is always even. So we can store diff/2 instead of diff. 
That idea allows us to create even more 'compressed' format of storing such numbers. **.bindiffvar** format uses 'variable length' coding to store difference between two consequitive primes numbers.
it makes file with primes numbers 10 times smaller size than txt format.

To specify file format use predefined names: txt, txtdiff, bindiff, bindiffvar as the first argument (<outfiletype>) of 'o' and 's' options.

## Start and Length
Second and third arguments define START and LENGTH range parameters for generating primes.
You can use factor modificators (case insensitive) to make it more convenient to define range.
B - bytes

K - kilobytes

M - megabytes

G - gigabytes

T - terabytes

P - petabytes

Examples for command lines arguments:
```
Erastofen.exe -s txt 0B 10g           - 'simple' mode, txt file format, seeks primes in a range 0...10G 
Erastofen.exe -o bin 900G 100G        - 'optimised' mode, bin file format, seeks primes from 900G to 1000G
Erastofen.exe -o txtdif 100T 10G      - 'optimised' mode, txtdiff file format, seeks primes from 100 terabytes =100'000'000'000'000 to 100T+10G =100'010'000'000'000
Erastofen.exe -s bindiff 5T 50g       - 'simple' mode, bindiff file format, START=5T and LENGTH=50G (5T...5T+50G)
Erastofen.exe -o bindiffvar 20P 1G    - 'optimised' mode, bindiffvar file format, START=20T and LENGTH=1G
Erastofen.exe -t 5 -o bindiffvar 20P 1G  - the same as above but primes generation will be performed in 4 threads + 1 main and controling thread. Total is 5.
Erastofen.exe -t 5 -p "primes - 3G.diffvar.bin" -o bindiffvar 20P 1G   - the same as above but primes will be preloaded from specified file instead of default one.
Erastofen.exe -o 0P 1G                - if you would like to generate primes starting from zero you can use any modificator for START argument. Value without modificator will generate an error.
```
