#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

#include "constants.h"

Grammar* g;
NonTerminalRuleRecords** ntrr;
int checkIfDone[NUM_NONTERMINALS] = {0};
int vectorSize = NUM_TERMINALS+1;

int syntaxErrorFlag;
int lexicalErrorFlag;

char* TerminalID[] = {
    "TK_ASSIGNOP",    // <---
    "TK_COMMENT",     // %
    "TK_FIELDID",     // [a-z][a-z]*
    "TK_ID",          // [b-d][2-7][b-d]*[2-7]*
    "TK_NUM",         // [0-9][0-9]*
    "TK_RNUM",        // Real numbers (both formats)
    "TK_FUNID",       // _[a-z|A-Z][a-z|A-Z]*[0-9]*
    "TK_RUID",        // #[a-z][a-z]*
    "TK_WITH",        // with
    "TK_PARAMETERS",  // parameters
    "TK_END",         // end
    "TK_WHILE",       // while
    "TK_UNION",       // union
    "TK_ENDUNION",    // endunion
    "TK_DEFINETYPE",  // definetype
    "TK_AS",          // as
    "TK_TYPE",        // type
    "TK_MAIN",        // _main
    "TK_GLOBAL",      // global
    "TK_PARAMETER",   // parameter
    "TK_LIST",        // list
    "TK_SQL",         // [
    "TK_SQR",         // ]
    "TK_INPUT",       // input
    "TK_OUTPUT",      // output
    "TK_INT",         // int
    "TK_REAL",        // real
    "TK_COMMA",       // ,
    "TK_SEM",         // ;
    "TK_COLON",       // :
    "TK_DOT",         // .
    "TK_ENDWHILE",    // endwhile
    "TK_OP",          // (
    "TK_CL",          // )
    "TK_IF",          // if
    "TK_THEN",        // then
    "TK_ENDIF",       // endif
    "TK_READ",        // read
    "TK_WRITE",       // write
    "TK_RETURN",      // return
    "TK_PLUS",        // +
    "TK_MINUS",       // -
    "TK_MUL",         // *
    "TK_DIV",         // /
    "TK_CALL",        // call
    "TK_RECORD",      // record
    "TK_ENDRECORD",   // endrecord
    "TK_ELSE",        // else
    "TK_AND",         // &&&
    "TK_OR",          // @@@
    "TK_NOT",         // ~
    "TK_LT",          // 
    "TK_LE",          // <=
    "TK_EQ",          // ==
    "TK_GT",          // >
    "TK_GE",          // >=
    "TK_NE",          // !=
    "TK_ERR",         // Error token
    "TK_EPS",         // Epsilon (empty production)
    "TK_DOLLAR",      // End marker for parsing stack
    "TK_EOF"          // End of file marker
};

