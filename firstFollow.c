#include <stdio.h>
#include <stdlib.h>
#include "firstFollow.h"
#include "constants.h"

void initialiseCheckIfDone() {
    for(int i=0; i < NUM_NONTERMINALS; i++)
        checkIfDone[i] = 0;
}

struct FirstAndFollow* initializeFirstAndFollow() {
    struct FirstAndFollow* fafl = (struct FirstAndFollow*)malloc(sizeof(struct FirstAndFollow));
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
        
        struct Rule* r = g->GRAMMAR_RULES[i];
        if (r->SYMBOLS == NULL || r->SYMBOLS->HEAD_SYMBOL == NULL) {
            printf("ERROR: NULL symbols in rule %d\n", i);
            continue;
        }
        
        struct Symbol* s = r->SYMBOLS->HEAD_SYMBOL;
        struct Symbol* trav = s;
        struct Symbol* nextSymbol = trav->next;
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

void populateFirst(int** firstVector, struct Grammar* g) {
    for(int i=0; i < NUM_NONTERMINALS; i++) {
        if(checkIfDone[i] == 0)
            calculateFirst(firstVector, i);
    }
}

void populateFollow(int** followVector, int** firstVector, struct Grammar* g) {
    for(int i=1; i <= NUM_GRAMMAR_RULES; i++) {
        struct Rule* r = g->GRAMMAR_RULES[i];
        struct Symbol* head = r->SYMBOLS->HEAD_SYMBOL;
        struct Symbol* trav = head->next;
        int epsilonIdentifier = 0;
        while(trav != NULL) {
            if(trav->IS_TERMINAL == 0) {
                struct Symbol* followTrav = trav->next;
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

void populateFollowTillStable(int** followVector, int** firstVector, struct Grammar* g) {
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

void verifyFirstAndFollow(struct FirstAndFollow* fafl) {
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

struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* g) {
    struct FirstAndFollow* fafl = initializeFirstAndFollow();
    populateFirst(fafl->FIRST, g);
    populateFollowTillStable(fafl->FOLLOW, fafl->FIRST, g);
    
    // Add validation
    verifyFirstAndFollow(fafl);
    
    return fafl;
}