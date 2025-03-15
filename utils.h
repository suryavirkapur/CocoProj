/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

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

void ensureTrailingNewlines(const char* filename);

int computeHash(const char* str);

bool isCharacterInRange(const char character, const char rangeStart, const char rangeEnd);
bool isCharacterEqualTo(const char character, const char targetCharacter);

char* duplicateSubstring(const char* sourceStart, const char* sourceEnd);

int str2int(const char* str);
int str2flt(const char* str);

#endif