char* NonTerminalID[] = {
    "program",
    "mainFunction",
    "otherFunctions",
    "function",
    "input_par",
    "output_par",
    "parameter_list",
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

void errorExit(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}


char* copyLexeme(char* str) {
    int len = strlen(str);
    char* lex = (char*)malloc(sizeof(char)*(len+1));
    for(int i=0; i < len; i++)
        lex[i] = str[i];
    lex[len] = '\0';
    return lex;
}

char* appendToSymbol(char* str, char c) {
    if (str == NULL) {
        char* newStr = (char*)malloc(2 * sizeof(char));
        if (newStr == NULL) {
            printf("Error: Memory allocation failed in appendToSymbol\n");
            return NULL;
        }
        newStr[0] = c;
        newStr[1] = '\0';
        return newStr;
    }
    
    int len = strlen(str);
    char* strConcat = (char*)malloc(sizeof(char)*(len+2));
    if (strConcat == NULL) {
        printf("Error: Memory allocation failed in appendToSymbol\n");
        return str; // Return original string if allocation fails
    }
    
    strcpy(strConcat, str);
    strConcat[len] = c;
    strConcat[len+1] = '\0';
    
    free(str); // Free the original string to prevent memory leak
    return strConcat;
}

int findInTerminalMap(char* str) {
    if (str == NULL) return -1;
    
    printf("Looking for terminal: '%s'\n", str);
    
    for(int i=0; i < NUM_TERMINALS; i++) {
        printf("Comparing with: '%s'\n", TerminalID[i]);
        if(TerminalID[i] != NULL && strcmp(str, TerminalID[i]) == 0) {
            printf("Match found at index %d\n", i);
            return i;
        }
    }
    
    printf("No match found for terminal: '%s'\n", str);
    return -1;
}

int findInNonTerminalMap(char* str) {
    if (str == NULL) return -1;
    
    for(int i=0; i < NUM_NONTERMINALS; i++) {
        if(NonTerminalID[i] != NULL && strcmp(str, NonTerminalID[i]) == 0)
            return i;
    }
    return -1;
}


char* getTerminal(int enumId) {
    return TerminalID[enumId];
}

char* getNonTerminal(int enumId) {
    return NonTerminalID[enumId];
}

ParsingTable* initialiseParsingTable() {
    ParsingTable* pt = (ParsingTable*)malloc(sizeof(ParsingTable));
    pt->entries = (int**)malloc(NUM_NONTERMINALS*sizeof(int*));
    for(int i=0; i < NUM_NONTERMINALS; i++) {
        pt->entries[i] = (int*)calloc(NUM_TERMINALS, sizeof(int));
    }
    return pt;
}

int initialiseGrammar() {
    g = (Grammar*)malloc(sizeof(Grammar));
    g->GRAMMAR_RULES_SIZE = NUM_GRAMMAR_RULES+1;
    g->GRAMMAR_RULES = (Rule**)malloc(sizeof(Rule*)*g->GRAMMAR_RULES_SIZE);
    g->GRAMMAR_RULES[0] = NULL;
}

Symbol* intialiseSymbol(char* symbol) {
    if (symbol == NULL) {
        printf("Error: NULL symbol passed to intialiseSymbol\n");
        return NULL;
    }
    
    Symbol* s = (Symbol*)malloc(sizeof(Symbol));
    if (s == NULL) {
        printf("Error: Memory allocation failed in intialiseSymbol\n");
        return NULL;
    }
    
    int idNonTerminal, idTerminal;
    
    // First check if it's a terminal (TK_* tokens)
    if (strncmp(symbol, "TK_", 3) == 0) {
        idTerminal = findInTerminalMap(symbol);
        if(idTerminal != -1) {
            s->TYPE.TERMINAL = idTerminal;
            s->IS_TERMINAL = 1;
        } else {
            printf("Error: Unknown terminal symbol: %s\n", symbol);
            free(s); // Clean up allocated memory
            return NULL;
        }
    } else {
        // Check if it's a non-terminal
        idNonTerminal = findInNonTerminalMap(symbol);
        if(idNonTerminal != -1) {
            s->TYPE.NON_TERMINAL = idNonTerminal;
            s->IS_TERMINAL = 0;
        } else {
            printf("Error: Unknown symbol: %s\n", symbol);
            free(s); // Clean up allocated memory
            return NULL;
        }
    }
    
    s->next = NULL;
    return s;
}

SymbolList* initialiseSymbolList() {
    SymbolList* sl = (SymbolList*)malloc(sizeof(SymbolList));
    sl->HEAD_SYMBOL = NULL;
    sl->TAIL_SYMBOL = NULL;
    sl->RULE_LENGTH = 0;
    return sl;
}

Rule* initialiseRule(SymbolList* sl, int ruleCount) {
    Rule* r = (Rule*)malloc(sizeof(Rule));
    r->SYMBOLS = sl;
    r->RULE_NO = ruleCount;
    return r;
}

NonTerminalRuleRecords** intialiseNonTerminalRecords() {
    NonTerminalRuleRecords** ntrr = (NonTerminalRuleRecords**)malloc(sizeof(NonTerminalRuleRecords*)*NUM_NONTERMINALS);
    return ntrr;
}

void initialiseCheckIfDone() {
    for(int i=0; i < NUM_NONTERMINALS; i++)
        checkIfDone[i] = 0;
}

FirstAndFollow* initialiseFirstAndFollow() {
    FirstAndFollow* fafl = (FirstAndFollow*)malloc(sizeof(FirstAndFollow));
    fafl->FIRST = (int**)malloc(sizeof(int*)*NUM_NONTERMINALS);
    fafl->FOLLOW = (int**)malloc(sizeof(int*)*NUM_NONTERMINALS);
    for(int i=0; i < NUM_NONTERMINALS; i++) {
        fafl->FIRST[i] = (int*)calloc(vectorSize, sizeof(int));
        fafl->FOLLOW[i] = (int*)calloc(vectorSize, sizeof(int));
    }
    return fafl;
}

void calculateFirst(int** firstVector, int enumId) {
    if (enumId < 0 || enumId >= NUM_NONTERMINALS) {
        printf("ERROR: Invalid enumId %d in calculateFirst\n", enumId);
        return;
    }
    
    if (firstVector == NULL) {
        printf("ERROR: NULL firstVector in calculateFirst\n");
        return;
    }
    
    printf("Calculating First for %s (enumId=%d)\n", getNonTerminal(enumId), enumId);
    
    if (ntrr == NULL || ntrr[enumId] == NULL) {
        printf("ERROR: NULL ntrr for enumId %d in calculateFirst\n", enumId);
        return;
    }
    
    int start = ntrr[enumId]->start;
    int end = ntrr[enumId]->end;
    int producesNull = 0;

    for(int i=start; i <= end; i++) {
        if (g == NULL || g->GRAMMAR_RULES == NULL || g->GRAMMAR_RULES[i] == NULL) {
            printf("ERROR: NULL grammar rule at index %d\n", i);
            continue;
        }
        
        Rule* r = g->GRAMMAR_RULES[i];
        if (r->SYMBOLS == NULL || r->SYMBOLS->HEAD_SYMBOL == NULL) {
            printf("ERROR: NULL symbols in rule %d\n", i);
            continue;
        }
        
        Symbol* s = r->SYMBOLS->HEAD_SYMBOL;
        Symbol* trav = s;
        Symbol* nextSymbol = trav->next;
        int ruleYieldsEpsilon = 1;
        
        while(nextSymbol != NULL) {
            if(nextSymbol->IS_TERMINAL == 1) {
                if(nextSymbol->TYPE.TERMINAL != TK_EPS) {
                    ruleYieldsEpsilon = 0;
                    firstVector[enumId][nextSymbol->TYPE.TERMINAL] = 1;
                }
                break;
            }
            
            int nonTermId = nextSymbol->TYPE.NON_TERMINAL;
            if (nonTermId < 0 || nonTermId >= NUM_NONTERMINALS) {
                printf("ERROR: Invalid non-terminal ID %d\n", nonTermId);
                break;
            }
            
            if(checkIfDone[nonTermId] == 0) {
                calculateFirst(firstVector, nonTermId);
            }
            
            for(int j=0; j < vectorSize; j++) {
                if(firstVector[nonTermId][j] == 1)
                    firstVector[enumId][j] = 1;
            }
            
            if(firstVector[nonTermId][TK_EPS] == 0) {
                ruleYieldsEpsilon = 0;
                break;
            }
            nextSymbol = nextSymbol->next;
        }
        
        if(ruleYieldsEpsilon)
            producesNull = 1;
    }
    
    if(producesNull)
        firstVector[enumId][TK_EPS] = 1;
    else
        firstVector[enumId][TK_EPS] = 0;
        
    checkIfDone[enumId] = 1;
}

void populateFirst(int** firstVector, Grammar* g) {
    for(int i=0; i < NUM_NONTERMINALS; i++) {
        if(checkIfDone[i] == 0)
            calculateFirst(firstVector, i);
    }
}

void populateFollow(int** followVector, int** firstVector, Grammar* g) {
    for(int i=1; i <= NUM_GRAMMAR_RULES; i++) {
        Rule* r = g->GRAMMAR_RULES[i];
        Symbol* head = r->SYMBOLS->HEAD_SYMBOL;
        Symbol* trav = head->next;
        int epsilonIdentifier = 0;
        while(trav != NULL) {
            if(trav->IS_TERMINAL == 0) {
                Symbol* followTrav = trav->next;
                while(followTrav != NULL) {
                    if(followTrav->IS_TERMINAL == 1 && followTrav->TYPE.TERMINAL != TK_EPS) {
                        followVector[trav->TYPE.NON_TERMINAL][followTrav->TYPE.TERMINAL] = 1;
                        break;
                    }
                    else {
                        for(int j=0; j < vectorSize; j++)
                            if(firstVector[followTrav->TYPE.NON_TERMINAL][j] == 1 && j != TK_EPS)
                                followVector[trav->TYPE.NON_TERMINAL][j] = 1;
                        if(firstVector[followTrav->TYPE.NON_TERMINAL][TK_EPS] == 0)
                            break;
                    }
                    followTrav = followTrav->next;
                }
                if(trav->next == NULL || (followTrav == NULL)) {
                    for(int j=0; j < vectorSize; j++)
                        if(followVector[head->TYPE.NON_TERMINAL][j] == 1 && j != TK_EPS)
                            followVector[trav->TYPE.NON_TERMINAL][j] = 1;
                }
            }
            trav = trav->next;
        }
    }
}

void populateFollowTillStable(int** followVector, int** firstVector, Grammar* g) {
    int** prevFollowVector = (int**)malloc(NUM_NONTERMINALS*sizeof(int*));
    for(int i=0; i < NUM_NONTERMINALS; i++) {
        prevFollowVector[i] = (int*)calloc(vectorSize, sizeof(int));
    }

    followVector[program][TK_DOLLAR] = 1;
    prevFollowVector[program][TK_DOLLAR] = 1;

    while(1) {
        populateFollow(followVector, firstVector, g);
        int stable = 1;
        for(int i=0; i < NUM_NONTERMINALS; i++) {
            for(int j=0; j < vectorSize; j++) {
                if(prevFollowVector[i][j] != followVector[i][j])
                    stable = 0;
            }
        }
        if(stable)
            break;
        for(int i=0; i < NUM_NONTERMINALS; i++) {
            for(int j=0; j < vectorSize; j++)
                prevFollowVector[i][j] = followVector[i][j];
        }
    }
}

void verifyFirstAndFollow(FirstAndFollow* fafl) {
    printf("Verifying First and Follow sets...\n");
    
    if (fafl == NULL) {
        printf("ERROR: FirstAndFollow is NULL\n");
        return;
    }
    
    if (fafl->FIRST == NULL) {
        printf("ERROR: FIRST array is NULL\n");
        return;
    }
    
    if (fafl->FOLLOW == NULL) {
        printf("ERROR: FOLLOW array is NULL\n");
        return;
    }
    
    // Verify that all non-terminals have First sets
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (fafl->FIRST[i] == NULL) {
            printf("ERROR: FIRST set for non-terminal %s (%d) is NULL\n", getNonTerminal(i), i);
            continue;
        }
        
        // Print the First set
        printf("FIRST(%s): ", getNonTerminal(i));
        int isEmpty = 1;
        for (int j = 0; j < vectorSize; j++) {
            if (fafl->FIRST[i][j] == 1) {
                if (j == TK_EPS) {
                    printf("ε ");
                } else {
                    printf("%s ", getTerminal(j));
                }
                isEmpty = 0;
            }
        }
        if (isEmpty) {
            printf("EMPTY");
        }
        printf("\n");
    }
    
    // Verify that all non-terminals have Follow sets
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (fafl->FOLLOW[i] == NULL) {
            printf("ERROR: FOLLOW set for non-terminal %s (%d) is NULL\n", getNonTerminal(i), i);
            continue;
        }
        
        // Print the Follow set
        printf("FOLLOW(%s): ", getNonTerminal(i));
        int isEmpty = 1;
        for (int j = 0; j < vectorSize; j++) {
            if (fafl->FOLLOW[i][j] == 1) {
                printf("%s ", getTerminal(j));
                isEmpty = 0;
            }
        }
        if (isEmpty) {
            printf("EMPTY");
        }
        printf("\n");
    }
    
    printf("First and Follow verification complete\n");
}

