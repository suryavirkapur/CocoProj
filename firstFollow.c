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

extern bool nonTerminalProcessed[];

void initialiseCheckIfDone() {
  for (int i = 0; i < NUM_NONTERMINALS; i++) { nonTerminalProcessed[i] = false; }
}

struct FirstAndFollow* createFirstAndFollowSets() {
  printf("[DEBUG] Starting first and follow computation\n");
  struct FirstAndFollow* firstAndFollowSets = (struct FirstAndFollow*)malloc(sizeof(struct FirstAndFollow));
  if (firstAndFollowSets == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for FirstAndFollow\n");
    return NULL;
  }

  firstAndFollowSets->FIRST = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
  if (firstAndFollowSets->FIRST == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for FIRST sets\n");
    free(firstAndFollowSets);
    return NULL;
  }

  firstAndFollowSets->FOLLOW = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
  if (firstAndFollowSets->FOLLOW == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for FOLLOW sets\n");
    free(firstAndFollowSets->FIRST);
    free(firstAndFollowSets);
    return NULL;
  }

  int symbolVectorSize = NUM_TERMINALS;
  for (int nonTerminalIndex = 0; nonTerminalIndex < NUM_NONTERMINALS; nonTerminalIndex++) {
    firstAndFollowSets->FIRST[nonTerminalIndex] = (int*)calloc(symbolVectorSize, sizeof(int));
    if (firstAndFollowSets->FIRST[nonTerminalIndex] == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for FIRST set row %d\n", nonTerminalIndex);

      for (int j = 0; j < nonTerminalIndex; j++) {
        free(firstAndFollowSets->FIRST[j]);
        free(firstAndFollowSets->FOLLOW[j]);
      }
      free(firstAndFollowSets->FIRST);
      free(firstAndFollowSets->FOLLOW);
      free(firstAndFollowSets);
      return NULL;
    }

    firstAndFollowSets->FOLLOW[nonTerminalIndex] = (int*)calloc(symbolVectorSize, sizeof(int));
    if (firstAndFollowSets->FOLLOW[nonTerminalIndex] == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for FOLLOW set row %d\n", nonTerminalIndex);

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

void computeFirstSets(int enumId, int** firstVector) {
 printf("[DEBUG] Starting computeFirstSets\n");
  if (enumId < 0 || enumId >= NUM_NONTERMINALS) {
    fprintf(stderr, "Error: Invalid non-terminal ID %d in computeFirstSets\n", enumId);
    return;
  }

  if (nonTerminalProcessed[enumId]) { return; }

  nonTerminalProcessed[enumId] = true;

  if (nonTerminalRuleRecords[enumId] == NULL) {
    fprintf(stderr, "Error: NULL rule record for non-terminal %d (%s)\n", enumId, getNonTerminal(enumId));
    return;
  }

  int ruleStartIndex = nonTerminalRuleRecords[enumId]->start;
  int ruleEndIndex   = nonTerminalRuleRecords[enumId]->end;

  if (ruleStartIndex < 1 || ruleEndIndex >= parsedGrammar->GRAMMAR_RULES_SIZE) {
    fprintf(stderr,
            "Error: Invalid rule range [%d, %d] for non-terminal %d (%s)\n",
            ruleStartIndex,
            ruleEndIndex,
            enumId,
            getNonTerminal(enumId));
    return;
  }

  for (int ruleIndex = ruleStartIndex; ruleIndex <= ruleEndIndex; ruleIndex++) {
    if (parsedGrammar->GRAMMAR_RULES[ruleIndex] == NULL) {
      fprintf(stderr, "Error: NULL rule at index %d\n", ruleIndex);
      continue;
    }

    struct Rule* rule = parsedGrammar->GRAMMAR_RULES[ruleIndex];

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
      firstVector[enumId][TK_EPS] = 1;
      continue;
    }

    if (firstRhsSymbol->isTerminal) {
      if (firstRhsSymbol->symType.TERMINAL == TK_EPS) {
        firstVector[enumId][TK_EPS] = 1;
      } else {
        firstVector[enumId][firstRhsSymbol->symType.TERMINAL] = 1;
      }
    } else {
      int nextNonTerminalId = firstRhsSymbol->symType.NON_TERMINAL;

      if (nextNonTerminalId < 0 || nextNonTerminalId >= NUM_NONTERMINALS) {
        fprintf(stderr, "Error: Invalid non-terminal ID %d in rule %d\n", nextNonTerminalId, ruleIndex);
        continue;
      }

      if (!nonTerminalProcessed[nextNonTerminalId]) { computeFirstSets(nextNonTerminalId, firstVector); }

      for (int i = 0; i < NUM_TERMINALS; i++) {
        if (i != TK_EPS && firstVector[nextNonTerminalId][i] == 1) { firstVector[enumId][i] = 1; }
      }

      if (firstVector[nextNonTerminalId][TK_EPS] == 1) {
        struct Symbol* nextSymbol = firstRhsSymbol->next;
        while (nextSymbol != NULL && firstVector[enumId][TK_EPS] == 0) {
          if (nextSymbol->isTerminal) {
            if (nextSymbol->symType.TERMINAL == TK_EPS) {
              firstVector[enumId][TK_EPS] = 1;
            } else {
              firstVector[enumId][nextSymbol->symType.TERMINAL] = 1;
              break;
            }
          } else {
            int nextNextNonTerminalId = nextSymbol->symType.NON_TERMINAL;

            if (nextNextNonTerminalId < 0 || nextNextNonTerminalId >= NUM_NONTERMINALS) {
              fprintf(stderr, "Error: Invalid non-terminal ID %d in rule %d\n", nextNextNonTerminalId, ruleIndex);
              break;
            }

            if (!nonTerminalProcessed[nextNextNonTerminalId]) { computeFirstSets(nextNextNonTerminalId, firstVector); }

            for (int i = 0; i < NUM_TERMINALS; i++) {
              if (i != TK_EPS && firstVector[nextNextNonTerminalId][i] == 1) { firstVector[enumId][i] = 1; }
            }

            if (firstVector[nextNextNonTerminalId][TK_EPS] == 0) { break; }
          }
          nextSymbol = nextSymbol->next;
        }

        if (nextSymbol == NULL) { firstVector[enumId][TK_EPS] = 1; }
      }
    }
  }
}

void fillFirst(int** firstVector, struct Grammar* parsedGrammar) {
  initialiseCheckIfDone();

  for (int i = 0; i < NUM_NONTERMINALS; i++) {
    if (!nonTerminalProcessed[i]) { computeFirstSets(i, firstVector); }
  }
}

void populateFollow(int** followVector, int** firstVector, struct Grammar* parsedGrammar) {
  if (followVector == NULL || firstVector == NULL || parsedGrammar == NULL) {
    fprintf(stderr, "Error: NULL parameter in populateFollow\n");
    return;
  }

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
          if (followVector[lhsNonTerminalId][i] == 1) { followVector[currentNonTerminalId][i] = 1; }
        }
      } else {
        if (nextSymbol->isTerminal) {
          if (nextSymbol->symType.TERMINAL != TK_EPS) {
            int terminalId = nextSymbol->symType.TERMINAL;

            if (terminalId >= 0 && terminalId < NUM_TERMINALS) {
              followVector[currentNonTerminalId][terminalId] = 1;
            } else {
              fprintf(stderr, "Error: Invalid terminal ID %d in rule %d\n", terminalId, ruleIndex);
            }
          }
        } else {
          int nextNonTerminalId = nextSymbol->symType.NON_TERMINAL;

          if (nextNonTerminalId < 0 || nextNonTerminalId >= NUM_NONTERMINALS) {
            fprintf(stderr, "Error: Invalid next non-terminal ID %d in rule %d\n", nextNonTerminalId, ruleIndex);
            currentSymbol = currentSymbol->next;
            continue;
          }

          for (int i = 0; i < NUM_TERMINALS; i++) {
            if (i != TK_EPS && firstVector[nextNonTerminalId][i] == 1) { followVector[currentNonTerminalId][i] = 1; }
          }

          if (firstVector[nextNonTerminalId][TK_EPS] == 1) {
            for (int i = 0; i < NUM_TERMINALS; i++) {
              if (followVector[lhsNonTerminalId][i] == 1) { followVector[currentNonTerminalId][i] = 1; }
            }
          }
        }
      }

