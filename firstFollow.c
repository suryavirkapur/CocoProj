/**
 * Group 46: First and Follow Sets Implementation
 * 
 * This module computes FIRST and FOLLOW sets for the grammar.
 */

#include <stdio.h>
#include <stdlib.h>
#include "firstFollow.h"
#include "constants.h"

// Global array to track processed non-terminals
extern bool nonTerminalProcessed[];

/**
 * Initialize the check if done array
 */
void initialiseCheckIfDone() {
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        nonTerminalProcessed[i] = false;
    }
}

/**
 * Initialize the First and Follow sets structure
 * 
 * @return Pointer to the initialized FirstAndFollow structure
 */
struct FirstAndFollow* createFirstAndFollowSets() {
    printf("Starting first and follow computation\n");
    // Allocate main structure
    struct FirstAndFollow* firstAndFollowSets = (struct FirstAndFollow*)malloc(sizeof(struct FirstAndFollow));
    if (firstAndFollowSets == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FirstAndFollow\n");
        return NULL;
    }

    // Allocate FIRST sets
    firstAndFollowSets->FIRST = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
    if (firstAndFollowSets->FIRST == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FIRST sets\n");
        free(firstAndFollowSets);
        return NULL;
    }

    // Allocate FOLLOW sets
    firstAndFollowSets->FOLLOW = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
    if (firstAndFollowSets->FOLLOW == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FOLLOW sets\n");
        free(firstAndFollowSets->FIRST);
        free(firstAndFollowSets);
        return NULL;
    }

    // Allocate and initialize each row
    int symbolVectorSize = NUM_TERMINALS;
    for (int nonTerminalIndex = 0; nonTerminalIndex < NUM_NONTERMINALS; nonTerminalIndex++) {
        // Allocate and zero-initialize FIRST set row
        firstAndFollowSets->FIRST[nonTerminalIndex] = (int*)calloc(symbolVectorSize, sizeof(int));
        if (firstAndFollowSets->FIRST[nonTerminalIndex] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for FIRST set row %d\n", nonTerminalIndex);
            
            // Clean up previously allocated memory
            for (int j = 0; j < nonTerminalIndex; j++) {
                free(firstAndFollowSets->FIRST[j]);
                free(firstAndFollowSets->FOLLOW[j]);
            }
            free(firstAndFollowSets->FIRST);
            free(firstAndFollowSets->FOLLOW);
            free(firstAndFollowSets);
            return NULL;
        }

        // Allocate and zero-initialize FOLLOW set row
        firstAndFollowSets->FOLLOW[nonTerminalIndex] = (int*)calloc(symbolVectorSize, sizeof(int));
        if (firstAndFollowSets->FOLLOW[nonTerminalIndex] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for FOLLOW set row %d\n", nonTerminalIndex);
            
            // Clean up previously allocated memory
            free(firstAndFollowSets->FIRST[nonTerminalIndex]);
            for (int j = 0; j < nonTerminalIndex; j++) {
                free(firstAndFollowSets->FIRST[j]);
                free(firstAndFollowSets->FOLLOW[j]);
            }
            free(firstAndFollowSets->FIRST);
            free(firstAndFollowSets->FOLLOW);
            free(firstAndFollowSets);
            return NULL;
        }
    }

    return firstAndFollowSets;
}

/**
 * Compute FIRST set for a specific non-terminal
 * 
 * @param firstVector Array of FIRST sets
 * @param enumId ID of the non-terminal
 */
