#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int computeHash(const char* str);

bool isCharacterInRange(const char character, const char rangeStart, const char rangeEnd);
bool isCharacterEqualTo(const char character, const char targetCharacter);

char* duplicateSubstring(const char* sourceStart, const char* sourceEnd);

#endif