      currentSymbol = currentSymbol->next;
    }
  }
}

void fillFollowF(int** followVector, int** firstVector, struct Grammar* parsedGrammar) {

  printf("[DEBUG] Starting fillFollowF\n");

  if (followVector == NULL || firstVector == NULL || parsedGrammar == NULL) {
    fprintf(stderr, "Error: NULL parameter in fillFollowF\n");
    return;
  }

  printf("[DEBUG] Allocating previous FOLLOW sets\n");
  int** prevFollowVector = (int**)malloc(NUM_NONTERMINALS * sizeof(int*));
  if (prevFollowVector == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for previous FOLLOW sets\n");
    return;
  }

  for (int i = 0; i < NUM_NONTERMINALS; i++) {
    prevFollowVector[i] = (int*)calloc(NUM_TERMINALS, sizeof(int));
    if (prevFollowVector[i] == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for previous FOLLOW set row %d\n", i);

      for (int j = 0; j < i; j++) { free(prevFollowVector[j]); }
      free(prevFollowVector);
      return;
    }
  }

  printf("[DEBUG] Adding $ to FOLLOW(program)\n");
  if (program >= 0 && program < NUM_NONTERMINALS && TK_DOLLAR >= 0 && TK_DOLLAR < NUM_TERMINALS) {
    followVector[program][TK_DOLLAR]     = 1;
    prevFollowVector[program][TK_DOLLAR] = 1;
  } else {
    fprintf(stderr, "Error: Invalid program ID %d or TK_DOLLAR ID %d\n", program, TK_DOLLAR);
  }

  int       iteration      = 0;
  const int MAX_ITERATIONS = 100;

  while (iteration < MAX_ITERATIONS) {
    printf("[DEBUG] FOLLOW iteration %d\n", iteration + 1);
    populateFollow(followVector, firstVector, parsedGrammar);

    bool stable = true;
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
      for (int j = 0; j < NUM_TERMINALS; j++) {
        if (prevFollowVector[i][j] != followVector[i][j]) {
          stable = false;
          goto checkStability;
        }
      }
    }

  checkStability:
    if (stable) {
      printf("[DEBUG] FOLLOW sets stable after %d iterations\n", iteration + 1);
      break;
    }

    for (int i = 0; i < NUM_NONTERMINALS; i++) {
      for (int j = 0; j < NUM_TERMINALS; j++) { prevFollowVector[i][j] = followVector[i][j]; }
    }

    iteration++;
  }

  if (iteration >= MAX_ITERATIONS) {
    fprintf(stderr, "Warning: FOLLOW sets did not stabilize after %d iterations\n", MAX_ITERATIONS);
  }

  for (int i = 0; i < NUM_NONTERMINALS; i++) { free(prevFollowVector[i]); }
  free(prevFollowVector);

  printf("[DEBUG] FOLLOW sets computation complete\n");
}

