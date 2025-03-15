/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

#include "grammar.h"
#include "constants.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Grammar*                 parsedGrammar;
struct NonTerminalRuleRecords** nonTerminalRuleRecords;
bool                            nonTerminalProcessed[NUM_NONTERMINALS] = {0};
int                             symbolVectorSize = NUM_TERMINALS + 1;

bool syntaxErrorOccurred;
bool lexicalErrorOccurred;

char* TerminalMap[] = {
    "TK_ASSIGNOP",   // <---
    "TK_COMMENT",    // %
    "TK_FIELDID",    // [a-z][a-z]*
    "TK_ID",         // [b-d][2-7][b-d]*[2-7]*
    "TK_NUM",        // [0-9][0-9]*
    "TK_RNUM",       // E or e or . or +|- or digit
    "TK_FUNID",      // _[a-z|A-Z][a-z|A-Z]*[0-9]*
    "TK_RUID",       // #[a-z][a-z]*
    "TK_WITH",       // with
    "TK_PARAMETERS", // parameters
    "TK_END",        // end
    "TK_WHILE",      // while
    "TK_UNION",      // union
    "TK_ENDUNION",   // endunion
    "TK_DEFINETYPE", // definetype
    "TK_AS",         // as
    "TK_TYPE",       // type
    "TK_MAIN",       // _main
    "TK_GLOBAL",     // global
    "TK_PARAMETER",  // parameter
    "TK_LIST",       // list
    "TK_SQL",        // [
    "TK_SQR",        // ]
    "TK_INPUT",      // input
    "TK_OUTPUT",     // output
    "TK_INT",        // int
    "TK_REAL",       // real
    "TK_COMMA",      // ,
    "TK_SEM",        // ;
    "TK_COLON",      // :
    "TK_DOT",        // .
    "TK_ENDWHILE",   // endwhile
    "TK_OP",         // (
    "TK_CL",         // )
    "TK_IF",         // if
    "TK_THEN",       // then
    "TK_ENDIF",      // endif
    "TK_READ",       // read
    "TK_WRITE",      // write
    "TK_RETURN",     // return
    "TK_PLUS",       // +
    "TK_MINUS",      // -
    "TK_MUL",        // *
    "TK_DIV",        // /
    "TK_CALL",       // call
    "TK_RECORD",     // record
    "TK_ENDRECORD",  // endrecord
    "TK_ELSE",       // else
    "TK_AND",        // &&&
    "TK_OR",         // @@@
    "TK_NOT",        // ~
    "TK_LT",         //
    "TK_LE",         // <=
    "TK_EQ",         // ==
    "TK_GT",         // >
    "TK_GE",         // >=
    "TK_NE",         // !=
    "TK_ERR",        // Error token
    "TK_EPS",        // Epsilon (empty production)
    "TK_DOLLAR",     // End marker for parsing stack
    "TK_EOF"         // End of file marker
};

char* NonTerminalMap[] = {
    "program",
    "mainFunction",
    "otherFunctions",
    "function",
    "input_par",
    "output_par",
    "parameterList",
    "dataType",
    "primitiveDatatype",
    "constructedDatatype",
    "moreParameters",
    "statementSequence",
    "typeDefinitions",
    "actualOrRedefined",
    "typeDefinition",
    "fieldDefinitions",
    "fieldDefinition",
    "fieldDataType",
    "remainingFields",
    "declarations",
    "declaration",
    "globalSpecifier",
    "otherStmts",
    "statement",
    "assignmentStmt",
    "singleOrRecId",
    "constructedVariable",
    "oneExpansion",
    "moreExpansions",
    "optionalFieldAccess",
    "funCallStmt",
    "outputParameters",
    "inputParameters",
    "iterativeStmt",
    "conditionalStmt",
    "elsePart",
    "ioStmt",
    "arithmeticExpression",
    "expPrime",
    "term",
    "termPrime",
    "factor",
    "highPrecedenceOperators",
    "lowPrecedenceOperators",
    "booleanExpression",
    "variable",
    "logicalOp",
    "relationalOp",
    "returnStmt",
    "optionalReturn",
    "idList",
    "remainingIdentifiers",
    "typeAliasStatement",
    "recordOrUnion"
};

int findInTerminalMap(char* str) {
    if (str == NULL) return -1;
    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (TerminalMap[i] != NULL && strcmp(str, TerminalMap[i]) == 0)
            return i;
    }
    return -1;
}

int findInNonTerminalMap(char* str) {
    if (str == NULL) return -1;
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (NonTerminalMap[i] != NULL && strcmp(str, NonTerminalMap[i]) == 0)
            return i;
    }
    return -1;
}

char* getTerminal(int mapIndex) {
    return TerminalMap[mapIndex];
}

