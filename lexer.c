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
#include "lexer.h"
#include "utils.h"

char* buffPair[2];
int   bufferIndex           = 0;
bool  currentBufferIsSecond = 1;
int   inputBytesRead        = 0;
int   fileDescriptor;
char* lexemeStart        = NULL;
char* forward            = NULL;
int   lexerState         = 0;
int   lineCount          = 1;
int   lookaheadCharacter = 0;
int   backtrackingState  = 0;
bool  lexDebug           = false;

struct Node {
  TokenName    tokenName;
  char*        tLexeme;
  struct Node* next;
};

struct KeywordNode {
  struct Node* keywords;
};

struct KeywordTable {
  struct KeywordNode* KEYWORDS;
};

struct KeywordTable* kt;

struct Node* addNode(struct Node* head, TokenName token, char* text) {
  if (lexDebug) printf("Adding node %s\n", text);
  struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
  if (newNode == NULL) { return NULL; }
  newNode->tLexeme   = text;
  newNode->tokenName = token;
  newNode->next      = head;
  return newNode;
}

void insertKeyword(struct KeywordTable* table, TokenName token, char* text) {
  if (lexDebug) printf("Inserting keyword %s\n", text);
  int index                       = computeHash(text);
  table->KEYWORDS[index].keywords = addNode(table->KEYWORDS[index].keywords, token, text);
}

struct Node* findKeyword(struct KeywordTable* hashTable, const char* keyword) {
  if (lexDebug) printf("Looking for keyword %s\n", keyword);
  uint32_t     bucketIndex = computeHash(keyword);
  struct Node* currentNode = hashTable->KEYWORDS[bucketIndex].keywords;

  while (currentNode != NULL) {
    if (strcmp(keyword, currentNode->tLexeme) == 0) { return currentNode; }
    currentNode = currentNode->next;
  }
  return NULL;
}

struct KeywordTable* createKeywordTable() {
  if (lexDebug) printf("Creating keyword table\n");
  struct KeywordTable* hashTable = (struct KeywordTable*)malloc(sizeof(struct KeywordTable));
  if (hashTable == NULL) return NULL;

  hashTable->KEYWORDS = (struct KeywordNode*)malloc(NUM_KEYWORDS * sizeof(struct KeywordNode));
  if (hashTable->KEYWORDS == NULL) {
    free(hashTable);
    return NULL;
  }

  for (int i = 0; i < NUM_KEYWORDS; i++) { hashTable->KEYWORDS[i].keywords = NULL; }

  struct {
    TokenName   token;
    const char* keyword;
  } keywordData[] = {{TK_WITH, "with"},
                     {TK_PARAMETERS, "parameters"},
                     {TK_END, "end"},
                     {TK_WHILE, "while"},
                     {TK_TYPE, "type"},
                     {TK_MAIN, "_main"},
                     {TK_GLOBAL, "global"},
                     {TK_PARAMETER, "parameter"},
                     {TK_LIST, "list"},
                     {TK_INPUT, "input"},
                     {TK_OUTPUT, "output"},
                     {TK_INT, "int"},
                     {TK_REAL, "real"},
                     {TK_ENDWHILE, "endwhile"},
                     {TK_IF, "if"},
                     {TK_THEN, "then"},
                     {TK_ENDIF, "endif"},
                     {TK_READ, "read"},
                     {TK_WRITE, "write"},
                     {TK_RETURN, "return"},
                     {TK_CALL, "call"},
                     {TK_RECORD, "record"},
                     {TK_ENDRECORD, "endrecord"},
                     {TK_ELSE, "else"},
                     {TK_DEFINETYPE, "definetype"},
                     {TK_AS, "as"},
                     {TK_UNION, "union"},
                     {TK_ENDUNION, "endunion"}};

  int numKeywords = sizeof(keywordData) / sizeof(keywordData[0]);
  for (int i = 0; i < numKeywords; i++) {
    insertKeyword(hashTable,
                  keywordData[i].token,
                  (char*)keywordData[i].keyword); // Cast to char* is safe here, since insertKeyword does not modify it.
  }

