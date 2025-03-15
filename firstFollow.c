/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/


#include "firstFollow.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>

struct Grammar* globalGrammar = NULL;

// recursively go through the entire grammar and compute the first set for each non-terminal
void computeFirstSets(int mapIndex, int** firstVector) {
    printf("[DEBUG] Starting computeFirstSets\n");
    if (mapIndex < 0 || mapIndex >= NUM_NONTERMINALS) {
        fprintf(stderr, "Error: Invalid non-terminal ID %d in computeFirstSets\n", mapIndex);
        return;
    }
    if (nonTerminalProcessed[mapIndex]) {
        return;
    }
    nonTerminalProcessed[mapIndex] = true;

    if (nonTerminalRuleRecords[mapIndex] == NULL) {
        fprintf(stderr, "Error: NULL rule record for non-terminal %d (%s)\n", mapIndex, getNonTerminal(mapIndex));
        return;
    }

    int ruleStartIndex = nonTerminalRuleRecords[mapIndex]->start;
    int ruleEndIndex   = nonTerminalRuleRecords[mapIndex]->end;

    if (ruleStartIndex < 1 || ruleEndIndex >= globalGrammar->GRAMMAR_RULES_SIZE) {
        fprintf(stderr,
                "Error: Invalid rule range [%d, %d] for non-terminal %d (%s)\n",
                ruleStartIndex,
                ruleEndIndex,
                mapIndex,
                getNonTerminal(mapIndex));
        return;
    }

    for (int ruleIndex = ruleStartIndex; ruleIndex <= ruleEndIndex; ruleIndex++) {
        if (globalGrammar->GRAMMAR_RULES[ruleIndex] == NULL) {
            fprintf(stderr, "Error: NULL rule at index %d\n", ruleIndex);
            continue;
        }
        struct Rule* rule = globalGrammar->GRAMMAR_RULES[ruleIndex];

        if (rule->symbols == NULL) {
            fprintf(stderr, "Error: NULL symbols in rule %d\n", ruleIndex);
            continue;
        }

        if (rule->symbols->HEAD_SYMBOL == NULL) {
            fprintf(stderr, "Error: NULL head symbol in rule %d\n", ruleIndex);
            continue;
        }

        struct Symbol* firstRhsSymbol = rule->symbols->HEAD_SYMBOL->next;
        if (firstRhsSymbol == NULL) {
            firstVector[mapIndex][TK_EPS] = 1;
            continue;
        }

        if (firstRhsSymbol->isTerminal) {
            if (firstRhsSymbol->symType.TERMINAL == TK_EPS) {
                firstVector[mapIndex][TK_EPS] = 1;
            } else {
                firstVector[mapIndex][firstRhsSymbol->symType.TERMINAL] = 1;
            }
        } else {
            int nextNonTerminalId = firstRhsSymbol->symType.NON_TERMINAL;
            if (nextNonTerminalId < 0 || nextNonTerminalId >= NUM_NONTERMINALS) {
                fprintf(stderr, "Error: Invalid non-terminal ID %d in rule %d\n", nextNonTerminalId, ruleIndex);
                continue;
            }
            if (!nonTerminalProcessed[nextNonTerminalId]) {
                computeFirstSets(nextNonTerminalId, firstVector);
            }
            for (int i = 0; i < NUM_TERMINALS; i++) {
                if (i != TK_EPS && firstVector[nextNonTerminalId][i] == 1) {
                    firstVector[mapIndex][i] = 1;
                }
            }
            if (firstVector[nextNonTerminalId][TK_EPS] == 1) {
                struct Symbol* nextSymbol = firstRhsSymbol->next;
                while (nextSymbol != NULL && firstVector[mapIndex][TK_EPS] == 0) {
                    if (nextSymbol->isTerminal) {
                        if (nextSymbol->symType.TERMINAL == TK_EPS) {
                            firstVector[mapIndex][TK_EPS] = 1;
                        } else {
                            firstVector[mapIndex][nextSymbol->symType.TERMINAL] = 1;
                            break;
                        }
                    } else {
                        int nextNextNonTerminalId = nextSymbol->symType.NON_TERMINAL;
                        if (nextNextNonTerminalId < 0 || nextNextNonTerminalId >= NUM_NONTERMINALS) {
                            fprintf(stderr, "Error: Invalid non-terminal ID %d in rule %d\n", nextNextNonTerminalId, ruleIndex);
                            break;
                        }
                        if (!nonTerminalProcessed[nextNextNonTerminalId]) {
                            computeFirstSets(nextNextNonTerminalId, firstVector);
                        }
                        for (int i = 0; i < NUM_TERMINALS; i++) {
                            if (i != TK_EPS && firstVector[nextNextNonTerminalId][i] == 1) {
                                firstVector[mapIndex][i] = 1;
                            }
                        }
                        if (firstVector[nextNextNonTerminalId][TK_EPS] == 0) {
                            break;
                        }
                    }
                    nextSymbol = nextSymbol->next;
                }
                if (nextSymbol == NULL) {
                    firstVector[mapIndex][TK_EPS] = 1;
                }
            }
        }
    }
}

