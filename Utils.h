#pragma once

#include <string>

size_t var_len_encode(uint8_t buf[9], uint64_t num);
size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t* num);
void Trim(std::string& str);
void TrimAndUpper(std::string& str);