void computeFirstSets(int** firstVector, int enumId) {
    if (enumId < 0 || enumId >= NUM_NONTERMINALS) {
        fprintf(stderr, "Error: Invalid non-terminal ID %d in computeFirstSets\n", enumId);
        return;
    }
    
    // Already processed?
    if (nonTerminalProcessed[enumId]) {
        return;
    }
    
    // Mark as processed to avoid infinite recursion
    nonTerminalProcessed[enumId] = true;
    
    // Safety check for rule records
    if (nonTerminalRuleRecords[enumId] == NULL) {
        fprintf(stderr, "Error: NULL rule record for non-terminal %d (%s)\n", 
                enumId, getNonTerminal(enumId));
        return;
    }
    
    int ruleStartIndex = nonTerminalRuleRecords[enumId]->start;
    int ruleEndIndex = nonTerminalRuleRecords[enumId]->end;
    
    // Validate rule indices
    if (ruleStartIndex < 1 || ruleEndIndex >= parsedGrammar->GRAMMAR_RULES_SIZE) {
        fprintf(stderr, "Error: Invalid rule range [%d, %d] for non-terminal %d (%s)\n", 
                ruleStartIndex, ruleEndIndex, enumId, getNonTerminal(enumId));
        return;
    }
    
    // Process each rule for this non-terminal
    for (int ruleIndex = ruleStartIndex; ruleIndex <= ruleEndIndex; ruleIndex++) {
        // Safety check for rule
        if (parsedGrammar->GRAMMAR_RULES[ruleIndex] == NULL) {
            fprintf(stderr, "Error: NULL rule at index %d\n", ruleIndex);
            continue;
        }
        
        struct Rule* rule = parsedGrammar->GRAMMAR_RULES[ruleIndex];
        
        // Safety check for symbols
        if (rule->SYMBOLS == NULL) {
            fprintf(stderr, "Error: NULL symbols in rule %d\n", ruleIndex);
            continue;
        }
        
        // Safety check for head symbol
        if (rule->SYMBOLS->HEAD_SYMBOL == NULL) {
            fprintf(stderr, "Error: NULL head symbol in rule %d\n", ruleIndex);
            continue;
        }
        
        // Get the first symbol of the RHS
        struct Symbol* firstRhsSymbol = rule->SYMBOLS->HEAD_SYMBOL->next;
        
        // Empty rule (epsilon production)
        if (firstRhsSymbol == NULL) {
            firstVector[enumId][TK_EPS] = 1;
            continue;
        }
        
        // Process the first symbol of the RHS
        if (firstRhsSymbol->IS_TERMINAL) {
            if (firstRhsSymbol->TYPE.TERMINAL == TK_EPS) {
                firstVector[enumId][TK_EPS] = 1;
            } else {
                firstVector[enumId][firstRhsSymbol->TYPE.TERMINAL] = 1;
            }
        } else {
            // Process a non-terminal
            int nextNonTerminalId = firstRhsSymbol->TYPE.NON_TERMINAL;
            
            // Safety check
            if (nextNonTerminalId < 0 || nextNonTerminalId >= NUM_NONTERMINALS) {
                fprintf(stderr, "Error: Invalid non-terminal ID %d in rule %d\n", 
                        nextNonTerminalId, ruleIndex);
                continue;
            }
            
            // Recursively compute FIRST set if needed
            if (!nonTerminalProcessed[nextNonTerminalId]) {
                computeFirstSets(firstVector, nextNonTerminalId);
            }
            
            // Add all terminals from FIRST(nextNonTerminal) to FIRST(enumId)
            for (int i = 0; i < NUM_TERMINALS; i++) {
                if (i != TK_EPS && firstVector[nextNonTerminalId][i] == 1) {
                    firstVector[enumId][i] = 1;
                }
            }
            
            // If nextNonTerminal can derive epsilon, check next symbol
            if (firstVector[nextNonTerminalId][TK_EPS] == 1) {
                struct Symbol* nextSymbol = firstRhsSymbol->next;
                while (nextSymbol != NULL && firstVector[enumId][TK_EPS] == 0) {
                    if (nextSymbol->IS_TERMINAL) {
                        if (nextSymbol->TYPE.TERMINAL == TK_EPS) {
                            firstVector[enumId][TK_EPS] = 1;
                        } else {
                            firstVector[enumId][nextSymbol->TYPE.TERMINAL] = 1;
                            break; // No more epsilon
                        }
                    } else {
                        int nextNextNonTerminalId = nextSymbol->TYPE.NON_TERMINAL;
                        
                        // Safety check
                        if (nextNextNonTerminalId < 0 || nextNextNonTerminalId >= NUM_NONTERMINALS) {
                            fprintf(stderr, "Error: Invalid non-terminal ID %d in rule %d\n", 
                                    nextNextNonTerminalId, ruleIndex);
                            break;
                        }
                        
                        // Recursively compute FIRST set if needed
                        if (!nonTerminalProcessed[nextNextNonTerminalId]) {
                            computeFirstSets(firstVector, nextNextNonTerminalId);
                        }
                        
                        // Add all terminals from FIRST(nextNextNonTerminal) to FIRST(enumId)
                        for (int i = 0; i < NUM_TERMINALS; i++) {
                            if (i != TK_EPS && firstVector[nextNextNonTerminalId][i] == 1) {
                                firstVector[enumId][i] = 1;
                            }
                        }
                        
                        // If nextNextNonTerminal cannot derive epsilon, we're done
                        if (firstVector[nextNextNonTerminalId][TK_EPS] == 0) {
                            break;
                        }
                    }
                    nextSymbol = nextSymbol->next;
                }
                
                // If we reached the end, this rule can derive epsilon
                if (nextSymbol == NULL) {
                    firstVector[enumId][TK_EPS] = 1;
                }
            }
        }
    }
}