// computes the first and follow sets for the given grammar
struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* parsedGrammar) {
    printf("[DEBUG] Starting first and follow computation\n");
    if (parsedGrammar == NULL) {
        fprintf(stderr, "Error: NULL grammar in computeFirstAndFollowSets\n");
        return NULL;
    }
    
    globalGrammar = parsedGrammar;
    // setup struct 
    struct FirstAndFollow* firstAndFollowSets = (struct FirstAndFollow*)malloc(sizeof(struct FirstAndFollow));
    if (firstAndFollowSets == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FirstAndFollow\n");
        return NULL;
    }

    firstAndFollowSets->firstSet = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
    if (firstAndFollowSets->firstSet == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for firstSet sets\n");
        free(firstAndFollowSets);
        return NULL;
    }

    firstAndFollowSets->followSet = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
    if (firstAndFollowSets->followSet == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for followSet sets\n");
        free(firstAndFollowSets->firstSet);
        free(firstAndFollowSets);
        return NULL;
    }

    int symbolVectorSize = NUM_TERMINALS;
    printf("[DEBUG] Initializing firstSet and followSet arrays\n");
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        firstAndFollowSets->firstSet[i] = (int*)calloc(symbolVectorSize, sizeof(int));
        if (firstAndFollowSets->firstSet[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for firstSet set row %d\n", i);
            for (int j = 0; j < i; j++) {
                free(firstAndFollowSets->firstSet[j]);
                free(firstAndFollowSets->followSet[j]);
            }
            free(firstAndFollowSets->firstSet);
            free(firstAndFollowSets->followSet);
            free(firstAndFollowSets);
            return NULL;
        }
        firstAndFollowSets->followSet[i] = (int*)calloc(symbolVectorSize, sizeof(int));
        if (firstAndFollowSets->followSet[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for followSet set row %d\n", i);
            free(firstAndFollowSets->firstSet[i]);
            for (int j = 0; j < i; j++) {
                free(firstAndFollowSets->firstSet[j]);
                free(firstAndFollowSets->followSet[j]);
            }
            free(firstAndFollowSets->firstSet);
            free(firstAndFollowSets->followSet);
            free(firstAndFollowSets);
            return NULL;
        }
    }

    printf("[DEBUG] Initializing processing flags\n");
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        nonTerminalProcessed[i] = false;
    }

    printf("[DEBUG] Computing firstSet sets\n");
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (!nonTerminalProcessed[i]) {
            printf("[DEBUG] Computing firstSet for non-terminal %d: %s\n", i, getNonTerminal(i));
            if (nonTerminalRuleRecords[i] == NULL) {
                fprintf(stderr, "Error: No rule records for non-terminal %d: %s\n", i, getNonTerminal(i));
                continue;
            }
            computeFirstSets(i, firstAndFollowSets->firstSet);
        }
    }

    printf("[DEBUG] Computing followSet sets\n");
    if (program >= 0 && program < NUM_NONTERMINALS && TK_DOLLAR >= 0 && TK_DOLLAR < NUM_TERMINALS) {
        firstAndFollowSets->followSet[program][TK_DOLLAR] = 1;
    } else {
        fprintf(stderr, "Error: Invalid program ID %d or TK_DOLLAR ID %d\n", program, TK_DOLLAR);
    }

    printf("[DEBUG] Allocating temporary storage for followSet iterations\n");
    int** prevFollowVector = (int**)malloc(NUM_NONTERMINALS * sizeof(int*));
    if (prevFollowVector == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for previous followSet sets\n");
        return firstAndFollowSets;
    }
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        prevFollowVector[i] = (int*)calloc(NUM_TERMINALS, sizeof(int));
        if (prevFollowVector[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for previous followSet set row %d\n", i);
            for (int j = 0; j < i; j++) {
                free(prevFollowVector[j]);
            }
            free(prevFollowVector);
            return firstAndFollowSets;
        }
    }
    for (int j = 0; j < NUM_TERMINALS; j++) {
        prevFollowVector[program][j] = firstAndFollowSets->followSet[program][j];
    }

    int iteration = 0;
    const int MAX_ITERATIONS = 100;
    while (iteration < MAX_ITERATIONS) {
        printf("[DEBUG] followSet iteration %d\n", iteration + 1);
        if (firstAndFollowSets->followSet == NULL || firstAndFollowSets->firstSet == NULL || parsedGrammar == NULL) {
            fprintf(stderr, "Error: NULL parameter in populateFollow\n");
        } else {
            for (int ruleIndex = 1; ruleIndex < parsedGrammar->GRAMMAR_RULES_SIZE; ruleIndex++) {
                if (parsedGrammar->GRAMMAR_RULES[ruleIndex] == NULL) {
                    fprintf(stderr, "Error: NULL rule at index %d\n", ruleIndex);
                    continue;
                }
                struct Rule* rule = parsedGrammar->GRAMMAR_RULES[ruleIndex];
                if (rule->symbols == NULL || rule->symbols->HEAD_SYMBOL == NULL) {
                    fprintf(stderr, "Error: NULL symbols or head symbol in rule %d\n", ruleIndex);
                    continue;
                }
                struct Symbol* lhsSymbol = rule->symbols->HEAD_SYMBOL;
                if (lhsSymbol->isTerminal) {
                    fprintf(stderr, "Error: LHS symbol in rule %d is a terminal\n", ruleIndex);
                    continue;
                }
                int lhsNonTerminalId = lhsSymbol->symType.NON_TERMINAL;
                if (lhsNonTerminalId < 0 || lhsNonTerminalId >= NUM_NONTERMINALS) {
                    fprintf(stderr, "Error: Invalid LHS non-terminal ID %d in rule %d\n", lhsNonTerminalId, ruleIndex);
                    continue;
                }
                struct Symbol* currentSymbol = lhsSymbol->next;
                while (currentSymbol != NULL) {
                    if (currentSymbol->isTerminal) {
                        currentSymbol = currentSymbol->next;
                        continue;
                    }
                    int currentNonTerminalId = currentSymbol->symType.NON_TERMINAL;
                    if (currentNonTerminalId < 0 || currentNonTerminalId >= NUM_NONTERMINALS) {
                        fprintf(stderr, "Error: Invalid RHS non-terminal ID %d in rule %d\n", currentNonTerminalId, ruleIndex);
                        currentSymbol = currentSymbol->next;
                        continue;
                    }
                    struct Symbol* nextSymbol = currentSymbol->next;
                    if (nextSymbol == NULL) {
                        for (int i = 0; i < NUM_TERMINALS; i++) {
                            if (firstAndFollowSets->followSet[lhsNonTerminalId][i] == 1)
                                firstAndFollowSets->followSet[currentNonTerminalId][i] = 1;
                        }
                    } else {
                        if (nextSymbol->isTerminal) {
                            if (nextSymbol->symType.TERMINAL != TK_EPS) {
                                int terminalId = nextSymbol->symType.TERMINAL;
                                if (terminalId >= 0 && terminalId < NUM_TERMINALS)
                                    firstAndFollowSets->followSet[currentNonTerminalId][terminalId] = 1;
                                else
                                    fprintf(stderr, "Error: Invalid terminal ID %d in rule %d\n", terminalId, ruleIndex);
                            }
                        } else {
                            int nextNonTerminalId = nextSymbol->symType.NON_TERMINAL;
                            if (nextNonTerminalId < 0 || nextNonTerminalId >= NUM_NONTERMINALS) {
                                fprintf(stderr, "Error: Invalid next non-terminal ID %d in rule %d\n", nextNonTerminalId, ruleIndex);
                                currentSymbol = currentSymbol->next;
                                continue;
                            }
                            for (int i = 0; i < NUM_TERMINALS; i++) {
                                if (i != TK_EPS && firstAndFollowSets->firstSet[nextNonTerminalId][i] == 1)
                                    firstAndFollowSets->followSet[currentNonTerminalId][i] = 1;
                            }
                            if (firstAndFollowSets->firstSet[nextNonTerminalId][TK_EPS] == 1) {
                                for (int i = 0; i < NUM_TERMINALS; i++) {
                                    if (firstAndFollowSets->followSet[lhsNonTerminalId][i] == 1)
                                        firstAndFollowSets->followSet[currentNonTerminalId][i] = 1;
                                }
                            }
                        }
                    }
                    currentSymbol = currentSymbol->next;
                }
            }
        }

        int stable = 1;
        for (int i = 0; i < NUM_NONTERMINALS; i++) {
            for (int j = 0; j < NUM_TERMINALS; j++) {
                if (prevFollowVector[i][j] != firstAndFollowSets->followSet[i][j]) {
                    stable = 0;
                    goto checkStability;
                }
            }
        }
checkStability:
        if (stable) {
            printf("[DEBUG] followSet sets stable after %d iterations\n", iteration + 1);
            break;
        }
        for (int i = 0; i < NUM_NONTERMINALS; i++) {
            for (int j = 0; j < NUM_TERMINALS; j++) {
                prevFollowVector[i][j] = firstAndFollowSets->followSet[i][j];
            }
        }
        iteration++;
    }
    if (iteration >= MAX_ITERATIONS) {
        fprintf(stderr, "Warning: followSet sets did not stabilize after %d iterations\n", MAX_ITERATIONS);
    }
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        free(prevFollowVector[i]);
    }
    free(prevFollowVector);
    printf("[DEBUG] followSet sets computation complete\n");

    verifyFirstAndFollow(firstAndFollowSets);
    printf("[DEBUG] First and Follow computation complete\n");
    return firstAndFollowSets;
}