FirstAndFollow* computeFirstAndFollowSets(Grammar* g) {
    printf("Starting computeFirstAndFollowSets()\n");
    FirstAndFollow* fafl = initialiseFirstAndFollow();
    populateFirst(fafl->FIRST, g);
    populateFollowTillStable(fafl->FOLLOW, fafl->FIRST, g);
    
    // Add validation
    verifyFirstAndFollow(fafl);
    
    printf("Finished computeFirstAndFollowSets()\n");
    return fafl;
}

void createParseTable(FirstAndFollow* fafl, ParsingTable* pt) {
    for(int i=1; i <= NUM_GRAMMAR_RULES; i++) {
        Rule* r = g->GRAMMAR_RULES[i];
        int lhsNonTerminal = r->SYMBOLS->HEAD_SYMBOL->TYPE.NON_TERMINAL;
        
        Symbol* rhsHead = r->SYMBOLS->HEAD_SYMBOL->next;
        Symbol* trav = rhsHead;
        int epsilonGenerated = 1;
        
        while(trav != NULL) {
            if(trav->IS_TERMINAL == 1 && trav->TYPE.TERMINAL != TK_EPS) {
                epsilonGenerated = 0;
                pt->entries[lhsNonTerminal][trav->TYPE.TERMINAL] = r->RULE_NO;
                break;
            }
            else if(trav->IS_TERMINAL == 1 && trav->TYPE.TERMINAL == TK_EPS) {
                epsilonGenerated = 1;
                break;
            }
            else {
                for(int j=0; j < NUM_TERMINALS; j++) {
                    if(fafl->FIRST[trav->TYPE.NON_TERMINAL][j] == 1)
                        pt->entries[lhsNonTerminal][j] = r->RULE_NO;
                }
                if(fafl->FIRST[trav->TYPE.NON_TERMINAL][TK_EPS] == 0) {
                    epsilonGenerated = 0;
                    break;
                }
            }
            trav = trav->next;
        }
        
        if(epsilonGenerated) {
            for(int j=0; j < NUM_TERMINALS; j++) {
                if(fafl->FOLLOW[lhsNonTerminal][j] == 1)
                    pt->entries[lhsNonTerminal][j] = r->RULE_NO;
            }
        }
    }
}