/**
 * Populate FIRST sets for all non-terminals
 * 
 * @param firstVector Array to store FIRST sets
 * @param parsedGrammar The grammar structure
 */
void populateFirst(int** firstVector, struct Grammar* parsedGrammar) {
    // Initialize the check array
    initialiseCheckIfDone();
    
    // Compute FIRST set for each non-terminal
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (!nonTerminalProcessed[i]) {
            computeFirstSets(firstVector, i);
        }
    }
}

/**
 * Populate FOLLOW sets for all non-terminals (single iteration)
 * 
 * @param followVector Array to store FOLLOW sets
 * @param firstVector FIRST sets array
 * @param parsedGrammar The grammar structure
 */
void populateFollow(int** followVector, int** firstVector, struct Grammar* parsedGrammar) {
    if (followVector == NULL || firstVector == NULL || parsedGrammar == NULL) {
        fprintf(stderr, "Error: NULL parameter in populateFollow\n");
        return;
    }
    
    // Process all grammar rules
    for (int ruleIndex = 1; ruleIndex < parsedGrammar->GRAMMAR_RULES_SIZE; ruleIndex++) {
        // Safety check for the rule
        if (parsedGrammar->GRAMMAR_RULES[ruleIndex] == NULL) {
            fprintf(stderr, "Error: NULL rule at index %d\n", ruleIndex);
            continue;
        }
        
        struct Rule* rule = parsedGrammar->GRAMMAR_RULES[ruleIndex];
        
        // Safety check for symbols
        if (rule->SYMBOLS == NULL || rule->SYMBOLS->HEAD_SYMBOL == NULL) {
            fprintf(stderr, "Error: NULL symbols or head symbol in rule %d\n", ruleIndex);
            continue;
        }
        
        struct Symbol* lhsSymbol = rule->SYMBOLS->HEAD_SYMBOL;
        
        // Safety check for LHS non-terminal
        if (lhsSymbol->IS_TERMINAL) {
            fprintf(stderr, "Error: LHS symbol in rule %d is a terminal\n", ruleIndex);
            continue;
        }
        
        int lhsNonTerminalId = lhsSymbol->TYPE.NON_TERMINAL;
        
        // Validate non-terminal ID
        if (lhsNonTerminalId < 0 || lhsNonTerminalId >= NUM_NONTERMINALS) {
            fprintf(stderr, "Error: Invalid LHS non-terminal ID %d in rule %d\n", 
                    lhsNonTerminalId, ruleIndex);
            continue;
        }
        
        // Process each symbol in the RHS
        struct Symbol* currentSymbol = lhsSymbol->next;
        
        while (currentSymbol != NULL) {
            // Skip terminals - only interested in non-terminals for FOLLOW sets
            if (currentSymbol->IS_TERMINAL) {
                currentSymbol = currentSymbol->next;
                continue;
            }
            
            int currentNonTerminalId = currentSymbol->TYPE.NON_TERMINAL;
            
            // Validate non-terminal ID
            if (currentNonTerminalId < 0 || currentNonTerminalId >= NUM_NONTERMINALS) {
                fprintf(stderr, "Error: Invalid RHS non-terminal ID %d in rule %d\n", 
                        currentNonTerminalId, ruleIndex);
                currentSymbol = currentSymbol->next;
                continue;
            }
            
            // Get the next symbol after current
            struct Symbol* nextSymbol = currentSymbol->next;
            
            // Case 1: Current is the last symbol
            if (nextSymbol == NULL) {
                // Add FOLLOW(LHS) to FOLLOW(current)
                for (int i = 0; i < NUM_TERMINALS; i++) {
                    if (followVector[lhsNonTerminalId][i] == 1) {
                        followVector[currentNonTerminalId][i] = 1;
                    }
                }
            } else {
                // Case 2: Next symbol is a terminal
                if (nextSymbol->IS_TERMINAL) {
                    // Skip epsilon
                    if (nextSymbol->TYPE.TERMINAL != TK_EPS) {
                        // Add the terminal to FOLLOW(current)
                        int terminalId = nextSymbol->TYPE.TERMINAL;
                        
                        // Validate terminal ID
                        if (terminalId >= 0 && terminalId < NUM_TERMINALS) {
                            followVector[currentNonTerminalId][terminalId] = 1;
                        } else {
                            fprintf(stderr, "Error: Invalid terminal ID %d in rule %d\n", 
                                    terminalId, ruleIndex);
                        }
                    }
                } else {
                    // Case 3: Next symbol is a non-terminal
                    int nextNonTerminalId = nextSymbol->TYPE.NON_TERMINAL;
                    
                    // Validate non-terminal ID
                    if (nextNonTerminalId < 0 || nextNonTerminalId >= NUM_NONTERMINALS) {
                        fprintf(stderr, "Error: Invalid next non-terminal ID %d in rule %d\n", 
                                nextNonTerminalId, ruleIndex);
                        currentSymbol = currentSymbol->next;
                        continue;
                    }
                    
                    // Add FIRST(next) - {ε} to FOLLOW(current)
                    for (int i = 0; i < NUM_TERMINALS; i++) {
                        if (i != TK_EPS && firstVector[nextNonTerminalId][i] == 1) {
                            followVector[currentNonTerminalId][i] = 1;
                        }
                    }
                    
                    // If next can derive epsilon, also add FOLLOW(LHS) to FOLLOW(current)
                    if (firstVector[nextNonTerminalId][TK_EPS] == 1) {
                        for (int i = 0; i < NUM_TERMINALS; i++) {
                            if (followVector[lhsNonTerminalId][i] == 1) {
                                followVector[currentNonTerminalId][i] = 1;
                            }
                        }
                    }
                }
            }
            
            currentSymbol = currentSymbol->next;
        }
    }
}

