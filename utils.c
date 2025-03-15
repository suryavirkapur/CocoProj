/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

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

void ensureTrailingNewlines(const char* filename) {
  int fd = open(filename, O_RDWR);
  if (fd == -1) {
    perror("open in ensureTrailingNewlines");
    return;
  }

  off_t filesize = lseek(fd, 0, SEEK_END);
  if (filesize == -1) {
    perror("lseek in ensureTrailingNewlines");
    close(fd);
    return;
  }

  int needNewlines = 1;
  if (filesize >= 2) {
    char lastTwo[2];
    if (lseek(fd, -2, SEEK_END) == -1) {
      perror("lseek (to check last two bytes)");
      close(fd);
      return;
    }
    if (read(fd, lastTwo, 2) != 2) {
      perror("read in ensureTrailingNewlines");
      close(fd);
      return;
    }
    if (lastTwo[0] == '\n' && lastTwo[1] == '\n') { needNewlines = 0; }
  }

  if (needNewlines) {
    if (lseek(fd, 0, SEEK_END) == -1) {
      perror("lseek (append newlines)");
      close(fd);
      return;
    }
    if (write(fd, "\n\n", 2) != 2) { perror("write in ensureTrailingNewlines"); }
  }

  close(fd);
}

int str2int(const char* str) {
  if (str == NULL || *str == '\0') return 0;

  char* endptr;
  long  num = strtol(str, &endptr, 10);

  return (int)num;
}

float str2flt(const char* str) {
  if (str == NULL || *str == '\0') return 0.0f;

  char*  endptr;
  double f = strtod(str, &endptr);

  return (float)f;
}

int computeHash(const char* str) {
  uint32_t hash = 5381;
  int      cc;
  while ((cc = *str++)) hash = ((hash << 5) + hash) + cc;
  return (int)(hash % NUM_KEYWORDS);
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