void verifyFirstAndFollow(struct FirstAndFollow* firstAndFollowSets) {
    if (firstAndFollowSets == NULL) {
        fprintf(stderr, "Error: NULL FirstAndFollow in verifyFirstAndFollow\n");
        return;
    }
    printf("[DEBUG] Verifying First and Follow sets...\n");
    printf("\n+-------------------------------------------------------------------------+\n");
    printf("| %-15s | %-25s | %-25s |\n", "Non-terminal", "firstSet Set", "followSet Set");
    printf("|-----------------|---------------------------|---------------------------|\n");
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (firstAndFollowSets->firstSet[i] == NULL) {
            fprintf(stderr, "Error: NULL firstSet set for non-terminal %s (%d)\n", getNonTerminal(i), i);
            continue;
        }
        if (firstAndFollowSets->followSet[i] == NULL) {
            fprintf(stderr, "Error: NULL followSet set for non-terminal %s (%d)\n", getNonTerminal(i), i);
            continue;
        }
        printf("| %-15s | ", getNonTerminal(i));
        int firstFlag = 1;
        int isEmptyFirst = 1;
        printf("{");
        for (int j = 0; j < NUM_TERMINALS; j++) {
            if (firstAndFollowSets->firstSet[i][j] == 1) {
                if (!firstFlag) { printf(", "); }
                if (j == TK_EPS) {
                    printf("ε");
                } else {
                    printf("%s", getTerminal(j));
                }
                firstFlag = 0;
                isEmptyFirst = 0;
            }
        }
        if (isEmptyFirst) { printf("∅"); }
        printf("}%-*s | ", isEmptyFirst ? 24 : 25 - (int)printf("}"), "");
        int followFlag = 1;
        int isEmptyFollow = 1;
        printf("{");
        for (int j = 0; j < NUM_TERMINALS; j++) {
            if (firstAndFollowSets->followSet[i][j] == 1) {
                if (!followFlag) { printf(", "); }
                printf("%s", getTerminal(j));
                followFlag = 0;
                isEmptyFollow = 0;
            }
        }
        if (isEmptyFollow) { printf("∅"); }
        printf("} |\n");
    }
    printf("+-------------------------------------------------------------------------+\n");
    printf("[DEBUG] First and Follow verification complete\n");
}
