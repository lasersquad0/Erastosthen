
//#include <iostream>
#include <cassert>
//#include <chrono>
//#include <sstream>
#include <algorithm>
#include "Utils.h"


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

void Trim(std::string& str) // str passed by value here intentionally
{
    // remove any leading and traling spaces tabs.
    size_t strBegin = str.find_first_not_of(" \t");
    if (strBegin == std::string::npos) return;

    size_t strEnd = str.find_last_not_of(" \t");
    assert(strEnd != std::string::npos);

    str.erase(strEnd + 1 /*, str.size() - strEnd*/);
    str.erase(0, strBegin);
}

static char mytoupper(int c) // to eliminate compile warning "warning C4244: '=': conversion from 'int' to 'char', possible loss of data"
{
    return (char)toupper(c);
}

void TrimAndUpper(std::string& str)
{
    // remove any leading and traling spaces, just in case.
    /*size_t strBegin = str.find_first_not_of(" \t");
    if (strBegin == std::string::npos) return;

    size_t strEnd = str.find_last_not_of(" \t");
    assert(strEnd != std::string::npos);

    str.erase(strEnd + 1/*, str.size() - strEnd*//*);
    str.erase(0, strBegin);*/

    Trim(str);

    // to uppercase
    transform(str.begin(), str.end(), str.begin(), ::mytoupper);
}