/**
 * Populate FOLLOW sets until they stabilize (fixed-point iteration)
 * 
 * @param followVector Array to store FOLLOW sets
 * @param firstVector FIRST sets array
 * @param parsedGrammar The grammar structure
 */
void populateFollowTillStable(int** followVector, int** firstVector, struct Grammar* parsedGrammar) {
    printf("Starting populateFollowTillStable\n");
    
    if (followVector == NULL || firstVector == NULL || parsedGrammar == NULL) {
        fprintf(stderr, "Error: NULL parameter in populateFollowTillStable\n");
        return;
    }
    
    // Create a copy of the FOLLOW sets to check for stability
    printf("Allocating previous FOLLOW sets\n");
    int** prevFollowVector = (int**)malloc(NUM_NONTERMINALS * sizeof(int*));
    if (prevFollowVector == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for previous FOLLOW sets\n");
        return;
    }
    
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        prevFollowVector[i] = (int*)calloc(NUM_TERMINALS, sizeof(int));
        if (prevFollowVector[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for previous FOLLOW set row %d\n", i);
            
            // Clean up previously allocated memory
            for (int j = 0; j < i; j++) {
                free(prevFollowVector[j]);
            }
            free(prevFollowVector);
            return;
        }
    }
    
    // Initialize FOLLOW(start symbol) with $
    printf("Adding $ to FOLLOW(program)\n");
    if (program >= 0 && program < NUM_NONTERMINALS && TK_DOLLAR >= 0 && TK_DOLLAR < NUM_TERMINALS) {
        followVector[program][TK_DOLLAR] = 1;
        prevFollowVector[program][TK_DOLLAR] = 1;
    } else {
        fprintf(stderr, "Error: Invalid program ID %d or TK_DOLLAR ID %d\n", program, TK_DOLLAR);
    }
    
    // Iterate until FOLLOW sets stabilize
    int iteration = 0;
    const int MAX_ITERATIONS = 100; // Safety limit to prevent infinite loops
    
    while (iteration < MAX_ITERATIONS) {
        printf("FOLLOW iteration %d\n", iteration + 1);
        // Perform one iteration
        populateFollow(followVector, firstVector, parsedGrammar);
        
        // Check if FOLLOW sets have changed
        bool stable = true;
        for (int i = 0; i < NUM_NONTERMINALS; i++) {
            for (int j = 0; j < NUM_TERMINALS; j++) {
                if (prevFollowVector[i][j] != followVector[i][j]) {
                    stable = false;
                    goto checkStability;  // Break out of both loops
                }
            }
        }
        
    checkStability:
        if (stable) {
            printf("FOLLOW sets stable after %d iterations\n", iteration + 1);
            break;  // FOLLOW sets have stabilized
        }
        
        // Update previous FOLLOW sets
        for (int i = 0; i < NUM_NONTERMINALS; i++) {
            for (int j = 0; j < NUM_TERMINALS; j++) {
                prevFollowVector[i][j] = followVector[i][j];
            }
        }
        
        iteration++;
    }
    
    if (iteration >= MAX_ITERATIONS) {
        fprintf(stderr, "Warning: FOLLOW sets did not stabilize after %d iterations\n", MAX_ITERATIONS);
    }
    
    // Free memory
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        free(prevFollowVector[i]);
    }
    free(prevFollowVector);
    
    printf("FOLLOW sets computation complete\n");
}

