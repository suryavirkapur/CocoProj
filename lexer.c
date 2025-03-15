#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "constants.h"
#include "utils.h"
#include "lexer.h"

char *buffPair[2];
int bufferToBeLoaded = 0;
int currentBuffer = 1;
int inputExhausted = 0;
int fp;
char *lexemeStart = NULL;
char *forward = NULL;
int lexerState = 0;
int lineCount = 1;
int nextCharacterReadAfterRetraction = 0;

struct Node
{
    TokenName TOKEN_NAME;
    char *LEXEME;
    struct Node *next;
};

struct KeywordNode
{
    struct Node *keywords;
};

struct KeywordTable
{
    struct KeywordNode *KEYWORDS;
};

struct KeywordTable *kt;

struct Node *addNode(struct Node *head, TokenName token, char *text)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL)
    {
        return NULL; // Handle memory allocation failure
    }

    newNode->LEXEME = text;
    newNode->TOKEN_NAME = token;
    newNode->next = head;

    return newNode;
}

void insertKeyword(struct KeywordTable *table, TokenName token, char *text)
{
    int index = computeHash(text);
    table->KEYWORDS[index].keywords = addNode(table->KEYWORDS[index].keywords, token, text);
}

struct Node *findKeyword(struct KeywordTable *hashTable, const char *keyword)
{
    uint32_t bucketIndex = computeHash(keyword);
    struct Node *currentNode = hashTable->KEYWORDS[bucketIndex].keywords;

    while (currentNode != NULL)
    {
        if (strcmp(keyword, currentNode->LEXEME) == 0)
        {
            return currentNode;
        }
        currentNode = currentNode->next;
    }
    return NULL;
}

struct KeywordTable *createKeywordTable()
{
    struct KeywordTable *hashTable = (struct KeywordTable *)malloc(sizeof(struct KeywordTable));
    if (hashTable == NULL)
    {
        return NULL; // Handle memory allocation failure
    }

    hashTable->KEYWORDS = (struct KeywordNode *)malloc(NUM_KEYWORDS * sizeof(struct KeywordNode));
    if (hashTable->KEYWORDS == NULL)
    {
        free(hashTable); // Free previously allocated memory
        return NULL;     // Handle memory allocation failure
    }

    for (int i = 0; i < NUM_KEYWORDS; i++)
    {
        hashTable->KEYWORDS[i].keywords = NULL;
    }

