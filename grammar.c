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
int                             symbolVectorSize                       = NUM_TERMINALS + 1;

bool syntaxErrorOccurred;
bool lexicalErrorOccurred;

char* TerminalID[] = {
    "TK_ASSIGNOP",   // <---
    "TK_COMMENT",    // %
    "TK_FIELDID",    // [a-z][a-z]*
    "TK_ID",         // [b-d][2-7][b-d]*[2-7]*
    "TK_NUM",        // [0-9][0-9]*
    "TK_RNUM",       // Real numbers (both formats)
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

char* NonTerminalID[] = {"program",
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
                         "recordOrUnion"};

int findInTerminalMap(char* str) {
    if (str == NULL) return -1;

    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (TerminalID[i] != NULL && strcmp(str, TerminalID[i]) == 0) { return i; }
    }

    return -1;
}

int findInNonTerminalMap(char* str) {
    if (str == NULL) return -1;

    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (NonTerminalID[i] != NULL && strcmp(str, NonTerminalID[i]) == 0) return i;
    }
    return -1;
}

char* getTerminal(int enumId) {
    return TerminalID[enumId];
}

char* getNonTerminal(int enumId) {
    return NonTerminalID[enumId];
}

int initializeGrammar() {
    parsedGrammar                     = (struct Grammar*)malloc(sizeof(struct Grammar));
    parsedGrammar->GRAMMAR_RULES_SIZE = NUM_GRAMMAR_RULES + 1;
    parsedGrammar->GRAMMAR_RULES      = (struct Rule**)malloc(sizeof(struct Rule*) * parsedGrammar->GRAMMAR_RULES_SIZE);
    parsedGrammar->GRAMMAR_RULES[0]   = NULL;
    return 0;
}

struct Rule* initializeRule(struct SymbolList* sl, int ruleCount) {
    struct Rule* r = (struct Rule*)malloc(sizeof(struct Rule));
    r->SYMBOLS     = sl;
    r->RULE_NO     = ruleCount;
    return r;
}

struct NonTerminalRuleRecords** initializeNonTerminalRecords() {
    struct NonTerminalRuleRecords** nonTerminalRuleRecords =
        (struct NonTerminalRuleRecords**)malloc(sizeof(struct NonTerminalRuleRecords*) * NUM_NONTERMINALS);
    return nonTerminalRuleRecords;
}

void initialiseCheckOnDone() {
    for (int i = 0; i < NUM_NONTERMINALS; i++) nonTerminalProcessed[i] = 0;
}