char* getNonTerminal(int mapIndex) {
    return NonTerminalMap[mapIndex];
}

int setupGrammar() {
    parsedGrammar = (struct Grammar*)malloc(sizeof(struct Grammar));
    if (parsedGrammar == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed for grammar\n");
        return -1;
    }
    parsedGrammar->GRAMMAR_RULES_SIZE = NUM_GRAMMAR_RULES + 1;
    parsedGrammar->GRAMMAR_RULES = (struct Rule**)malloc(sizeof(struct Rule*) * parsedGrammar->GRAMMAR_RULES_SIZE);
    if (parsedGrammar->GRAMMAR_RULES == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed for grammar rules\n");
        free(parsedGrammar);
        return -1;
    }
    parsedGrammar->GRAMMAR_RULES[0] = NULL;
    return 0;
}

struct Rule* setupRule(struct SymbolList* sl, int ruleCount) {
    struct Rule* rule = (struct Rule*)malloc(sizeof(struct Rule));
    if (rule == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed for rule\n");
        return NULL;
    }
    rule->symbols = sl;
    rule->ruleNum = ruleCount;
    return rule;
}

struct NonTerminalRuleRecords** setupNonTerminalRecords() {
    struct NonTerminalRuleRecords** records = (struct NonTerminalRuleRecords**)malloc(sizeof(struct NonTerminalRuleRecords*) * NUM_NONTERMINALS);
    if (records == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed for NonTerminalRuleRecords\n");
        exit(EXIT_FAILURE);
    }
    return records;
}

void initialiseCheckOnDone() {
    for (int i = 0; i < NUM_NONTERMINALS; i++)
        nonTerminalProcessed[i] = false;
}

char* trimWhitespace(char* str) {
    if (str == NULL)
        return NULL;
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    return str;
}

