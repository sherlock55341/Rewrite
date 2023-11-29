#pragma once

#include <global.hpp>

RW_NAMESPACE_START

size_t getFileSize(const std::string &path);

unsigned decode(char **pos);

void encode(char *buf, int &cur, unsigned x);

std::string getDesignName(const std::string& path);

RW_NAMESPACE_END