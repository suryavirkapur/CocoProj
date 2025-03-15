#ifndef UTILS_H
#define UTILS_H

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int computeHash(const char* str);

bool isCharacterInRange(const char character, const char rangeStart, const char rangeEnd);
bool isCharacterEqualTo(const char character, const char targetCharacter);

char* duplicateSubstring(const char* sourceStart, const char* sourceEnd);

#endif