  return hashTable;
}

void setupBuffers(int f) {
  if (lexDebug) printf("Setting up buffers\n");
  lexemeStart           = NULL;
  forward               = NULL;
  fileDescriptor        = f;
  inputBytesRead        = 0;
  lineCount             = 1;
  lookaheadCharacter    = 0;
  buffPair[0]           = (char*)malloc(BUFFER_SIZE * sizeof(char));
  buffPair[1]           = (char*)malloc(BUFFER_SIZE * sizeof(char));
  lexerState            = 0;
  currentBufferIsSecond = 1;
  bufferIndex           = 0;
}

void setupLexer(int f) {
  if (lexDebug) printf("Setting up lexer\n");
  setupBuffers(f);
  kt                = createKeywordTable();
  backtrackingState = 0;
}

int getInputStream() {
  if (inputBytesRead != 0) return EOF;

  memset(buffPair[bufferIndex], EOF, BUFFER_SIZE);

  int res = read(fileDescriptor, buffPair[bufferIndex], BUFFER_SIZE);
  if (res == 0 || res < BUFFER_SIZE) { inputBytesRead = 1; }

  bufferIndex           = 1 - bufferIndex;
  currentBufferIsSecond = 1 - currentBufferIsSecond;
  if (res == -1) {
    printf("Error: Input Buffers failed to be loaded\n");
    return -1;
  }
  return res;
}

char nextChar() {
  if (lexDebug) printf("Next char\n");
  if (lexemeStart == NULL && forward == NULL) {
    int res = getInputStream();
    if (res == -1) { return EOF; }
    lexemeStart        = buffPair[currentBufferIsSecond];
    forward            = buffPair[currentBufferIsSecond];
    char* curr_forward = forward;
    forward++;

    if (lookaheadCharacter == 0 && *curr_forward == '\n') { lineCount++; }

    if (lookaheadCharacter == 1) lookaheadCharacter = 0;

    return *curr_forward;
  }

  char* curr_forward = forward;

  if (curr_forward - buffPair[currentBufferIsSecond] == BUFFER_SIZE - 1) {
    int res = getInputStream();
    if (res == -1) { return EOF; }
    forward = buffPair[currentBufferIsSecond];
  } else if (*curr_forward == EOF) {
    return EOF;
  } else
    forward++;

  if (lookaheadCharacter == 0 && *curr_forward == '\n') { lineCount++; }

  if (lookaheadCharacter == 1) lookaheadCharacter = 0;

  return *curr_forward;
}

void accept() {
  if (lexDebug) printf("Accept\n");
  lexemeStart = forward;
  backtrackingState++;
}

void retract(int amt) {
  if (lexDebug) printf("Retract %d\n", amt);
  while (amt > 0) {
    --forward;
    --amt;
  }
  backtrackingState--;
  lookaheadCharacter = 1;
}

Token* fillToken(Token* t, TokenName tokenName, char* lexeme, int lineNumber, int isNumber, tVal* value) {
  if (lexDebug) printf("Filled token with line %d\n", lineNumber);
  t->tokenName = tokenName;
  t->lineNum   = lineNumber;
  t->isNum     = isNumber;
  t->tLexeme   = lexeme;
  t->VALUE     = value;
  return t;
}

