#pragma once

#include <vector>
#include <string>
#include <cstring>
#include "Defines.h"

std::vector<uint8_t> hex2bytes (std::string hex);
std::string swapEndian(std::string str);
bool getTypeUrl(std::string url);
std::string getBlockHash (bool is_remote);
void hex2bytes(const char* hex, uint8_t privateKey_[KEYS_SIZE]);
void bytes2hex(const uint8_t* bytes, size_t length, char* hexString);