char** splitString(char* str, const char* delimiter, int* count) {
    if (str == NULL || delimiter == NULL || count == NULL) {
        fprintf(stderr, "ERROR: NULL parameter in splitString\n");
        return NULL;
    }
    char** result = NULL;
    char* token = strtok(str, delimiter);
    *count = 0;
    while (token != NULL) {
        result = realloc(result, sizeof(char*) * (*count + 1));
        if (result == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        result[*count] = trimWhitespace(strdup(token));
        (*count)++;
        token = strtok(NULL, delimiter);
    }
    return result;
}

struct Grammar* extractGrammar() {
    int ruleCount = 1;
    int fd = open(GRAMMAR_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Failed to open grammar file %s\n", GRAMMAR_FILE_PATH);
        exit(EXIT_FAILURE);
    }

    char c;
    int actualRead;
    /* Use a dynamic buffer for the current symbol with an initial capacity */
    size_t bufSize = 64;
    char* symbol = (char*)malloc(bufSize);
    if (symbol == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
    symbol[0] = '\0';

    int symbolsRead = 0;
    struct Symbol* currentNonTerminal = NULL;
    struct SymbolList* sl = NULL;

    setupGrammar();
    nonTerminalRuleRecords = setupNonTerminalRecords();
    initialiseCheckOnDone();

    while ((actualRead = read(fd, &c, sizeof(char))) > 0) {
        if (c == ' ' || c == '\t') {
            if (strlen(symbol) > 0) {
                symbolsRead++;
                struct Symbol* s;
                if (strcmp(symbol, "eps") == 0) {
                    s = (struct Symbol*)malloc(sizeof(struct Symbol));
                    if (s == NULL) {
                        fprintf(stderr, "ERROR: Memory allocation failed\n");
                        free(symbol);
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                    s->symType.TERMINAL = TK_EPS;
                    s->isTerminal = true;
                    s->next = NULL;
                } else {
                    s = initializeSymbol(symbol);
                    if (s == NULL) {
                        fprintf(stderr, "ERROR: Failed to initialize symbol %s\n", symbol);
                        free(symbol);
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                }
                if (sl == NULL) {
                    sl = initializeSymbolList();
                    if (sl == NULL) {
                        fprintf(stderr, "ERROR: Failed to initialize symbol list\n");
                        free(symbol);
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                }
                addToSymbolList(sl, s);

                if (symbolsRead == 1) {
                    if (currentNonTerminal == NULL) {
                        if (s->isTerminal == true) {
                            fprintf(stderr, "ERROR: First symbol in rule cannot be a terminal: %s\n", symbol);
                            free(symbol);
                            close(fd);
                            exit(EXIT_FAILURE);
                        }
                        nonTerminalRuleRecords[s->symType.NON_TERMINAL] =
                            (struct NonTerminalRuleRecords*)malloc(sizeof(struct NonTerminalRuleRecords));
                        if (nonTerminalRuleRecords[s->symType.NON_TERMINAL] == NULL) {
                            fprintf(stderr, "ERROR: Memory allocation failed for NonTerminalRuleRecords\n");
                            free(symbol);
                            close(fd);
                            exit(EXIT_FAILURE);
                        }
                        nonTerminalRuleRecords[s->symType.NON_TERMINAL]->start = ruleCount;
                    } else if (currentNonTerminal->symType.NON_TERMINAL != s->symType.NON_TERMINAL) {
                        nonTerminalRuleRecords[currentNonTerminal->symType.NON_TERMINAL]->end = ruleCount - 1;
                        nonTerminalRuleRecords[s->symType.NON_TERMINAL] =
                            (struct NonTerminalRuleRecords*)malloc(sizeof(struct NonTerminalRuleRecords));
                        if (nonTerminalRuleRecords[s->symType.NON_TERMINAL] == NULL) {
                            fprintf(stderr, "ERROR: Memory allocation failed for NonTerminalRuleRecords\n");
                            free(symbol);
                            close(fd);
                            exit(EXIT_FAILURE);
                        }
                        nonTerminalRuleRecords[s->symType.NON_TERMINAL]->start = ruleCount;
                    }
                    currentNonTerminal = s;
                }
                /* Reset the symbol buffer instead of freeing and reallocating */
                symbol[0] = '\0';
            }
        } else if (c == '\n' || c == '\r') {
            if (strlen(symbol) > 0) {
                struct Symbol* s;
                if (strcmp(symbol, "eps") == 0) {
                    s = (struct Symbol*)malloc(sizeof(struct Symbol));
                    if (s == NULL) {
                        fprintf(stderr, "ERROR: Memory allocation failed\n");
                        free(symbol);
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                    s->symType.TERMINAL = TK_EPS;
                    s->isTerminal = true;
                    s->next = NULL;
                } else {
                    s = initializeSymbol(symbol);
                    if (s == NULL) {
                        fprintf(stderr, "ERROR: Failed to initialize symbol %s\n", symbol);
                        free(symbol);
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                }
                if (sl == NULL) {
                    sl = initializeSymbolList();
                    if (sl == NULL) {
                        fprintf(stderr, "ERROR: Failed to initialize symbol list\n");
                        free(symbol);
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                }
                addToSymbolList(sl, s);
                struct Rule* rule = setupRule(sl, ruleCount);
                if (rule == NULL) {
                    fprintf(stderr, "ERROR: Failed to initialize rule\n");
                    free(symbol);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                parsedGrammar->GRAMMAR_RULES[ruleCount] = rule;
                ruleCount++;
                symbolsRead = 0;
                symbol[0] = '\0';
                sl = NULL;
            }
            if (c == '\r') {
                char nextChar;
                if (read(fd, &nextChar, sizeof(char)) > 0 && nextChar != '\n') {
                    lseek(fd, -1, SEEK_CUR);
                }
            }
        } else {
            if (symbolsRead == 0 && sl == NULL) {
                sl = initializeSymbolList();
                if (sl == NULL) {
                    fprintf(stderr, "ERROR: Failed to initialize symbol list\n");
                    free(symbol);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
            size_t len = strlen(symbol);
            /* Expand the buffer if necessary */
            if (len + 2 > bufSize) {
                bufSize *= 2;
                char* temp = realloc(symbol, bufSize);
                if (temp == NULL) {
                    fprintf(stderr, "ERROR: Memory allocation failed\n");
                    free(symbol);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                symbol = temp;
            }
            symbol[len] = c;
            symbol[len + 1] = '\0';
        }
    }
    if (symbol != NULL && strlen(symbol) > 0) {
        struct Symbol* s;
        if (strcmp(symbol, "eps") == 0) {
            s = (struct Symbol*)malloc(sizeof(struct Symbol));
            if (s != NULL) {
                s->symType.TERMINAL = TK_EPS;
                s->isTerminal = true;
                s->next = NULL;
                if (sl != NULL) {
                    addToSymbolList(sl, s);
                    struct Rule* rule = setupRule(sl, ruleCount);
                    if (rule != NULL) {
                        parsedGrammar->GRAMMAR_RULES[ruleCount] = rule;
                        ruleCount++;
                    }
                }
            }
        } else {
            s = initializeSymbol(symbol);
            if (s != NULL && sl != NULL) {
                addToSymbolList(sl, s);
                struct Rule* rule = setupRule(sl, ruleCount);
                if (rule != NULL) {
                    parsedGrammar->GRAMMAR_RULES[ruleCount] = rule;
                    ruleCount++;
                }
            }
        }
    }
    free(symbol);
    close(fd);
    if (currentNonTerminal != NULL) {
        nonTerminalRuleRecords[currentNonTerminal->symType.NON_TERMINAL]->end = ruleCount - 1;
    }
    return parsedGrammar;
}