void verifyFirstAndFollow(struct FirstAndFollow* firstAndFollowSets) {
  if (firstAndFollowSets == NULL) {
    fprintf(stderr, "Error: NULL FirstAndFollow in verifyFirstAndFollow\n");
    return;
  }

  printf("[DEBUG] Verifying First and Follow sets...\n");

  printf("\n+-------------------------------------------------------------------------+\n");
  printf("| %-15s | %-25s | %-25s |\n", "Non-terminal", "FIRST Set", "FOLLOW Set");
  printf("|-----------------|---------------------------|---------------------------|\n");

  for (int i = 0; i < NUM_NONTERMINALS; i++) {
    if (firstAndFollowSets->FIRST[i] == NULL) {
      fprintf(stderr, "Error: NULL FIRST set for non-terminal %s (%d)\n", getNonTerminal(i), i);
      continue;
    }
    if (firstAndFollowSets->FOLLOW[i] == NULL) {
      fprintf(stderr, "Error: NULL FOLLOW set for non-terminal %s (%d)\n", getNonTerminal(i), i);
      continue;
    }

    printf("| %-15s | ", getNonTerminal(i));

    bool first        = true;
    bool isEmptyFirst = true;
    printf("{");
    for (int j = 0; j < NUM_TERMINALS; j++) {
      if (firstAndFollowSets->FIRST[i][j] == 1) {
        if (!first) { printf(", "); }
        if (j == TK_EPS) {
          printf("ε");
        } else {
          printf("%s", getTerminal(j));
        }
        first        = false;
        isEmptyFirst = false;
      }
    }
    if (isEmptyFirst) { printf("∅"); }

    printf("%-*s | ", isEmptyFirst ? 24 : 25 - (int)printf("}"), "");

    first              = true;
    bool isEmptyFollow = true;
    printf("{");
    for (int j = 0; j < NUM_TERMINALS; j++) {
      if (firstAndFollowSets->FOLLOW[i][j] == 1) {
        if (!first) { printf(", "); }
        printf("%s", getTerminal(j));
        first         = false;
        isEmptyFollow = false;
      }
    }
    if (isEmptyFollow) { printf("∅"); }
    printf("} |\n");
  }
  printf("+-------------------------------------------------------------------------+\n");

  printf("[DEBUG] First and Follow verification complete\n");
}

struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* parsedGrammar) {
  printf("[DEBUG] Starting first and follow computation\n");

  if (parsedGrammar == NULL) {
    fprintf(stderr, "Error: NULL grammar in computeFirstAndFollowSets\n");
    return NULL;
  }

  printf("[DEBUG] Creating FirstAndFollow structure\n");
  struct FirstAndFollow* firstAndFollowSets = (struct FirstAndFollow*)malloc(sizeof(struct FirstAndFollow));
  if (firstAndFollowSets == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for FirstAndFollow\n");
    return NULL;
  }

  printf("[DEBUG] Allocating FIRST array\n");
  firstAndFollowSets->FIRST = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
  if (firstAndFollowSets->FIRST == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for FIRST sets\n");
    free(firstAndFollowSets);
    return NULL;
  }

  printf("[DEBUG] Allocating FOLLOW array\n");
  firstAndFollowSets->FOLLOW = (int**)malloc(sizeof(int*) * NUM_NONTERMINALS);
  if (firstAndFollowSets->FOLLOW == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for FOLLOW sets\n");
    free(firstAndFollowSets->FIRST);
    free(firstAndFollowSets);
    return NULL;
  }

  printf("[DEBUG] Initializing arrays\n");
  int symbolVectorSize = NUM_TERMINALS;

  for (int i = 0; i < NUM_NONTERMINALS; i++) {
    firstAndFollowSets->FIRST[i] = (int*)calloc(symbolVectorSize, sizeof(int));
    if (firstAndFollowSets->FIRST[i] == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for FIRST set row %d\n", i);

      for (int j = 0; j < i; j++) { free(firstAndFollowSets->FIRST[j]); }
      free(firstAndFollowSets->FIRST);
      free(firstAndFollowSets->FOLLOW);
      free(firstAndFollowSets);
      return NULL;
    }

    firstAndFollowSets->FOLLOW[i] = (int*)calloc(symbolVectorSize, sizeof(int));
    if (firstAndFollowSets->FOLLOW[i] == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for FOLLOW set row %d\n", i);

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

  printf("[DEBUG] Initializing processing flags\n");
  for (int i = 0; i < NUM_NONTERMINALS; i++) { nonTerminalProcessed[i] = false; }

  printf("[DEBUG] Computing FIRST sets\n");
  for (int i = 0; i < NUM_NONTERMINALS; i++) {
    if (!nonTerminalProcessed[i]) {
      printf("[DEBUG] Computing FIRST for non-terminal %d: %s\n", i, getNonTerminal(i));
      if (nonTerminalRuleRecords[i] == NULL) {
        fprintf(stderr, "Error: No rule records for non-terminal %d: %s\n", i, getNonTerminal(i));
        continue;
      }
      computeFirstSets(i,firstAndFollowSets->FIRST);
    }
  }

  printf("[DEBUG] Computing FOLLOW sets\n");
  firstAndFollowSets->FOLLOW[program][TK_DOLLAR] = 1;

  fillFollowF(firstAndFollowSets->FOLLOW, firstAndFollowSets->FIRST, parsedGrammar);
  verifyFirstAndFollow(firstAndFollowSets);

  printf("[DEBUG] First and Follow computation complete\n");
  return firstAndFollowSets;
}