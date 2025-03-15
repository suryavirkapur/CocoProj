#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "constants.h"

int computeHash(const char* str) {
    uint32_t hash = 5381; // Start with a prime number
    int      c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return (int)(hash % NUM_KEYWORDS); // Ensure positive result
}

bool isCharacterInRange(const char character, const char rangeStart, const char rangeEnd) {
    return (character >= rangeStart && character <= rangeEnd);
}

bool isCharacterEqualTo(const char character, const char targetCharacter) {
    return (character == targetCharacter);
}

char* duplicateSubstring(const char* sourceStart, const char* sourceEnd) {
    if (sourceStart == NULL || sourceEnd == NULL || sourceEnd < sourceStart) { return NULL; }

    size_t substringLength   = sourceEnd - sourceStart;
    char*  destinationBuffer = (char*)malloc((substringLength + 1) * sizeof(char));

    if (destinationBuffer == NULL) { return NULL; }

    memcpy(destinationBuffer, sourceStart, substringLength);
    destinationBuffer[substringLength] = '\0';

    return destinationBuffer;
}