void addToSymbolList(SymbolList* ls, Symbol* s) {
    Symbol* h = ls->HEAD_SYMBOL;
    if(h == NULL) {
        ls->HEAD_SYMBOL = s;
        ls->TAIL_SYMBOL = s;
        ls->RULE_LENGTH = 1;
        return;
    }
    ls->TAIL_SYMBOL->next = s;
    ls->TAIL_SYMBOL = s;
    ls->RULE_LENGTH += 1;
}

void verifyGrammar() {
    printf("Verifying grammar...\n");
    
    if (g == NULL) {
        printf("ERROR: Grammar is NULL\n");
        return;
    }
    
    if (g->GRAMMAR_RULES == NULL) {
        printf("ERROR: Grammar rules array is NULL\n");
        return;
    }
    
    printf("Grammar has %d rules\n", g->GRAMMAR_RULES_SIZE);
    
    for (int i = 1; i < g->GRAMMAR_RULES_SIZE; i++) {
        Rule* r = g->GRAMMAR_RULES[i];
        if (r == NULL) {
            printf("ERROR: Rule %d is NULL\n", i);
            continue;
        }
        
        if (r->SYMBOLS == NULL) {
            printf("ERROR: Symbols for rule %d is NULL\n", i);
            continue;
        }
        
        Symbol* head = r->SYMBOLS->HEAD_SYMBOL;
        if (head == NULL) {
            printf("ERROR: Head symbol for rule %d is NULL\n", i);
            continue;
        }
        
        printf("Rule %d: ", i);
        
        // Print the rule
        Symbol* trav = head;
        while (trav != NULL) {
            if (trav->IS_TERMINAL) {
                printf("%s ", getTerminal(trav->TYPE.TERMINAL));
            } else {
                printf("%s ", getNonTerminal(trav->TYPE.NON_TERMINAL));
            }
            trav = trav->next;
        }
        printf("\n");
    }
    printf("Grammar verification complete\n");
}

void verifyNTRR() {
    printf("Verifying non-terminal rule records...\n");
    
    if (ntrr == NULL) {
        printf("ERROR: NTRR is NULL\n");
        return;
    }
    
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        printf("Checking non-terminal %d: %s\n", i, getNonTerminal(i));
        if (ntrr[i] == NULL) {
            printf("ERROR: NTRR for non-terminal %s (%d) is NULL\n", getNonTerminal(i), i);
            continue;
        }
        
        printf("Non-terminal %s (%d): Rules %d to %d\n", 
               getNonTerminal(i), i, ntrr[i]->start, ntrr[i]->end);
               
        // Verify that the rule range is valid
        if (ntrr[i]->start > ntrr[i]->end) {
            printf("ERROR: Invalid rule range for non-terminal %s (%d)\n", getNonTerminal(i), i);
        }
        
        // Verify that the rule range is within bounds
        if (ntrr[i]->start < 1 || ntrr[i]->end >= g->GRAMMAR_RULES_SIZE) {
            printf("ERROR: Rule range out of bounds for non-terminal %s (%d)\n", getNonTerminal(i), i);
        }
    }
    
    printf("NTRR verification complete\n");
}

