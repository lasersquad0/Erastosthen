# Erastofen
This is a tool that implements Eratosthen algorithm of generating prime numbers.
This implementation is able to generate numbers in the range FROM-LENGTH specified in command line.

**Command line parameters**
There can be up to three command line arguments.
Only the first argument is mandatory, while two others can be omited.
If argument is omitted than default hardcoded value is used for that parameter.

First command line argument consists of two symbold.
First symbol defines prime numbers generation mode, second symbol defines type of output file where primes numbers will be stored.

**Generation mode**

There are two modes: simple and 'compressed'.
Eratosthen algorythm requires much memory for its implementation.
Compressed mode is an experimental, it requires two times less memory than simple mode.
Simple mode is strainghtforward implementation of Eratosthen algorithm.
Use symbol 's' to use simple mode of generation prime numbers
Use symbol 'c' to use compressed mode of generating primes.

**Output file type**
Generated  primes numbers can be stored into file in 5 different formats. 
Select file format that is appropriate for you.

Supported formats: txt, txtdiff, bin, bindiff, bindiffvar.
.txt - standard txt format where all prime number are stored as comma separated desimal symbols and can be easily viewed in notepad. Files of this format have the biggest size.
.txtdiff - this also text format the can be viewed in notepad. Each prime number starting from second is written as difference with previous one. first prime number is writtes as is. That allows to make file smaller. 
It becomes especially important is you need to generate big primes number and in big range (let say 100G range).
.bin - numbers are stored in binary format. Each number takes exacly 8 bytes (long long in C++). to work with this format you probably need to write your code to read this format. This is easy. 
Please pay attention that some other programming languages and/or platforms may use other sequence of bytes when save/read binary integers from file. 
.bindiff - idea is the same as .txtdiff but in binry format. 2 bytes (short C++ type) are used for saving diff values. It make bindiff file two time less size than .bin file.
.bindiffvar - main idea is the following: prime numbers usually stay close to each other. For even big numbers (but numbers that fit into C++ long long) maximum distance between two consequitive primes is always less than 800.
Is most cases distance is even less than 255. It means that if distance is less than 255 we can code this difference with 1 byte, and for rare cases when the difference is larger than 255 we can code such value with 2 bytes.
That allows us to create even more 'compressed' format of storing such numbers. .bindiffvar format uses 'variable length' coding to store difference between two consequitive primes numbers.
it makes file with primes numbers 10 times less in size than txt format.

To specify file format use numbers 1..5 rights after mode sylbol ('c' or 's').
1 is txt format, 5 is bindiffvar format.

**Start and Length **
Second and third arguments define START and LENGTH parameters of generating primes.
You can use factor modificators to make it more convenient to define range.
B - bytes
M - megabytes
G - gigabytes
T - terabytes
P - petabytes


Examples for command lines arguments:
Erastofen.exe c1            - 'compressed' mode, txt file format, START and LENGTH - default hardcoded values are used. START=20T LENGTH=10G
Erastofen.exe c2 900G       - 'compressed' mode, txtdiff file format, START and LENGTH - default hardcoded values are used.
Erastofen.exe c4 100G 10G   - 'compressed' mode, bindiff file format, START=100'000'000'000 and LENGTH=10'000'000'000
Erastofen.exe s3 5T 50G     - 'simple' mode, bin file format, START=5T and LENGTH=50G
Erastofen.exe s1 20000G 1G  - 'simple' mode, txt file format, START=20T and LENGTH=1G
Erastofen.exe c2 100M 100M  - 'compressed' mode, txtdiff file format, START=100'000'000 and LENGTH=100'000'000
Erastofen.exe c5 1000B 1G   - 'compressed' mode, bindiffvar file format, START=1000 and LENGTH=1G
Erastofen.exe c5 0G 1G      - 'compressed' mode, bindiff file format, START=0 and LENGTH=1G