/**
 * Verify and print the computed FIRST and FOLLOW sets
 * 
 * @param firstAndFollowSets The computed sets
 */
void verifyFirstAndFollow(struct FirstAndFollow* firstAndFollowSets) {
    if (firstAndFollowSets == NULL) {
        fprintf(stderr, "Error: NULL FirstAndFollow in verifyFirstAndFollow\n");
        return;
    }
    
    printf("Verifying First and Follow sets...\n");
    
    // Verify FIRST sets
    printf("\nFIRST Sets:\n");
    printf("===========\n");
    
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (firstAndFollowSets->FIRST[i] == NULL) {
            fprintf(stderr, "Error: NULL FIRST set for non-terminal %s (%d)\n", getNonTerminal(i), i);
            continue;
        }
        
        printf("FIRST(%s) = { ", getNonTerminal(i));
        bool empty = true;
        
        for (int j = 0; j < NUM_TERMINALS; j++) {
            if (firstAndFollowSets->FIRST[i][j] == 1) {
                if (j == TK_EPS) {
                    printf("ε ");
                } else {
                    printf("%s ", getTerminal(j));
                }
                empty = false;
            }
        }
        
        if (empty) {
            printf("∅");  // Empty set
        }
        
        printf(" }\n");
    }
    
    // Verify FOLLOW sets
    printf("\nFOLLOW Sets:\n");
    printf("============\n");
    
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (firstAndFollowSets->FOLLOW[i] == NULL) {
            fprintf(stderr, "Error: NULL FOLLOW set for non-terminal %s (%d)\n", getNonTerminal(i), i);
            continue;
        }
        
        printf("FOLLOW(%s) = { ", getNonTerminal(i));
        bool empty = true;
        
        for (int j = 0; j < NUM_TERMINALS; j++) {
            if (firstAndFollowSets->FOLLOW[i][j] == 1) {
                printf("%s ", getTerminal(j));
                empty = false;
            }
        }
        
        if (empty) {
            printf("∅");  // Empty set
        }
        
        printf(" }\n");
    }
    
    printf("\nFirst and Follow verification complete\n");
}