    // Initialize keywords (using an array of structs for clarity)
    struct
    {
        TokenName token;
        const char *keyword;
    } keywordData[] = {
        {TK_WITH, "with"},
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
    for (int i = 0; i < numKeywords; i++)
    {
        insertKeyword(hashTable, keywordData[i].token, (char *)keywordData[i].keyword); // Cast to char* is safe here, since insertKeyword does not modify it.
    }

    return hashTable;
}

void initializeBuffers(int f)
{
    fp = f;
    buffPair[0] = (char *)malloc(BUFFER_SIZE * sizeof(char));
    buffPair[1] = (char *)malloc(BUFFER_SIZE * sizeof(char));
    lexemeStart = NULL;
    forward = NULL;
    currentBuffer = 1;
    bufferToBeLoaded = 0;
    inputExhausted = 0;
    lineCount = 1;
    nextCharacterReadAfterRetraction = 0;
    lexerState = 0;
}

void initializeLexer(int f)
{
    initializeBuffers(f);
    kt = createKeywordTable();
}

int getInputStream()
{
    if (inputExhausted != 0)
        return EOF;

    memset(buffPair[bufferToBeLoaded], EOF, BUFFER_SIZE);

    int res = read(fp, buffPair[bufferToBeLoaded], BUFFER_SIZE);
    if (res == 0 || res < BUFFER_SIZE)
    {
        inputExhausted = 1;
    }

    bufferToBeLoaded = 1 - bufferToBeLoaded;
    currentBuffer = 1 - currentBuffer;
    if (res == -1)
    {
        printf("Error: Input Buffers failed to be loaded\n");
        return -1;
    }
    return res;
}

char nextChar()
{
    if (lexemeStart == NULL && forward == NULL)
    {
        int res = getInputStream();
        if (res == -1)
        {
            return EOF;
        }
        lexemeStart = buffPair[currentBuffer];
        forward = buffPair[currentBuffer];
        char *curr_forward = forward;
        forward++;

        if (nextCharacterReadAfterRetraction == 0 && *curr_forward == '\n')
        {
            lineCount++;
        }

        if (nextCharacterReadAfterRetraction == 1)
            nextCharacterReadAfterRetraction = 0;

        return *curr_forward;
    }

    char *curr_forward = forward;

    if (curr_forward - buffPair[currentBuffer] == BUFFER_SIZE - 1)
    {
        int res = getInputStream();
        if (res == -1)
        {
            return EOF;
        }
        forward = buffPair[currentBuffer];
    }
    else if (*curr_forward == EOF)
    {
        return EOF;
    }
    else
        forward++;

    if (nextCharacterReadAfterRetraction == 0 && *curr_forward == '\n')
    {
        lineCount++;
    }

    if (nextCharacterReadAfterRetraction == 1)
        nextCharacterReadAfterRetraction = 0;

    return *curr_forward;
}

void accept()
{
    lexemeStart = forward;
}

void retract(int amt)
{
    while (amt > 0)
    {
        --forward;
        --amt;
    }

    nextCharacterReadAfterRetraction = 1;
}

Token *populateToken(Token *t, TokenName tokenName, char *lexeme, int lineNumber, int isNumber, Value *value)
{
    t->TOKEN_NAME = tokenName;
    t->LINE_NO = lineNumber;
    t->IS_NUMBER = isNumber;
    t->LEXEME = lexeme;
    t->VALUE = value;
    return t;
}

int stringToInteger(char *str)
{
    int num;
    sscanf(str, "%d", &num);
    return num;
}

float stringToFloat(char *str)
{
    float f;
    sscanf(str, "%f", &f);
    return f;
}

Token *getToken()
{
    lexerState = 0;
    char c = 1;
    Token *t = (Token *)malloc(sizeof(Token));
    int errorType;

    while (1)
    {
        if (c == EOF)
            return NULL;

        switch (lexerState)
        {
        case 0:
        {
            c = nextChar();
            if (isCharacterEqualTo(c, '<'))
            {
                lexerState = 16;
            }
            else if (isCharacterEqualTo(c, '#'))
            {
                lexerState = 52;
            }
            else if (isCharacterInRange(c, 'b', 'd'))
            {
                lexerState = 35;
            }
            else if (isCharacterEqualTo(c, 'a') || isCharacterInRange(c, 'e', 'z'))
            {
                lexerState = 40;
            }
            else if (isCharacterEqualTo(c, '_'))
            {
                lexerState = 47;
            }
            else if (isCharacterEqualTo(c, '['))
            {
                lexerState = 7;
            }
            else if (isCharacterEqualTo(c, ']'))
            {
                lexerState = 8;
            }
            else if (isCharacterEqualTo(c, ','))
            {
                lexerState = 9;
            }
            else if (isCharacterEqualTo(c, ';'))
            {
                lexerState = 12;
            }
            else if (isCharacterEqualTo(c, ':'))
            {
                lexerState = 11;
            }
            else if (isCharacterEqualTo(c, '.'))
            {
                lexerState = 10;
            }
            else if (isCharacterEqualTo(c, '('))
            {
                lexerState = 5;
            }
            else if (isCharacterEqualTo(c, ')'))
            {
                lexerState = 6;
            }
            else if (isCharacterEqualTo(c, '+'))
            {
                lexerState = 1;
            }
            else if (isCharacterEqualTo(c, '-'))
            {
                lexerState = 2;
            }
            else if (isCharacterEqualTo(c, '*'))
            {
                lexerState = 3;
            }
            else if (isCharacterEqualTo(c, '/'))
            {
                lexerState = 4;
            }
            else if (isCharacterEqualTo(c, '~'))
            {
                lexerState = 13;
            }
            else if (isCharacterEqualTo(c, '!'))
            {
                lexerState = 14;
            }
            else if (isCharacterEqualTo(c, '>'))
            {
                lexerState = 22;
            }
            else if (isCharacterEqualTo(c, '='))
            {
                lexerState = 25;
            }
            else if (isCharacterEqualTo(c, '@'))
            {
                lexerState = 27;
            }
            else if (isCharacterEqualTo(c, '&'))
            {
                lexerState = 30;
            }
            else if (isCharacterEqualTo(c, '%'))
            {
                lexerState = 33;
            }
            else if (isCharacterInRange(c, '0', '9'))
            {
                lexerState = 42;
            }
            else if (isCharacterEqualTo(c, ' ') || isCharacterEqualTo(c, '\f') || isCharacterEqualTo(c, '\r') || isCharacterEqualTo(c, '\t') || isCharacterEqualTo(c, '\v'))
            {
                lexemeStart++;
                lexerState = 0;
            }
            else if (isCharacterEqualTo(c, '\n'))
            {
                lexemeStart++;
                lexerState = 0;
            }
            else if (isCharacterEqualTo(c, EOF))
            {
                return NULL;
            }
            else
            {
                printf("Line %d Error: Unknown Symbol <%c>\n", lineCount, c);
                errorType = 6;
                lexerState = 55;
            }
            break;
        }
        case 1:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_PLUS, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 2:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_MINUS, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 3:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_MUL, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 4:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_DIV, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 5:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_OP, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 6:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_CL, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 7:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_SQL, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 8:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_SQR, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 9:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_COMMA, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 10:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_DOT, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 11:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_COLON, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 12:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_SEM, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 13:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_NOT, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 14:
        {
            c = nextChar();
            if (c == '=')
            {
                lexerState = 15;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 15:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_NE, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 16:
        {
            c = nextChar();
            if (c == '-')
            {
                lexerState = 17;
            }
            else if (c == '=')
            {
                lexerState = 20;
            }
            else
            {
                lexerState = 21;
            }
            break;
        }
        case 17:
        {
            c = nextChar();
            if (c == '-')
            {
                lexerState = 18;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 18:
        {
            c = nextChar();
            if (c == '-')
            {
                lexerState = 19;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 19:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_ASSIGNOP, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 20:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_LE, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 21:
        {
            retract(1);
            char *lex = duplicateSubstring(lexemeStart, forward);

            if (c == '\n')
                populateToken(t, TK_LT, lex, lineCount - 1, 0, NULL);
            else
                populateToken(t, TK_LT, lex, lineCount, 0, NULL);

            accept();
            return t;
            break;
        }
        case 22:
        {
            c = nextChar();
            if (c == '=')
            {
                lexerState = 23;
            }
            else
            {
                lexerState = 24;
            }
            break;
        }
        case 23:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_GE, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 24:
        {
            retract(1);
            char *lex = duplicateSubstring(lexemeStart, forward);
            if (c == '\n')
                populateToken(t, TK_GT, lex, lineCount - 1, 0, NULL);
            else
                populateToken(t, TK_GT, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 25:
        {
            c = nextChar();
            if (c == '=')
            {
                lexerState = 26;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 26:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_EQ, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 27:
        {
            c = nextChar();
            if (c == '@')
            {
                lexerState = 28;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 28:
        {
            c = nextChar();
            if (c == '@')
            {
                lexerState = 29;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 29:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_OR, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 30:
        {
            c = nextChar();
            if (c == '&')
            {
                lexerState = 31;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 31:
        {
            c = nextChar();
            if (c == '&')
            {
                lexerState = 32;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 32:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            populateToken(t, TK_AND, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 33:
        {
            c = nextChar();
            while (c != '\n' && c != EOF)
            {
                c = nextChar();
            }
            lexerState = 34;
            break;
        }
        case 34:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            if (c == '\n')
                populateToken(t, TK_COMMENT, lex, lineCount - 1, 0, NULL);
            else
                populateToken(t, TK_COMMENT, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 35:
        {
            c = nextChar();
            if (isCharacterInRange(c, '2', '7'))
            {
                lexerState = 36;
            }
            else if (isCharacterInRange(c, 'a', 'z'))
            {
                lexerState = 40;
            }
            else
            {
                lexerState = 41;
            }
            break;
        }
        case 36:
        {
            c = nextChar();
            while (isCharacterInRange(c, 'b', 'd'))
                c = nextChar();

            if (isCharacterInRange(c, '2', '7'))
                lexerState = 37;
            else
            {
                lexerState = 39;
            }
            break;
        }
        case 37:
        {
            c = nextChar();
            while (isCharacterInRange(c, '2', '7'))
                c = nextChar();

            if (!isCharacterInRange(c, '2', '7') && !isCharacterInRange(c, 'b', 'd'))
            {
                lexerState = 38;
            }
            else
            {
                printf("Line %d Error: Two identifiers are not allowed back to back without a break\n", lineCount);
                errorType = 5;
                lexerState = 55;
            }
            break;
        }
        case 38:
        {
            retract(1);
            int identifierLength = forward - lexemeStart + 1;
            if (identifierLength < 2)
            {
                printf("Line %d Error: Identifier length is less than 2\n", lineCount);
                errorType = 4;
                lexerState = 55;
            }
            else if (identifierLength > 20)
            {
                printf("Line %d \tError: Variable Identifier is longer than the prescribed length of 20 characters.\n", lineCount);
                errorType = 4;
                lexerState = 55;
            }
            else
            {
                char *lex = duplicateSubstring(lexemeStart, forward);
                if (c == '\n')
                    populateToken(t, TK_ID, lex, lineCount - 1, 0, NULL);
                else
                    populateToken(t, TK_ID, lex, lineCount, 0, NULL);
                accept();
                return t;
            }
            break;
        }
        case 39:
        {
            retract(1);
            char *lex = duplicateSubstring(lexemeStart, forward);
            if (c == '\n')
                populateToken(t, TK_ID, lex, lineCount - 1, 0, NULL);
            else
                populateToken(t, TK_ID, lex, lineCount, 0, NULL);

            accept();
            return t;
            break;
        }
        case 40:
        {
            c = nextChar();
            while (isCharacterInRange(c, 'a', 'z'))
                c = nextChar();

            // We've now read all lowercase letters and hit a non-letter character
            lexerState = 41;
            break;
        }
        case 41:
        {
            retract(1); // Retract to handle the non-letter character
            char *lex = duplicateSubstring(lexemeStart, forward);

            // Check if it's a keyword regardless of surrounding whitespace
            struct Node *n = findKeyword(kt, lex);
            if (n == NULL)
            {
                // Not a keyword, handle as FIELDID
                populateToken(t, TK_FIELDID, lex, lineCount, 0, NULL);
            }
            else
            {
                // It's a keyword (like "end"), set the appropriate token
                populateToken(t, n->TOKEN_NAME, lex, lineCount, 0, NULL);
            }
            accept();
            return t;
        }
        case 42:
        {
            c = nextChar();
            while (isCharacterInRange(c, '0', '9'))
                c = nextChar();

            if (c == '.')
            {
                lexerState = 44;
            }
            else
            {
                lexerState = 43;
            }
            break;
        }
        case 43:
        {
            retract(1);
            char *lex = duplicateSubstring(lexemeStart, forward);
            Value *val = malloc(sizeof(Value));
            val->INT_VALUE = stringToInteger(lex);
            if (c == '\n')
                populateToken(t, TK_NUM, lex, lineCount - 1, 1, val);
            else
                populateToken(t, TK_NUM, lex, lineCount, 1, val);
            accept();
            return t;
            break;
        }
        case 44:
        {
            c = nextChar();
            if (isCharacterInRange(c, '0', '9'))
            {
                lexerState = 45;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 45:
        {
            c = nextChar();
            if (isCharacterInRange(c, '0', '9'))
            {
                lexerState = 46;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 46:
        {
            c = nextChar();
            if (c == 'E')
            {
                // Handle scientific notation
                lexerState = 56; // New state for exponent part
            }
            else
            {
                // Normal real number without exponent
                retract(1);
                char *lex = duplicateSubstring(lexemeStart, forward);
                Value *val = (Value *)malloc(sizeof(Value));
                val->FLOAT_VALUE = stringToFloat(lex);
                populateToken(t, TK_RNUM, lex, lineCount, 2, val);
                accept();
                return t;
            }
            break;
        }
        case 47:
        {
            c = nextChar();
            if (isCharacterInRange(c, 'a', 'z') || isCharacterInRange(c, 'A', 'Z'))
            {
                lexerState = 48;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 48:
        {
            c = nextChar();
            while (isCharacterInRange(c, 'a', 'z') || isCharacterInRange(c, 'A', 'Z'))
                c = nextChar();

            if (isCharacterInRange(c, '0', '9'))
            {
                lexerState = 49;
            }
            else
            {
                lexerState = 51;
            }
            break;
        }
        case 49:
        {
            c = nextChar();
            while (isCharacterInRange(c, '0', '9'))
                c = nextChar();

            lexerState = 50;

            break;
        }
        case 50:
        {
            retract(1);
            int identifierLength = forward - lexemeStart + 1;
            if (identifierLength > 30)
            {
                printf("Line %d Error: Function identifier length exceeds 30 characters\n", lineCount);
                errorType = 4;
                lexerState = 55;
            }
            else
            {
                char *lex = duplicateSubstring(lexemeStart, forward);
                if (c == '\n')
                    populateToken(t, TK_FUNID, lex, lineCount - 1, 0, NULL);
                else
                    populateToken(t, TK_FUNID, lex, lineCount, 0, NULL);
                accept();
                return t;
            }
            break;
        }
        case 51:
        {
            retract(1);
            char *lex = duplicateSubstring(lexemeStart, forward);
            struct Node *n = findKeyword(kt, lex);
            if (n == NULL)
            {
                if (c == '\n')
                    populateToken(t, TK_FUNID, lex, lineCount - 1, 0, NULL);
                else
                    populateToken(t, TK_FUNID, lex, lineCount, 0, NULL);
            }
            else
            {
                if (c == '\n')
                    populateToken(t, n->TOKEN_NAME, lex, lineCount - 1, 0, NULL);
                else
                    populateToken(t, n->TOKEN_NAME, lex, lineCount, 0, NULL);
            }
            accept();
            return t;
            break;
        }
        case 52:
        {
            c = nextChar();
            if (isCharacterInRange(c, 'a', 'z'))
            {
                lexerState = 53;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        case 53:
        {
            c = nextChar();
            while (isCharacterInRange(c, 'a', 'z'))
                c = nextChar();

            lexerState = 54;
            break;
        }
        case 54:
        {
            retract(1);
            char *lex = duplicateSubstring(lexemeStart, forward);
            if (c == '\n')
                populateToken(t, TK_RUID, lex, lineCount - 1, 0, NULL);
            else
                populateToken(t, TK_RUID, lex, lineCount, 0, NULL);
            accept();
            return t;
            break;
        }
        case 55:
        {
            char *lex = duplicateSubstring(lexemeStart, forward);
            if (errorType == 3 && c == '\n')
                populateToken(t, TK_ERR, lex, lineCount - 1, errorType, NULL);
            else
                populateToken(t, TK_ERR, lex, lineCount, errorType, NULL);
            accept();
            return t;
            break;
        }
        case 56:
        {
            c = nextChar();
            if (c == '+' || c == '-' || isCharacterInRange(c, '0', '9'))
            {
                if (c == '+' || c == '-')
                {
                    lexerState = 57; // Read sign, now need digits
                }
                else
                {
                    // First digit of exponent
                    retract(1);
                    lexerState = 57;
                }
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }

        case 57:
        {
            // Read first exponent digit
            c = nextChar();
            if (isCharacterInRange(c, '0', '9'))
            {
                lexerState = 58; // Read first digit, now need second digit
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }

        case 58:
        {
            // Read second exponent digit
            c = nextChar();
            if (isCharacterInRange(c, '0', '9'))
            {
                // Successfully parsed scientific notation
                char *lex = duplicateSubstring(lexemeStart, forward);
                Value *val = (Value *)malloc(sizeof(Value));
                val->FLOAT_VALUE = stringToFloat(lex);
                populateToken(t, TK_RNUM, lex, lineCount, 2, val);
                accept();
                return t;
            }
            else
            {
                char *pattern = duplicateSubstring(lexemeStart, forward - sizeof(char));
                printf("Line %d Error: Unknown pattern <%s>\n", lineCount, pattern);
                free(pattern);
                errorType = 3;
                lexerState = 55;
                retract(1);
            }
            break;
        }
        }
    }
}

void removeComments(char *testCaseFile, char *cleanFile)
{
    int tcf = open(testCaseFile, O_RDONLY);
    initializeBuffers(tcf);
    int check = 0;
    char c;
    while ((c = nextChar()) != EOF)
    {
        switch (check)
        {
        case 0:
        {
            if (c == ' ' || c == '\f' || c == '\r' || c == '\t' || c == '\v')
            {
                write(1, &c, 1);
                check = 0;
            }
            else if (c == '%')
            {
                check = 3;
            }
            else if (c == '\n')
            {
                write(1, &c, 1);
                check = 0;
            }
            else
            {
                write(1, &c, 1);
                check = 2;
            }
            break;
        }
        case 2:
        {
            write(1, &c, 1);
            if (c == '\n')
                check = 0;
            break;
        }
        case 3:
        {
            if (c == '\n')
            {
                write(1, &c, 1);
                check = 0;
            }
            break;
        }
        }
    }
    close(tcf);
}

void printBuffers()
{
    char c;
    while ((c = nextChar()) != EOF)
    {
        printf("%c ", c);
    }
    printf("\n");
}