Token* getToken() {
  if (lexDebug) printf("Getting token\n");

  // track dfa state
  lexerState = 0;
  char c     = 1;
  int  errorType;

  // make a new token
  Token* t = (Token*)malloc(sizeof(Token));

  while (1) {
    if (lexDebug) printf("Lexer state %d\n", lexerState);
    if (c == EOF) return NULL;

    switch (lexerState) {
    case 0: {
      c = nextChar();
      if (isCharacterEqualTo(c, '<')) {
        lexerState        = 16;
        backtrackingState = 16;
      } else if (isCharacterEqualTo(c, '#')) {
        lexerState        = 52;
        backtrackingState = 52;
      } else if (isCharacterInRange(c, 'b', 'd')) {
        lexerState        = 35;
        backtrackingState = 35;
      } else if (isCharacterEqualTo(c, 'a') || isCharacterInRange(c, 'e', 'z')) {
        lexerState        = 40;
        backtrackingState = 40;
      } else if (isCharacterEqualTo(c, '_')) {
        lexerState        = 47;
        backtrackingState = 47;
      } else if (isCharacterEqualTo(c, '[')) {
        lexerState        = 7;
        backtrackingState = 7;
      } else if (isCharacterEqualTo(c, ']')) {
        lexerState        = 8;
        backtrackingState = 8;
      } else if (isCharacterEqualTo(c, ',')) {
        lexerState        = 9;
        backtrackingState = 9;
      } else if (isCharacterEqualTo(c, ';')) {
        lexerState        = 12;
        backtrackingState = 12;
      } else if (isCharacterEqualTo(c, ':')) {
        lexerState        = 11;
        backtrackingState = 11;
      } else if (isCharacterEqualTo(c, '.')) {
        lexerState        = 10;
        backtrackingState = 10;
      } else if (isCharacterEqualTo(c, '(')) {
        lexerState        = 5;
        backtrackingState = 5;
      } else if (isCharacterEqualTo(c, ')')) {
        lexerState        = 6;
        backtrackingState = 6;
      } else if (isCharacterEqualTo(c, '+')) {
        lexerState        = 1;
        backtrackingState = 1;
      } else if (isCharacterEqualTo(c, '-')) {
        lexerState        = 2;
        backtrackingState = 2;
      } else if (isCharacterEqualTo(c, '*')) {
        lexerState        = 3;
        backtrackingState = 3;
      } else if (isCharacterEqualTo(c, '/')) {
        lexerState        = 4;
        backtrackingState = 4;
      } else if (isCharacterEqualTo(c, '~')) {
        lexerState        = 13;
        backtrackingState = 13;
      } else if (isCharacterEqualTo(c, '!')) {
        lexerState        = 14;
        backtrackingState = 14;
      } else if (isCharacterEqualTo(c, '>')) {
        lexerState        = 22;
        backtrackingState = 22;
      } else if (isCharacterEqualTo(c, '=')) {
        lexerState        = 25;
        backtrackingState = 25;
      } else if (isCharacterEqualTo(c, '@')) {
        lexerState        = 27;
        backtrackingState = 27;
      } else if (isCharacterEqualTo(c, '&')) {
        lexerState        = 30;
        backtrackingState = 30;
      } else if (isCharacterEqualTo(c, '%')) {
        lexerState        = 33;
        backtrackingState = 33;
      } else if (isCharacterInRange(c, '0', '9')) {
        lexerState        = 42;
        backtrackingState = 42;
      } else if (isCharacterEqualTo(c, ' ') || isCharacterEqualTo(c, '\f') || isCharacterEqualTo(c, '\r') ||
                 isCharacterEqualTo(c, '\t') || isCharacterEqualTo(c, '\v')) {
        lexemeStart++;
        lexerState        = 0;
        backtrackingState = 0;
      } else if (isCharacterEqualTo(c, '\n')) {
        lexemeStart++;
        lexerState        = 0;
        backtrackingState = 0;
      } else if (isCharacterEqualTo(c, EOF)) {
        return NULL;
      } else {
        printf("Line %d Error: Unknown Symbol <%c>\n", lineCount, c);
        errorType         = 6;
        lexerState        = 55;
        backtrackingState = 55;
      }
      break;
    }
    case 1: {
      if (lexDebug) printf("[DEBUG] Token is +\n and State is 1\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_PLUS, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 2: {
      if (lexDebug) printf("[DEBUG] Token is -\n and State is 2\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_MINUS, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 3: {
      if (lexDebug) printf("[DEBUG] Token is *\n and State is 3\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_MUL, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 4: {
      if (lexDebug) printf("[DEBUG] Token is /\n and State is 4\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_DIV, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 5: {
      if (lexDebug) printf("[DEBUG] Token is (\n and State is 5\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_OP, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 6: {
      if (lexDebug) printf("[DEBUG] Token is )\n and State is 6\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_CL, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 7: {
      if (lexDebug) printf("[DEBUG] Token is [\n and State is 7\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_SQL, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 8: {
      if (lexDebug) printf("[DEBUG] Token is ]\n and State is 8\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_SQR, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 9: {
      if (lexDebug) printf("[DEBUG] Token is ,\n and State is 9\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_COMMA, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 10: {
      if (lexDebug) printf("[DEBUG] Token is .\n and State is 10\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_DOT, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 11: {
      if (lexDebug) printf("[DEBUG] Token is :\n and State is 11\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_COLON, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 12: {
      if (lexDebug) printf("[DEBUG] Token is ;\n and State is 12\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_SEM, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 13: {
      if (lexDebug) printf("[DEBUG] Token is !\n and State is 13\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_NOT, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 14: {

      c = nextChar();
      if (c == '=') {
        if (lexDebug) printf("[DEBUG] Token is != and State is 14\n");
        lexerState = 15;
      } else {
        if (lexDebug) printf("[DEBUG] Token is != and State is 15\n");
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 15: {
      if (lexDebug) printf("[DEBUG] Token is <\n and State is 15\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_NE, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 16: {
      if (lexDebug) printf("[DEBUG] Token is <= and State is 16\n");
      c = nextChar();
      if (c == '-') {
        lexerState = 17;
      } else if (c == '=') {
        lexerState = 20;
      } else {
        lexerState = 21;
      }
      break;
    }
    case 17: {
      if (lexDebug) printf("[DEBUG] Token is >= and State is 17\n");
      c = nextChar();
      if (c == '-') {
        lexerState = 18;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 18: {
      if (lexDebug) printf("[DEBUG] Token is - and State is 18\n");
      c = nextChar();
      if (c == '-') {
        lexerState = 19;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 19: {
      if (lexDebug) printf("[DEBUG] Token is <=\n and State is 19\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_ASSIGNOP, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 20: {
      if (lexDebug) printf("[DEBUG] Token is <=\n and State is 20\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_LE, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 21: {
      if (lexDebug) printf("[DEBUG] Token is <= and State is 21\n");
      retract(1);
      char* lex = duplicateSubstring(lexemeStart, forward);

      if (c == '\n')
        fillToken(t, TK_LT, lex, lineCount - 1, 0, NULL);
      else
        fillToken(t, TK_LT, lex, lineCount, 0, NULL);

      accept();
      return t;
      break;
    }
    case 22: {
      if (lexDebug) printf("[DEBUG] Token is >= and State is 22\n");
      c = nextChar();
      if (c == '=') {
        lexerState = 23;
      } else {
        lexerState = 24;
      }
      break;
    }
    case 23: {

      if (lexDebug) printf("[DEBUG] Token is >= and State is 23\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_GE, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 24: {

      if (lexDebug) printf("[DEBUG] Token is >= and State is 24\n");
      retract(1);
      char* lex = duplicateSubstring(lexemeStart, forward);
      if (c == '\n')
        fillToken(t, TK_GT, lex, lineCount - 1, 0, NULL);
      else
        fillToken(t, TK_GT, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 25: {

      if (lexDebug) printf("[DEBUG] Token is >= and State is 24\n");
      c = nextChar();
      if (c == '=') {

        if (lexDebug) printf("[DEBUG] Token is >=, State 23, Going 26\n");
        lexerState = 26;
      } else {
        if (lexDebug) printf("[DEBUG] Token is >=, State 23, Going 55\n");
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 26: {
      if (lexDebug) printf("[DEBUG] Token is >=\n and State is 26\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_EQ, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 27: {
      c = nextChar();
      if (c == '@') {
        if (lexDebug) printf("[DEBUG] Token is @, State 27\n");
        lexerState = 28;
      } else {
        if (lexDebug) printf("[DEBUG] Token is @, State 27, Going 55\n");
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 28: {
      c = nextChar();
      if (c == '@') {
        if (lexDebug) printf("[DEBUG] Token is @, State 28\n");
        lexerState = 29;
      } else {
        if (lexDebug) printf("[DEBUG] Token is @, State 28, Going 55\n");
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 29: {
      if (lexDebug) printf("[DEBUG] Token is @\n and State is 29\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_OR, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 30: {
      if (lexDebug) printf("[DEBUG] Token is &, State 30\n");
      c = nextChar();
      if (c == '&') {
        lexerState = 31;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 31: {
      if (lexDebug) printf("[DEBUG] Token is &, State 31\n");
      c = nextChar();
      if (c == '&') {
        lexerState = 32;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 32: {
      if (lexDebug) printf("[DEBUG] Token is &\n and State is 32\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      fillToken(t, TK_AND, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 33: {
      if (lexDebug) printf("[DEBUG]  \n and State is 33\n");
      c = nextChar();
      while (c != '\n' && c != EOF) { c = nextChar(); }
      lexerState = 34;
      break;
    }
    case 34: {
      if (lexDebug) printf("[DEBUG]  \n and State is 34\n");
      char* lex = duplicateSubstring(lexemeStart, forward);
      if (c == '\n')
        fillToken(t, TK_COMMENT, lex, lineCount - 1, 0, NULL);
      else
        fillToken(t, TK_COMMENT, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 35: {
      if (lexDebug) printf("[DEBUG]  \n and State is 35\n");
      backtrackingState--;
      c = nextChar();
      if (isCharacterInRange(c, '2', '7')) {
        lexerState = 36;
      } else if (isCharacterInRange(c, 'a', 'z')) {
        lexerState = 40;
      } else {
        lexerState = 41;
      }
      break;
    }
    case 36: {
      if (lexDebug) printf("[DEBUG]  , State 36, STARTING 37\n");
      c = nextChar();
      while (isCharacterInRange(c, 'b', 'd')) c = nextChar();

      if (isCharacterInRange(c, '2', '7'))
        lexerState = 37;
      else { lexerState = 39; }
      break;
    }
    case 37: {
      c = nextChar();
      while (isCharacterInRange(c, '2', '7')) c = nextChar();

      if (!isCharacterInRange(c, '2', '7') && !isCharacterInRange(c, 'b', 'd')) {
        if (lexDebug) printf("[DEBUG]  , State 37, Going 38\n");
        lexerState = 38;
      } else {
        printf("Line %d Error: Two identifiers are not allowed back to back without a break\n", lineCount);
        errorType  = 5;
        lexerState = 55;
      }
      break;
    }
    case 38: {
      if (lexDebug) printf("[DEBUG]  , State 38\n");
      retract(1);
      int identifierLength = forward - lexemeStart + 1;
      if (identifierLength < 2) {
        printf("Line %d Error: Identifier length is less than 2\n", lineCount);
        errorType  = 4;
        lexerState = 55;
      } else if (identifierLength > 20) {
        printf("Line %d \tError: Variable Identifier is longer than the prescribed length of 20 characters.\n",
               lineCount);
        errorType  = 4;
        lexerState = 55;
      } else {
        char* lex = duplicateSubstring(lexemeStart, forward);
        if (c == '\n')
          fillToken(t, TK_ID, lex, lineCount - 1, 0, NULL);
        else
          fillToken(t, TK_ID, lex, lineCount, 0, NULL);
        accept();
        return t;
      }
      break;
    }
    case 39: {
      if (lexDebug) printf("[DEBUG]  , State 39\n");
      retract(1);
      char* lex = duplicateSubstring(lexemeStart, forward);
      if (c == '\n')
        fillToken(t, TK_ID, lex, lineCount - 1, 0, NULL);
      else
        fillToken(t, TK_ID, lex, lineCount, 0, NULL);

      accept();
      return t;
      break;
    }
    case 40: {
      if (lexDebug) printf("[DEBUG]  , State 40\n");
      c = nextChar();
      while (isCharacterInRange(c, 'a', 'z')) c = nextChar();
      lexerState = 41;
      break;
    }
    case 41: {
      // this broke something
      if (lexDebug) printf("[DEBUG]  , State 41\n");
      retract(1);
      char*        lex = duplicateSubstring(lexemeStart, forward);
      struct Node* n   = findKeyword(kt, lex);
      if (n == NULL) {
        fillToken(t, TK_FIELDID, lex, lineCount, 0, NULL);
      } else {
        fillToken(t, n->tokenName, lex, lineCount, 0, NULL);
      }
      accept();
      return t;
    }
    case 42: {
      if (lexDebug) printf("[DEBUG]  , State 42\n");
      c = nextChar();
      while (isCharacterInRange(c, '0', '9')) c = nextChar();

      if (c == '.') {
        if (lexDebug) printf("[DEBUG]  , State 42, Going 44, Looking ., REAL NUMBER\n");
        lexerState = 44;
      } else {
        lexerState = 43;
      }
      break;
    }
    case 43: {
      if (lexDebug) printf("[DEBUG]  , State 43, Looking NUMBER\n");
      retract(1);
      char* lex      = duplicateSubstring(lexemeStart, forward);
      tVal* val      = malloc(sizeof(tVal));
      val->INT_VALUE = str2int(lex);
      if (c == '\n')
        fillToken(t, TK_NUM, lex, lineCount - 1, 1, val);
      else
        fillToken(t, TK_NUM, lex, lineCount, 1, val);
      accept();
      return t;
      break;
    }
    case 44: {
      if (lexDebug) printf("[DEBUG]  , State 44\n");
      c = nextChar();
      if (isCharacterInRange(c, '0', '9')) {
        lexerState = 45;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 45: {
      if (lexDebug) printf("[DEBUG]  , State 45\n");
      c = nextChar();
      if (isCharacterInRange(c, '0', '9')) {
        lexerState = 46;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 46: {
      c = nextChar();
      if (c == 'E') {
        if (lexDebug) printf("[DEBUG]  , State 46, Going 56, Real NumPath\n");
        // handle scientific notation
        lexerState = 56;
      } else {
        retract(1);
        char* lex        = duplicateSubstring(lexemeStart, forward);
        tVal* val        = (tVal*)malloc(sizeof(tVal));
        val->FLOAT_VALUE = str2flt(lex);
        fillToken(t, TK_RNUM, lex, lineCount, 2, val);
        accept();
        return t;
      }
      break;
    }
    case 47: {
      if (lexDebug) printf("[DEBUG]  , State 47\n");
      c = nextChar();
      if (isCharacterInRange(c, 'a', 'z') || isCharacterInRange(c, 'A', 'Z')) {
        lexerState = 48;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 48: {
      if (lexDebug) printf("[DEBUG]  , State 48\n");
      c = nextChar();
      while (isCharacterInRange(c, 'a', 'z') || isCharacterInRange(c, 'A', 'Z')) c = nextChar();

      if (isCharacterInRange(c, '0', '9')) {
        lexerState = 49;
      } else {
        lexerState = 51;
      }
      break;
    }
    case 49: {
      if (lexDebug) printf("[DEBUG]  , State 49\n");
      c = nextChar();
      while (isCharacterInRange(c, '0', '9')) c = nextChar();
      lexerState = 50;
      break;
    }
    case 50: {
      retract(1);
      int identifierLength = forward - lexemeStart + 1;
      if (identifierLength > 30) {
        printf("Line %d Error: Function identifier length exceeds 30 characters\n", lineCount);
        errorType  = 4;
        lexerState = 55;
      } else {
        char* lex = duplicateSubstring(lexemeStart, forward);
        if (c == '\n')
          fillToken(t, TK_FUNID, lex, lineCount - 1, 0, NULL);
        else
          fillToken(t, TK_FUNID, lex, lineCount, 0, NULL);
        accept();
        return t;
      }
      break;
    }
    case 51: {
      if (lexDebug) printf("[DEBUG]  , State 51\n");
      retract(1);
      char*        lex = duplicateSubstring(lexemeStart, forward);
      struct Node* n   = findKeyword(kt, lex);
      if (n == NULL) {
        if (c == '\n')
          fillToken(t, TK_FUNID, lex, lineCount - 1, 0, NULL);
        else
          fillToken(t, TK_FUNID, lex, lineCount, 0, NULL);
      } else {
        if (c == '\n')
          fillToken(t, n->tokenName, lex, lineCount - 1, 0, NULL);
        else
          fillToken(t, n->tokenName, lex, lineCount, 0, NULL);
      }
      accept();
      return t;
      break;
    }
    case 52: {
      if (lexDebug) printf("[DEBUG]  , State 52\n");
      c = nextChar();
      if (isCharacterInRange(c, 'a', 'z')) {
        lexerState = 53;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 53: {
      if (lexDebug) printf("[DEBUG]  , State 53\n");
      c = nextChar();
      while (isCharacterInRange(c, 'a', 'z')) c = nextChar();

      lexerState = 54;
      break;
    }
    case 54: {
      if (lexDebug) printf("[DEBUG]  , State 54\n");
      retract(1);
      char* lex = duplicateSubstring(lexemeStart, forward);
      if (c == '\n')
        fillToken(t, TK_RUID, lex, lineCount - 1, 0, NULL);
      else
        fillToken(t, TK_RUID, lex, lineCount, 0, NULL);
      accept();
      return t;
      break;
    }
    case 55: {
      char* lex = duplicateSubstring(lexemeStart, forward);
      if (errorType == 11 && c == '\n')
        fillToken(t, TK_ERR, lex, lineCount - 1, errorType, NULL);
      else
        fillToken(t, TK_ERR, lex, lineCount, errorType, NULL);
      accept();
      return t;
      break;
    }
    case 56: {
      if (lexDebug) printf("[DEBUG]  , State 56\n");
      c = nextChar();
      if (c == '+' || c == '-' || isCharacterInRange(c, '0', '9')) {
        if (c == '+' || c == '-') {
          lexerState = 57; // Read sign, now need digits
        } else {
          // First digit of exponent
          retract(1);
          lexerState = 57;
        }
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    case 57: {
      if (lexDebug) printf("[DEBUG]  , State 57\n");
      // Read first exponent digit
      c = nextChar();
      if (isCharacterInRange(c, '0', '9')) {
        lexerState = 58; // Read first digit, now need second digit
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }

    case 58: {
      if (lexDebug) printf("[DEBUG]  , State 58\n");
      // Read second exponent digit
      c = nextChar();
      if (isCharacterInRange(c, '0', '9')) {
        // Successfully parsed scientific notation
        char* lex        = duplicateSubstring(lexemeStart, forward);
        tVal* val        = (tVal*)malloc(sizeof(tVal));
        val->FLOAT_VALUE = str2flt(lex);
        fillToken(t, TK_RNUM, lex, lineCount, 2, val);
        accept();
        return t;
      } else {
        char* pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
        printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
        free(pattern);
        errorType  = 11;
        lexerState = 55;
        retract(1);
      }
      break;
    }
    }
  }
}

void removeComments(char* testCaseFile, char* cleanFile) {
  int tcf = open(testCaseFile, O_RDONLY);
  setupBuffers(tcf);
  int  check = 0;
  char c;
  while ((c = nextChar()) != EOF) {
    switch (check) {
    case 0: {
      if (c == ' ' || c == '\f' || c == '\r' || c == '\t' || c == '\v') {
        write(1, &c, 1);
        check = 0;
      } else if (c == '%') {
        check = 3;
      } else if (c == '\n') {
        write(1, &c, 1);
        check = 0;
      } else {
        write(1, &c, 1);
        check = 2;
      }
      break;
    }
    case 2: {
      write(1, &c, 1);
      if (c == '\n') check = 0;
      break;
    }
    case 3: {
      if (c == '\n') {
        write(1, &c, 1);
        check = 0;
      }
      break;
    }
    }
  }
  close(tcf);
}