char* trimWhitespace(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char** splitString(char* str, const char* delimiter, int* count) {
    char** result = NULL;
    char*  token  = strtok(str, delimiter);
    *count        = 0;

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
    int fd        = open(GRAMMAR_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        printf("ERROR: Failed to open grammar file %s\n", GRAMMAR_FILE_PATH);
        exit(1);
    }

    char  c;
    int   actualRead;
    char* symbol = (char*)malloc(1); // Initialize with empty string
    if (symbol == NULL) {
        printf("ERROR: Memory allocation failed\n");
        close(fd);
        exit(1);
    }

    symbol[0]                             = '\0';
    int                symbolsRead        = 0;
    struct Symbol*     currentNonTerminal = NULL;
    struct SymbolList* sl                 = NULL;

    initializeGrammar();
    nonTerminalRuleRecords = initializeNonTerminalRecords();
    initialiseCheckOnDone();

    while ((actualRead = read(fd, &c, sizeof(char))) > 0) {
        if (c == ' ' || c == '\t') {
            if (strlen(symbol) > 0) {
                symbolsRead++;

                // Handle symbol correctly
                struct Symbol* s;
                if (strcmp(symbol, "eps") == 0) {
                    s = (struct Symbol*)malloc(sizeof(struct Symbol));
                    if (s == NULL) {
                        printf("ERROR: Memory allocation failed\n");
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                    s->TYPE.TERMINAL = TK_EPS;
                    s->IS_TERMINAL   = 1;
                    s->next          = NULL;
                } else {
                    s = initializeSymbol(symbol);
                    if (s == NULL) {
                        printf("ERROR: Failed to initialize symbol %s\n", symbol);
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                }

                // Add to symbol list
                if (sl == NULL) {
                    sl = initializeSymbolList();
                    if (sl == NULL) {
                        printf("ERROR: Failed to initialize symbol list\n");
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                }

                addToSymbolList(sl, s);

                if (symbolsRead == 1) {
                    if (currentNonTerminal == NULL) {
                        if (s->IS_TERMINAL == 1) {
                            printf("ERROR: First symbol in rule cannot be a terminal: %s\n", symbol);
                            free(symbol);
                            close(fd);
                            exit(1);
                        }

                        nonTerminalRuleRecords[s->TYPE.NON_TERMINAL] =
                            (struct NonTerminalRuleRecords*)malloc(sizeof(struct NonTerminalRuleRecords));
                        if (nonTerminalRuleRecords[s->TYPE.NON_TERMINAL] == NULL) {
                            printf("ERROR: Memory allocation failed for NTRR\n");
                            free(symbol);
                            close(fd);
                            exit(1);
                        }
                        nonTerminalRuleRecords[s->TYPE.NON_TERMINAL]->start = ruleCount;
                    } else if (currentNonTerminal->TYPE.NON_TERMINAL != s->TYPE.NON_TERMINAL) {
                        nonTerminalRuleRecords[currentNonTerminal->TYPE.NON_TERMINAL]->end = ruleCount - 1;
                        nonTerminalRuleRecords[s->TYPE.NON_TERMINAL] =
                            (struct NonTerminalRuleRecords*)malloc(sizeof(struct NonTerminalRuleRecords));
                        if (nonTerminalRuleRecords[s->TYPE.NON_TERMINAL] == NULL) {
                            printf("ERROR: Memory allocation failed for NTRR\n");
                            free(symbol);
                            close(fd);
                            exit(1);
                        }
                        nonTerminalRuleRecords[s->TYPE.NON_TERMINAL]->start = ruleCount;
                    }
                    currentNonTerminal = s;
                }

                // Reset symbol
                free(symbol);
                symbol = (char*)malloc(1);
                if (symbol == NULL) {
                    printf("ERROR: Memory allocation failed\n");
                    close(fd);
                    exit(1);
                }
                symbol[0] = '\0';
            }
        } else if (c == '\n' || c == '\r') {
            if (strlen(symbol) > 0) {
                // Process the last symbol on the line
                struct Symbol* s;
                if (strcmp(symbol, "eps") == 0) {
                    s = (struct Symbol*)malloc(sizeof(struct Symbol));
                    if (s == NULL) {
                        printf("ERROR: Memory allocation failed\n");
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                    s->TYPE.TERMINAL = TK_EPS;
                    s->IS_TERMINAL   = 1;
                    s->next          = NULL;
                } else {
                    s = initializeSymbol(symbol);
                    if (s == NULL) {
                        printf("ERROR: Failed to initialize symbol %s\n", symbol);
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                }

                if (sl == NULL) {
                    sl = initializeSymbolList();
                    if (sl == NULL) {
                        printf("ERROR: Failed to initialize symbol list\n");
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                }

                addToSymbolList(sl, s);

                // Create the rule and add it to the grammar
                struct Rule* r = initializeRule(sl, ruleCount);
                if (r == NULL) {
                    printf("ERROR: Failed to initialize rule\n");
                    free(symbol);
                    close(fd);
                    exit(1);
                }

                parsedGrammar->GRAMMAR_RULES[ruleCount] = r;
                ruleCount++;

                // Reset for next line
                symbolsRead = 0;
                free(symbol);
                symbol = (char*)malloc(1);
                if (symbol == NULL) {
                    printf("ERROR: Memory allocation failed\n");
                    close(fd);
                    exit(1);
                }
                symbol[0] = '\0';
                sl        = NULL;
            }

            // Skip any additional newline characters
            if (c == '\r') {
                char nextChar;
                if (read(fd, &nextChar, sizeof(char)) > 0 && nextChar != '\n') {
                    lseek(fd, -1, SEEK_CUR); // Move back if not followed by '\n'
                }
            }
        } else {
            // We're reading a symbol character
            if (symbolsRead == 0 && sl == NULL) {
                sl = initializeSymbolList();
                if (sl == NULL) {
                    printf("ERROR: Failed to initialize symbol list\n");
                    free(symbol);
                    close(fd);
                    exit(1);
                }
            }

            // Append character to symbol
            char* newSymbol = (char*)malloc(strlen(symbol) + 2);
            if (newSymbol == NULL) {
                printf("ERROR: Memory allocation failed\n");
                free(symbol);
                close(fd);
                exit(1);
            }
            strcpy(newSymbol, symbol);
            newSymbol[strlen(symbol)]     = c;
            newSymbol[strlen(symbol) + 1] = '\0';
            free(symbol);
            symbol = newSymbol;
        }
    }

    // Process any remaining symbol at EOF
    if (symbol != NULL && strlen(symbol) > 0) {
        struct Symbol* s;
        if (strcmp(symbol, "eps") == 0) {
            s = (struct Symbol*)malloc(sizeof(struct Symbol));
            if (s != NULL) {
                s->TYPE.TERMINAL = TK_EPS;
                s->IS_TERMINAL   = 1;
                s->next          = NULL;

                if (sl != NULL) {
                    addToSymbolList(sl, s);
                    struct Rule* r = initializeRule(sl, ruleCount);
                    if (r != NULL) {
                        parsedGrammar->GRAMMAR_RULES[ruleCount] = r;
                        ruleCount++;
                    }
                }
            }
        } else {
            s = initializeSymbol(symbol);
            if (s != NULL && sl != NULL) {
                addToSymbolList(sl, s);
                struct Rule* r = initializeRule(sl, ruleCount);
                if (r != NULL) {
                    parsedGrammar->GRAMMAR_RULES[ruleCount] = r;
                    ruleCount++;
                }
            }
        }
    }

    free(symbol);
    close(fd);

    if (currentNonTerminal != NULL) {
        nonTerminalRuleRecords[currentNonTerminal->TYPE.NON_TERMINAL]->end = ruleCount - 1;
    }

    return parsedGrammar;
}