Grammar* extractGrammar() {
    printf("Starting extractGrammar()\n");
    int ruleCount = 1;
    int fd = open(GRAMMAR_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        printf("ERROR: Failed to open grammar file %s\n", GRAMMAR_FILE_PATH);
        exit(1);
    }
    
    char c;
    int actualRead;
    char* symbol = (char*)malloc(1); // Initialize with empty string
    if (symbol == NULL) {
        printf("ERROR: Memory allocation failed\n");
        close(fd);
        exit(1);
    }
    
    symbol[0] = '\0';
    int symbolsRead = 0;
    Symbol* currentNonTerminal = NULL;
    SymbolList* sl = NULL;
    
    initialiseGrammar();
    ntrr = intialiseNonTerminalRecords();
    initialiseCheckIfDone();
    
    while((actualRead = read(fd, &c, sizeof(char))) > 0) {
        if (c == ' ' || c == '\t') {
            if (strlen(symbol) > 0) {
                symbolsRead++;
                printf("Processing symbol: %s\n", symbol);

                // Handle symbol correctly
                Symbol* s;
                if (strcmp(symbol, "eps") == 0) {
                    s = (Symbol*)malloc(sizeof(Symbol));
                    if (s == NULL) {
                        printf("ERROR: Memory allocation failed\n");
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                    s->TYPE.TERMINAL = TK_EPS;
                    s->IS_TERMINAL = 1;
                    s->next = NULL;
                } else {
                    s = intialiseSymbol(symbol);
                    if (s == NULL) {
                        printf("ERROR: Failed to initialize symbol %s\n", symbol);
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                }
                
                // Add to symbol list
                if (sl == NULL) {
                    printf("ERROR: SymbolList is NULL when processing symbol %s\n", symbol);
                    free(symbol);
                    close(fd);
                    exit(1);
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
                        
                        ntrr[s->TYPE.NON_TERMINAL] = (NonTerminalRuleRecords*)malloc(sizeof(NonTerminalRuleRecords));
                        if (ntrr[s->TYPE.NON_TERMINAL] == NULL) {
                            printf("ERROR: Memory allocation failed for NTRR\n");
                            free(symbol);
                            close(fd);
                            exit(1);
                        }
                        ntrr[s->TYPE.NON_TERMINAL]->start = ruleCount;
                    } else if (currentNonTerminal->TYPE.NON_TERMINAL != s->TYPE.NON_TERMINAL) {
                        ntrr[currentNonTerminal->TYPE.NON_TERMINAL]->end = ruleCount-1;
                        ntrr[s->TYPE.NON_TERMINAL] = (NonTerminalRuleRecords*)malloc(sizeof(NonTerminalRuleRecords));
                        if (ntrr[s->TYPE.NON_TERMINAL] == NULL) {
                            printf("ERROR: Memory allocation failed for NTRR\n");
                            free(symbol);
                            close(fd);
                            exit(1);
                        }
                        ntrr[s->TYPE.NON_TERMINAL]->start = ruleCount;
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
                Symbol* s;
                if (strcmp(symbol, "eps") == 0) {
                    s = (Symbol*)malloc(sizeof(Symbol));
                    if (s == NULL) {
                        printf("ERROR: Memory allocation failed\n");
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                    s->TYPE.TERMINAL = TK_EPS;
                    s->IS_TERMINAL = 1;
                    s->next = NULL;
                } else {
                    s = intialiseSymbol(symbol);
                    if (s == NULL) {
                        printf("ERROR: Failed to initialize symbol %s\n", symbol);
                        free(symbol);
                        close(fd);
                        exit(1);
                    }
                }
                
                if (sl == NULL) {
                    printf("ERROR: SymbolList is NULL when processing end of line\n");
                    free(symbol);
                    close(fd);
                    exit(1);
                }
                
                addToSymbolList(sl, s);
                
                // Create the rule and add it to the grammar
                Rule* r = initialiseRule(sl, ruleCount);
                if (r == NULL) {
                    printf("ERROR: Failed to initialize rule\n");
                    free(symbol);
                    close(fd);
                    exit(1);
                }
                
                g->GRAMMAR_RULES[ruleCount] = r;
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
                sl = NULL;
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
                sl = initialiseSymbolList();
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
            newSymbol[strlen(symbol)] = c;
            newSymbol[strlen(symbol) + 1] = '\0';
            free(symbol);
            symbol = newSymbol;
        }
    }
    
    // Process any remaining symbol at EOF
    if (symbol != NULL && strlen(symbol) > 0) {
        Symbol* s;
        if (strcmp(symbol, "eps") == 0) {
            s = (Symbol*)malloc(sizeof(Symbol));
            if (s != NULL) {
                s->TYPE.TERMINAL = TK_EPS;
                s->IS_TERMINAL = 1;
                s->next = NULL;
                
                if (sl != NULL) {
                    addToSymbolList(sl, s);
                    Rule* r = initialiseRule(sl, ruleCount);
                    if (r != NULL) {
                        g->GRAMMAR_RULES[ruleCount] = r;
                        ruleCount++;
                    }
                }
            }
        } else {
            s = intialiseSymbol(symbol);
            if (s != NULL && sl != NULL) {
                addToSymbolList(sl, s);
                Rule* r = initialiseRule(sl, ruleCount);
                if (r != NULL) {
                    g->GRAMMAR_RULES[ruleCount] = r;
                    ruleCount++;
                }
            }
        }
    }
    
    free(symbol);
    close(fd);
    
    
    if (currentNonTerminal != NULL) {
        ntrr[currentNonTerminal->TYPE.NON_TERMINAL]->end = ruleCount-1;
    }

    // Verify the loaded grammar
    verifyGrammar();
    verifyNTRR();
    
    printf("Finished extractGrammar()\n");
    return g;
}

NaryTreeNode* createLeafNode(int enumId) {
    NaryTreeNode* ntn = (NaryTreeNode*)malloc(sizeof(NaryTreeNode));
    ntn->IS_LEAF_NODE = 1;
    ntn->NODE_TYPE.L.ENUM_ID = enumId;
    ntn->next = NULL;
    return ntn;
}

NaryTreeNode* createNonLeafNode(int enumId) {
    NaryTreeNode* ntn = (NaryTreeNode*)malloc(sizeof(NaryTreeNode));
    ntn->IS_LEAF_NODE = 0;
    ntn->NODE_TYPE.NL.ENUM_ID = enumId;
    ntn->NODE_TYPE.NL.NUMBER_CHILDREN = 0;
    ntn->next = NULL;
    ntn->NODE_TYPE.NL.child = NULL;
    return ntn;
}

NaryTreeNode* createNode(int isTerminal, SymbolType type, NaryTreeNode* parent) {
    NaryTreeNode* ntn;
    if(isTerminal == 1) {
        ntn = createLeafNode(type.TERMINAL);
        ntn->parent = parent;
    }
    else {
        ntn = createNonLeafNode(type.NON_TERMINAL);
        ntn->parent = parent;
    }
    return ntn;
}

ParseTree* initialiseParseTree() {
    ParseTree* pt = (ParseTree*)malloc(sizeof(ParseTree));
    pt->root = createNonLeafNode(program);
    pt->root->parent = NULL;
    return pt;
}

void addRuleToParseTree(NaryTreeNode* ntn, Rule* r) {
    if (ntn == NULL || r == NULL) {
        printf("Error: NULL pointer passed to addRuleToParseTree\n");
        return;
    }
    
    if(ntn->IS_LEAF_NODE == 1) {
        printf("TERMINALS CANNOT HAVE CHILDREN! \n");
        return;
    }
    
    int numberChild = 0;
    Symbol* trav = r->SYMBOLS->HEAD_SYMBOL->next;
    NaryTreeNode* childHead = NULL;
    NaryTreeNode* childTrav = NULL;
    
    while(trav != NULL) {
        NaryTreeNode* newNode = createNode(trav->IS_TERMINAL, trav->TYPE, ntn);
        if (newNode == NULL) {
            printf("Error: Failed to create tree node\n");
            return;
        }
        
        if(childHead == NULL) {
            childHead = newNode;
            childTrav = childHead;
        }
        else {
            childTrav->next = newNode;
            childTrav = childTrav->next;
        }
        numberChild++;
        trav = trav->next;
    }
    
    ntn->NODE_TYPE.NL.RULE_NO = r->RULE_NO;
    ntn->NODE_TYPE.NL.child = childHead;
    ntn->NODE_TYPE.NL.NUMBER_CHILDREN = numberChild;
}

void printNaryTree(NaryTreeNode* nt) {
    if(nt->IS_LEAF_NODE == 1) {
        printf("%s ", getTerminal(nt->NODE_TYPE.L.ENUM_ID));
        return;
    }
    
    printf("%s\n", getNonTerminal(nt->NODE_TYPE.NL.ENUM_ID));
    
    NaryTreeNode* childTrav = nt->NODE_TYPE.NL.child;
    while(childTrav != NULL) {
        if(childTrav->IS_LEAF_NODE == 1)
            printf("%s ", getTerminal(childTrav->NODE_TYPE.L.ENUM_ID));
        else
            printf("%s ", getNonTerminal(childTrav->NODE_TYPE.NL.ENUM_ID));
        
        childTrav = childTrav->next;
    }
    
    printf("\n");
    
    childTrav = nt->NODE_TYPE.NL.child;
    while(childTrav != NULL) {
        if(childTrav->IS_LEAF_NODE == 0)
            printNaryTree(childTrav);
        childTrav = childTrav->next;
    }
}

void printTree(ParseTree* pt) {
    NaryTreeNode* nt = pt->root;
    printNaryTree(nt);
}

StackNode* createStackNode(NaryTreeNode* ntn) {
    StackNode* stn = (StackNode*)malloc(sizeof(StackNode));
    stn->TREE_NODE = ntn;
    stn->next = NULL;
    return stn;
}

NaryTreeNode* top(Stack* st) {
    if(st->HEAD == NULL)
        return NULL;
    else
        return st->HEAD->TREE_NODE;
}

void push(Stack* st, NaryTreeNode* ntn) {
    StackNode* stn = createStackNode(ntn);
    StackNode* head = st->HEAD;
    
    if(head == NULL) {
        st->HEAD = stn;
        st->NUM_NODES++;
        return;
    }
    
    stn->next = head;
    st->HEAD = stn;
    st->NUM_NODES++;
    return;
}

void pop(Stack* st) {
    StackNode* head = st->HEAD;
    
    if(head == NULL)
        return;
    
    st->HEAD = st->HEAD->next;
    st->NUM_NODES--;
}

void pushTreeChildren(Stack* st, NaryTreeNode* ntn) {
    if(ntn == NULL)
        return;
    pushTreeChildren(st, ntn->next);
    push(st, ntn);
}

Stack* initialiseStack(ParseTree* pt) {
    Stack* st = (Stack*)malloc(sizeof(Stack));
    st->HEAD = NULL;
    st->NUM_NODES = 0;
    
    SymbolType sType;
    sType.TERMINAL = TK_DOLLAR;
    NaryTreeNode* ntn = createNode(1, sType, NULL);
    push(st, ntn);
    push(st, pt->root);
    return st;
}

ParseTree* parseInputSourceCode(char* testcaseFile, ParsingTable* pTable, FirstAndFollow* fafl) {
    int f = open(testcaseFile, O_RDONLY);
    
    initializeLexer(f);
    ParseTree* pt = initialiseParseTree();
    Stack* st = initialiseStack(pt);
    
    syntaxErrorFlag = 0;
    lexicalErrorFlag = 0;
    Token* missedToken = NULL;
    Token* inputToken = getToken();
    
    while(1) {
        if(inputToken == NULL)
            break;
            
        if(inputToken->TOKEN_NAME == TK_COMMENT) {
            inputToken = getToken();
            continue;
        }
        
        if(inputToken->TOKEN_NAME == TK_ERR) {
            lexicalErrorFlag = 1;
        }
        
        NaryTreeNode* stackTop = top(st);
        
        if(stackTop->IS_LEAF_NODE == 1) {
            if(inputToken->TOKEN_NAME == stackTop->NODE_TYPE.L.ENUM_ID) {
                stackTop->NODE_TYPE.L.TK = (Token*)malloc(sizeof(Token));
                stackTop->NODE_TYPE.L.TK->LEXEME = copyLexeme(inputToken->LEXEME);
                stackTop->NODE_TYPE.L.TK->LINE_NO = inputToken->LINE_NO;
                stackTop->NODE_TYPE.L.TK->TOKEN_NAME = inputToken->TOKEN_NAME;
                stackTop->NODE_TYPE.L.TK->IS_NUMBER = inputToken->IS_NUMBER;
                stackTop->NODE_TYPE.L.TK->VALUE = inputToken->VALUE;
                
                pop(st);
                inputToken = getToken();
                continue;
            }
            else {
                syntaxErrorFlag = 1;
                
                if(inputToken->TOKEN_NAME != TK_ERR)
                    printf("Line %d : The token %s for the lexeme %s does not match with the expected token %s\n", inputToken->LINE_NO, getTerminal(inputToken->TOKEN_NAME), inputToken->LEXEME, getTerminal(stackTop->NODE_TYPE.L.ENUM_ID));
                
                if(inputToken->TOKEN_NAME == TK_ERR) {
                    stackTop->NODE_TYPE.L.TK = (Token*)malloc(sizeof(Token));
                    stackTop->NODE_TYPE.L.TK->LEXEME = inputToken->LEXEME;
                    stackTop->NODE_TYPE.L.TK->LINE_NO = inputToken->LINE_NO;
                    stackTop->NODE_TYPE.L.TK->TOKEN_NAME = stackTop->NODE_TYPE.L.ENUM_ID;
                    stackTop->NODE_TYPE.L.TK->IS_NUMBER = 0;
                    stackTop->NODE_TYPE.L.TK->VALUE = NULL;
                    inputToken = getToken();
                    pop(st);
                }
                else {
                    stackTop->NODE_TYPE.L.TK = (Token*)malloc(sizeof(Token));
                    stackTop->NODE_TYPE.L.TK->LEXEME = "ERROR_MISSED_LEXEME";
                    stackTop->NODE_TYPE.L.TK->LINE_NO = inputToken->LINE_NO;
                    stackTop->NODE_TYPE.L.TK->TOKEN_NAME = stackTop->NODE_TYPE.L.ENUM_ID;
                    stackTop->NODE_TYPE.L.TK->IS_NUMBER = 0;
                    stackTop->NODE_TYPE.L.TK->VALUE = NULL;
                    missedToken = inputToken;
                    pop(st);
                }
                
                continue;
            }
        }
        else {
            int ruleNumber = pTable->entries[stackTop->NODE_TYPE.NL.ENUM_ID][inputToken->TOKEN_NAME];
            
            if(ruleNumber != 0) {
                Rule* r = g->GRAMMAR_RULES[ruleNumber];
                addRuleToParseTree(stackTop, r);
                
                pop(st);
                
                NaryTreeNode* childNode = stackTop->NODE_TYPE.NL.child;
                
                if(childNode->IS_LEAF_NODE == 1 && childNode->NODE_TYPE.L.ENUM_ID == TK_EPS);
                else
                    pushTreeChildren(st, childNode);
            }
            else {
                syntaxErrorFlag = 1;
                
                if(inputToken->TOKEN_NAME == TK_ERR) {
                    inputToken = getToken();
                    continue;
                }
                
                if(fafl->FIRST[stackTop->NODE_TYPE.NL.ENUM_ID][TK_EPS] == 1) {
                    pop(st);
                    continue;
                }
                
                if(inputToken != missedToken)
                    printf("Line %d : The token %s for the lexeme %s does not match with the Non Terminal %s\n", inputToken->LINE_NO, getTerminal(inputToken->TOKEN_NAME), inputToken->LEXEME, getNonTerminal(stackTop->NODE_TYPE.NL.ENUM_ID));
                
                while(inputToken != NULL && fafl->FOLLOW[stackTop->NODE_TYPE.NL.ENUM_ID][inputToken->TOKEN_NAME] == 0) {
                    inputToken = getToken();
                }
                
                if(inputToken == NULL)
                    break;
                else {
                    pop(st);
                    continue;
                }
            }
        }
    }
    
    NaryTreeNode* stackTop = top(st);
    if(lexicalErrorFlag == 0 && syntaxErrorFlag == 0 && stackTop->IS_LEAF_NODE == 1 && stackTop->NODE_TYPE.L.ENUM_ID == TK_DOLLAR) {
        printf("\n \nSuccessfully Parsed the whole Input\n");
    }
    else {
        printf("\n \nParsing unsuccesful\n");
    }
    
    close(f);
    
    return pt;
}

void printParseTreeHelper(NaryTreeNode* pt, FILE* f) {
    if(pt == NULL)
        return;
        
    if(pt->IS_LEAF_NODE == 1) {
        int tokenEnumID = pt->NODE_TYPE.L.ENUM_ID;
        char lexeme[30];
        for(int i=0; i < 29; i++)
            lexeme[i] = ' ';
        lexeme[29] = '\0';
        
        if(tokenEnumID != TK_EPS) {
            for(int i=0; i < strlen(pt->NODE_TYPE.L.TK->LEXEME); i++)
                lexeme[i] = pt->NODE_TYPE.L.TK->LEXEME[i];
        }
        else {
            char* str = "EPSILON";
            for(int i=0; i < strlen(str); i++)
                lexeme[i] = str[i];
        }
        
        int lineNumber;
        int isNumber;
        int valueIfInt;
        float valueIfFloat;
        if(tokenEnumID != TK_EPS) {
            lineNumber = pt->NODE_TYPE.L.TK->LINE_NO;
            isNumber = pt->NODE_TYPE.L.TK->IS_NUMBER;
            if(isNumber == 1)
                valueIfInt = pt->NODE_TYPE.L.TK->VALUE->INT_VALUE;
            else if(isNumber == 2)
                valueIfFloat = pt->NODE_TYPE.L.TK->VALUE->FLOAT_VALUE;
        }
        else {
            lineNumber = -1;
        }
        
        char tokenName[20];
        for(int i=0; i < 19; i++)
            tokenName[i] = ' ';
        tokenName[19] = '\0';
        
        char* obtainedTokenName = getTerminal(pt->NODE_TYPE.L.ENUM_ID);
        
        for(int i=0; i < strlen(obtainedTokenName); i++) {
            tokenName[i] = obtainedTokenName[i];
        }
        
        char parent[30];
        for(int i=0; i < 29; i++)
            parent[i] = ' ';
            
        parent[29] = '\0';
        char* obtainedParent = getNonTerminal(pt->parent->NODE_TYPE.NL.ENUM_ID);
        for(int i=0; i < strlen(obtainedParent); i++)
            parent[i] = obtainedParent[i];
            
        char* isLeafNode = "yes";
        char* currentSymbol = "----";
        
        if(tokenEnumID == TK_EPS || isNumber == 0)
            fprintf(f, "%s      %d    %s %s %s %s %s\n", lexeme, lineNumber, tokenName, "----   ", parent, isLeafNode, currentSymbol);
        else if(isNumber == 1)
            fprintf(f, "%s      %d    %s %d          %s %s %s\n", lexeme, lineNumber, tokenName, valueIfInt, parent, isLeafNode, currentSymbol);
        else
            fprintf(f, "%s      %d    %s %f    %s %s %s\n", lexeme, lineNumber, tokenName, valueIfFloat, parent, isLeafNode, currentSymbol);
    }
    else {
        NaryTreeNode* trav = pt->NODE_TYPE.NL.child;
        
        if(trav!=NULL) {
            printParseTreeHelper(pt->NODE_TYPE.NL.child, f);
            trav = trav->next;
        }
        
        char lexeme[30];
        for(int i=0; i < 29; i++)
            lexeme[i] = ' ';
        lexeme[29] = '\0';
        lexeme[0] = '-'; lexeme[1] = '-'; lexeme[2] = '-'; lexeme[3] = '-';
        
        int lineNumber = -1;
        char* tokenName = "-----------        ";
        char* valueIfNumber = "----   ";
        char* currentSymbol = getNonTerminal(pt->NODE_TYPE.NL.ENUM_ID);
        char parent[30];
        for(int i=0; i < 29; i++)
            parent[i] = ' ';
            
        parent[29] = '\0';
        
        char* obtainedParent;
        if(pt->parent != NULL)
            obtainedParent = getNonTerminal(pt->parent->NODE_TYPE.NL.ENUM_ID);
        else
            obtainedParent = "NULL";
            
        for(int i=0; i < strlen(obtainedParent); i++)
            parent[i] = obtainedParent[i];
            
        char* isLeafNode = "no";
        
        fprintf(f, "%s      %d    %s %s %s %s %s\n", lexeme, lineNumber, tokenName, valueIfNumber, parent, isLeafNode, currentSymbol);
        
        while(trav!=NULL){
            printParseTreeHelper(trav, f);
            trav = trav->next;
        }
    }
}

void printParseTree(ParseTree* pt, char* outfile) {
    FILE* f;
    
    if(outfile == NULL)
        f = stdout;
    else
        f = fopen(outfile, "wb");
        
    if(f == NULL) {
        printf("Error opening the outfile\n");
        return;
    }
    
    printParseTreeHelper(pt->root, f);
    
    if(f != stdout)
        fclose(f);
}

int getParseTreeNodeCount() {
    return 0;
}

int getParseTreeMemory() {
    return 0;
}

int getErrorStatus() {
    return (lexicalErrorFlag || syntaxErrorFlag);
}