/**
 * Compute both FIRST and FOLLOW sets for the grammar
 * 
 * @param parsedGrammar The grammar structure
 * @return Pointer to the computed FirstAndFollow structure
 */
struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* parsedGrammar) {
    printf("Starting first and follow computation\n");
    
    if (parsedGrammar == NULL) {
        fprintf(stderr, "Error: NULL grammar in computeFirstAndFollowSets\n");
        return NULL;
    }
    
    printf("Creating FirstAndFollow structure\n");
    // Create the FirstAndFollow structure with additional safety
    struct FirstAndFollow* firstAndFollowSets = (struct FirstAndFollow*)malloc(sizeof(struct FirstAndFollow));
    if (firstAndFollowSets == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FirstAndFollow\n");
        return NULL;
    }
    
    printf("Allocating FIRST array\n");
    // Allocate the FIRST array
    firstAndFollowSets->FIRST = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
    if (firstAndFollowSets->FIRST == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FIRST sets\n");
        free(firstAndFollowSets);
        return NULL;
    }
    
    printf("Allocating FOLLOW array\n");
    // Allocate the FOLLOW array
    firstAndFollowSets->FOLLOW = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
    if (firstAndFollowSets->FOLLOW == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for FOLLOW sets\n");
        free(firstAndFollowSets->FIRST);
        free(firstAndFollowSets);
        return NULL;
    }
    
    printf("Initializing arrays\n");
    // Calculate the symbolVectorSize safely
    int symbolVectorSize = NUM_TERMINALS;
    
    // Initialize each row of the arrays
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        firstAndFollowSets->FIRST[i] = (int*)calloc(symbolVectorSize, sizeof(int));
        if (firstAndFollowSets->FIRST[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for FIRST set row %d\n", i);
            
            // Clean up previously allocated memory
            for (int j = 0; j < i; j++) {
                free(firstAndFollowSets->FIRST[j]);
            }
            free(firstAndFollowSets->FIRST);
            free(firstAndFollowSets->FOLLOW);
            free(firstAndFollowSets);
            return NULL;
        }
        
        firstAndFollowSets->FOLLOW[i] = (int*)calloc(symbolVectorSize, sizeof(int));
        if (firstAndFollowSets->FOLLOW[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for FOLLOW set row %d\n", i);
            
            // Clean up previously allocated memory
            free(firstAndFollowSets->FIRST[i]);
            for (int j = 0; j < i; j++) {
                free(firstAndFollowSets->FIRST[j]);
                free(firstAndFollowSets->FOLLOW[j]);
            }
            free(firstAndFollowSets->FIRST);
            free(firstAndFollowSets->FOLLOW);
            free(firstAndFollowSets);
            return NULL;
        }
    }
    
    printf("Initializing processing flags\n");
    // Initialize processing flags
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        nonTerminalProcessed[i] = false;
    }
    
    printf("Computing FIRST sets\n");
    // Compute FIRST sets with safety checks
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        if (!nonTerminalProcessed[i]) {
            printf("Computing FIRST for non-terminal %d: %s\n", i, getNonTerminal(i));
            if (nonTerminalRuleRecords[i] == NULL) {
                fprintf(stderr, "Error: No rule records for non-terminal %d: %s\n", 
                        i, getNonTerminal(i));
                continue;
            }
            computeFirstSets(firstAndFollowSets->FIRST, i);
        }
    }
    
    printf("Computing FOLLOW sets\n");
    // Add $ to FOLLOW of start symbol (program)
    firstAndFollowSets->FOLLOW[program][TK_DOLLAR] = 1;
    
    // Compute FOLLOW sets
    populateFollowTillStable(firstAndFollowSets->FOLLOW, firstAndFollowSets->FIRST, parsedGrammar);
    
    printf("First and Follow computation complete\n");
    return firstAndFollowSets;
}