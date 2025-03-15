/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

#include "parse.h"
#include "constants.h"
#include "lexer.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct ParsingTable* createParsingTable() {
  struct ParsingTable* parseTable = (struct ParsingTable*)malloc(sizeof(struct ParsingTable));
  if (parseTable == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for parsing table\n");
    return NULL;
  }

  parseTable->entries = (int**)malloc(NUM_NONTERMINALS * sizeof(int*));
  if (parseTable->entries == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for parsing table entries\n");
    free(parseTable);
    return NULL;
  }

  for (int i = 0; i < NUM_NONTERMINALS; i++) {
    parseTable->entries[i] = (int*)calloc(NUM_TERMINALS, sizeof(int));
    if (parseTable->entries[i] == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for parsing table row %d\n", i);
      // Free previously allocated memory
      for (int j = 0; j < i; j++) { free(parseTable->entries[j]); }
      free(parseTable->entries);
      free(parseTable);
      return NULL;
    }
  }

  return parseTable;
}

void createParseTable(struct FirstAndFollow* firstAndFollowSets, struct ParsingTable* parseTable) {
  if (firstAndFollowSets == NULL || parseTable == NULL || parsedGrammar == NULL) {
    fprintf(stderr, "Error: NULL parameter in createParseTable\n");
    return;
  }

  // Iterate through all grammar rules
  for (int i = 1; i <= NUM_GRAMMAR_RULES; i++) {
    struct Rule* rule = parsedGrammar->GRAMMAR_RULES[i];
    if (rule == NULL || rule->symbols == NULL || rule->symbols->HEAD_SYMBOL == NULL) {
      fprintf(stderr, "Error: Invalid rule at index %d\n", i);
      continue;
    }

    int            lhsNonTerminal   = rule->symbols->HEAD_SYMBOL->symType.NON_TERMINAL;
    struct Symbol* rhsHead          = rule->symbols->HEAD_SYMBOL->next;
    struct Symbol* trav             = rhsHead;
    int            epsilonGenerated = 1;

    // Process RHS of the rule
    while (trav != NULL) {
      if (trav->isTerminal == 1) {
        if (trav->symType.TERMINAL != TK_EPS) {
          // If first symbol is terminal, add rule to table
          epsilonGenerated                                 = 0;
          parseTable->entries[lhsNonTerminal][trav->symType.TERMINAL] = rule->ruleNum;
          break;
        } else {
          // Epsilon rule
          epsilonGenerated = 1;
          break;
        }
      } else {
        // If first symbol is non-terminal, add rule for each terminal in FIRST set
        for (int j = 0; j < NUM_TERMINALS; j++) {
          if (firstAndFollowSets->FIRST[trav->symType.NON_TERMINAL][j] == 1) {
            parseTable->entries[lhsNonTerminal][j] = rule->ruleNum;
          }
        }

        // Check if this non-terminal can derive epsilon
        if (firstAndFollowSets->FIRST[trav->symType.NON_TERMINAL][TK_EPS] == 0) {
          epsilonGenerated = 0;
          break;
        }
      }
      trav = trav->next;
    }

    // If RHS can derive epsilon, add rule for each terminal in FOLLOW set
    if (epsilonGenerated) {
      for (int j = 0; j < NUM_TERMINALS; j++) {
        if (firstAndFollowSets->FOLLOW[lhsNonTerminal][j] == 1) { parseTable->entries[lhsNonTerminal][j] = rule->ruleNum; }
      }
    }
  }
}

int isSynchronizingToken(TokenName token) {
  return (token == TK_SEM || token == TK_ENDRECORD || token == TK_ENDUNION || token == TK_ENDIF ||
          token == TK_ENDWHILE || token == TK_ELSE || token == TK_END || token == TK_CL || token == TK_SQR);
}

Token* createTokenCopy(Token* original) {
  if (original == NULL) return NULL;

  Token* copy = (Token*)malloc(sizeof(Token));
  if (copy == NULL) return NULL;

  copy->tLexeme   = copyLexeme(original->tLexeme);
  copy->lineNum   = original->lineNum;
  copy->tokenName = original->tokenName;
  copy->isNum     = original->isNum;
  copy->VALUE     = original->VALUE;

  return copy;
}

Token* createSyntheticToken(int tokenType, int lineNo) {
  Token* synthetic = (Token*)malloc(sizeof(Token));
  if (synthetic == NULL) return NULL;

  synthetic->tLexeme   = copyLexeme("ERROR_SYNTHETIC");
  synthetic->lineNum   = lineNo;
  synthetic->tokenName = tokenType;
  synthetic->isNum     = 0;
  synthetic->VALUE     = NULL;

  return synthetic;
}

int isInFollow(struct FirstAndFollow* firstAndFollowSets, int nonTerminal, int token) {
  if (firstAndFollowSets == NULL || nonTerminal < 0 || nonTerminal >= NUM_NONTERMINALS || token < 0 ||
      token >= NUM_TERMINALS) {
    return 0;
  }

  return firstAndFollowSets->FOLLOW[nonTerminal][token] == 1;
}

struct ParseTree*
parseSourceCode(char* sourceFile, struct ParsingTable* pTable, struct FirstAndFollow* firstAndFollowSets) {
  if (sourceFile == NULL || pTable == NULL || firstAndFollowSets == NULL) {
    fprintf(stderr, "Error: NULL parameter in parseSourceCode\n");
    return NULL;
  }

  int fd = open(sourceFile, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: Unable to open source file %s\n", sourceFile);
    return NULL;
  }

  // Initialize components
  setupLexer(fd);
  struct ParseTree* parseTable = initializeParseTree();
  struct Stack*     st = initializeStack(parseTable);

  // Reset error tracking flags
  syntaxErrorOccurred  = false;
  lexicalErrorOccurred = false;

  // Get first token
  Token* inputToken = getToken();
  // Skip comments
  while (inputToken != NULL && inputToken->tokenName == TK_COMMENT) { inputToken = getToken(); }

  if (inputToken == NULL) {
    printf("Empty input file or only comments\n");
    close(fd);
    return parseTable;
  }

  // Main parsing loop
  while (1) {
    if (inputToken == NULL) { break; }

    // Skip comments
    if (inputToken->tokenName == TK_COMMENT) {
      inputToken = getToken();
      continue;
    }

    // Handle lexical errors
    if (inputToken->tokenName == TK_ERR) {
      lexicalErrorOccurred = true;
      // Get next token and continue
      inputToken = getToken();
      while (inputToken != NULL && inputToken->tokenName == TK_COMMENT) { inputToken = getToken(); }
      continue;
    }

    // Check stack
    struct NaryTreeNode* stackTop = top(st);
    if (stackTop == NULL) {
      printf("Error: Stack is empty\n");
      break;
    }

    // Case 1: Terminal on top of the stack
    if (stackTop->isLeaf == 1) {
      // Check for end of parsing
      if (stackTop->nodeType.L.enumID == TK_DOLLAR) {
        if (inputToken == NULL || inputToken->tokenName == TK_EOF) {
          // Successful parse
          break;
        }
        // Handle extra tokens
        syntaxErrorOccurred = true;
        // Just consume the remaining tokens
        while (inputToken != NULL && inputToken->tokenName != TK_EOF) {
          inputToken = getToken();
          // Skip comments
          while (inputToken != NULL && inputToken->tokenName == TK_COMMENT) { inputToken = getToken(); }
        }
        break;
      }

      // Terminal matches input token
      if (inputToken->tokenName == stackTop->nodeType.L.enumID) {
        // Store token in parse tree
        stackTop->nodeType.L.TOKEN = createTokenCopy(inputToken);

        // Consume token and pop stack
        pop(st);
        inputToken = getToken();
        while (inputToken != NULL && inputToken->tokenName == TK_COMMENT) { inputToken = getToken(); }
        continue;
      }
      // Terminal does not match input token - syntax error
      else {
        // Report the error
        fprintf(stderr,
                "Syntax error at line %d: Expected %s, found %s\n",
                inputToken->lineNum,
                getTerminal(stackTop->nodeType.L.enumID),
                getTerminal(inputToken->tokenName));

        syntaxErrorOccurred = true;

        // Create synthetic token for the parse tree
        stackTop->nodeType.L.TOKEN = createSyntheticToken(stackTop->nodeType.L.enumID, inputToken->lineNum);

        // Pop terminal without consuming input
        pop(st);
        continue;
      }
    }
    // Case 2: Non-terminal on top of the stack
    else {
      int nonTerminalID = stackTop->nodeType.NL.enumID;
      int tokenID       = inputToken->tokenName;

      // Bounds check before accessing parsing table
      if (nonTerminalID < 0 || nonTerminalID >= NUM_NONTERMINALS || tokenID < 0 || tokenID >= NUM_TERMINALS) {
        fprintf(stderr, "Error: Invalid indices for parsing table lookup - NT: %d, T: %d\n", nonTerminalID, tokenID);
        syntaxErrorOccurred = true;

        // Skip this token and continue
        inputToken = getToken();
        while (inputToken != NULL && inputToken->tokenName == TK_COMMENT) { inputToken = getToken(); }
        continue;
      }

      int ruleNumber = pTable->entries[nonTerminalID][tokenID];

      // If no rule is found, check for the special case of typeDefinitions with TK_READ.
      if (ruleNumber == 0) {
        if (strcmp(getNonTerminal(nonTerminalID), "typeDefinitions") == 0 && tokenID == TK_READ) {
          // Assume an empty production for typeDefinitions and pop it.
          pop(st);
          continue;
        }
      }

      // Rule exists in parsing table
      if (ruleNumber != 0) {
        // Verify rule exists
        if (ruleNumber < 1 || ruleNumber >= parsedGrammar->GRAMMAR_RULES_SIZE ||
            parsedGrammar->GRAMMAR_RULES[ruleNumber] == NULL) {
          fprintf(stderr,
                  "Error: Invalid rule number %d for NT: %s, T: %s\n",
                  ruleNumber,
                  getNonTerminal(nonTerminalID),
                  getTerminal(tokenID));
          syntaxErrorOccurred = true;

          // Pop this non-terminal and continue
          pop(st);
          continue;
        }

        struct Rule* rule = parsedGrammar->GRAMMAR_RULES[ruleNumber];
        addRuleToParseTree(stackTop, rule);

        pop(st);

        // Push children onto stack in reverse order (skip pushing epsilon)
        struct NaryTreeNode* childNode = stackTop->nodeType.NL.child;
        if (childNode != NULL && !(childNode->isLeaf == 1 && childNode->nodeType.L.enumID == TK_EPS)) {
          pushTreeChildren(st, childNode);
        }
      }
      // No rule in parsing table - syntax error
      else {
        fprintf(stderr,
                "Syntax error at line %d: Unexpected token %s for non-terminal %s\n",
                inputToken->lineNum,
                getTerminal(tokenID),
                getNonTerminal(nonTerminalID));

        syntaxErrorOccurred = true;

        // Error recovery strategy
        if (isInFollow(firstAndFollowSets, nonTerminalID, tokenID)) {
          // If token is in FOLLOW, create an empty derivation
          struct Symbol* epsilonSymbol = (struct Symbol*)malloc(sizeof(struct Symbol));
          if (epsilonSymbol != NULL) {
            epsilonSymbol->isTerminal   = 1;
            epsilonSymbol->symType.TERMINAL = TK_EPS;
            epsilonSymbol->next          = NULL;

            struct SymbolList* sl = initializeSymbolList();
            if (sl != NULL) {
              struct Symbol* ntSymbol = (struct Symbol*)malloc(sizeof(struct Symbol));
              if (ntSymbol != NULL) {
                ntSymbol->isTerminal       = 0;
                ntSymbol->symType.NON_TERMINAL = nonTerminalID;
                ntSymbol->next              = epsilonSymbol;

                addToSymbolList(sl, ntSymbol);
                addToSymbolList(sl, epsilonSymbol);

                struct Rule* syntheticRule = initializeRule(sl, 0);
                if (syntheticRule != NULL) {
                  // Add empty rule to parse tree
                  addRuleToParseTree(stackTop, syntheticRule);
                }
              }
            }
          }

          // Pop the non-terminal from stack
          pop(st);
        } else if (isSynchronizingToken(tokenID)) {
          // Synchronizing token found, resume parsing
          printf("Error recovery: Found synchronizing token %s\n", getTerminal(tokenID));
          pop(st);
        } else {
          // Skip current token and continue
          printf("Error recovery: Skipping token %s\n", getTerminal(tokenID));
          inputToken = getToken();
          while (inputToken != NULL && inputToken->tokenName == TK_COMMENT) { inputToken = getToken(); }
        }
      }
    }
  }

  // Report parsing status
  if (!lexicalErrorOccurred && !syntaxErrorOccurred) {
    printf("\nParsing successful\n");
  } else {
    printf("\nParsing unsuccessful\n");
    if (lexicalErrorOccurred) { printf("- Lexical errors detected\n"); }
    if (syntaxErrorOccurred) { printf("- Syntax errors detected\n"); }
  }

  close(fd);
  return parseTable;
}

void printParseTreeHelper(struct NaryTreeNode* parseTable, FILE* f) {
  if (parseTable == NULL) return;

  if (parseTable->isLeaf == 1) {
    // Process leaf node (terminal)
    int tokenEnumID = parseTable->nodeType.L.enumID;

    // Initialize lexeme with spaces
    char lexeme[30];
    memset(lexeme, ' ', 29);
    lexeme[29] = '\0';

    // Fill lexeme field
    if (tokenEnumID != TK_EPS) {
      if (parseTable->nodeType.L.TOKEN != NULL && parseTable->nodeType.L.TOKEN->tLexeme != NULL) {
        strncpy(lexeme, parseTable->nodeType.L.TOKEN->tLexeme, strlen(parseTable->nodeType.L.TOKEN->tLexeme));
      } else {
        strncpy(lexeme, "----", 4);
      }
    } else {
      strncpy(lexeme, "EPSILON", 7);
    }

    // Get token value and line number
    int   lineNumber   = -1;
    int   isNumber     = 0;
    int   valueIfInt   = 0;
    float valueIfFloat = 0.0;

    if (tokenEnumID != TK_EPS && parseTable->nodeType.L.TOKEN != NULL) {
      lineNumber = parseTable->nodeType.L.TOKEN->lineNum;
      isNumber   = parseTable->nodeType.L.TOKEN->isNum;
      if (isNumber == 1 && parseTable->nodeType.L.TOKEN->VALUE != NULL)
        valueIfInt = parseTable->nodeType.L.TOKEN->VALUE->INT_VALUE;
      else if (isNumber == 2 && parseTable->nodeType.L.TOKEN->VALUE != NULL)
        valueIfFloat = parseTable->nodeType.L.TOKEN->VALUE->FLOAT_VALUE;
    }

    // Initialize token name with spaces
    char tokenName[20];
    memset(tokenName, ' ', 19);
    tokenName[19] = '\0';

    // Fill token name field
    char* obtainedTokenName = getTerminal(parseTable->nodeType.L.enumID);
    if (obtainedTokenName != NULL) { strncpy(tokenName, obtainedTokenName, strlen(obtainedTokenName)); }

    // Initialize parent field with spaces
    char parent[30];
    memset(parent, ' ', 29);
    parent[29] = '\0';

    // Fill parent field
    if (parseTable->parent != NULL) {
      char* obtainedParent = getNonTerminal(parseTable->parent->nodeType.NL.enumID);
      if (obtainedParent != NULL) { strncpy(parent, obtainedParent, strlen(obtainedParent)); }
    } else {
      strncpy(parent, "ROOT", 4);
    }

    // Print based on format: lexeme lineno tokenName valueIfNumber parentNodeSymbol isLeafNode NodeSymbol
    if (lineNumber == -1) {
      // No valid line number
      if (tokenEnumID == TK_EPS || isNumber == 0)
        fprintf(
            f, "%-30s %-3s %-20s %-10s %-30s %-3s %-30s\n", lexeme, "---", tokenName, "----", parent, "yes", "----");
      else if (isNumber == 1)
        fprintf(f,
                "%-30s %-3s %-20s %-10d %-30s %-3s %-30s\n",
                lexeme,
                "---",
                tokenName,
                valueIfInt,
                parent,
                "yes",
                "----");
      else
        fprintf(f,
                "%-30s %-3s %-20s %-10f %-30s %-3s %-30s\n",
                lexeme,
                "---",
                tokenName,
                valueIfFloat,
                parent,
                "yes",
                "----");
    } else {
      // Valid line number
      if (tokenEnumID == TK_EPS || isNumber == 0)
        fprintf(f,
                "%-30s %-3d %-20s %-10s %-30s %-3s %-30s\n",
                lexeme,
                lineNumber,
                tokenName,
                "----",
                parent,
                "yes",
                "----");
      else if (isNumber == 1)
        fprintf(f,
                "%-30s %-3d %-20s %-10d %-30s %-3s %-30s\n",
                lexeme,
                lineNumber,
                tokenName,
                valueIfInt,
                parent,
                "yes",
                "----");
      else
        fprintf(f,
                "%-30s %-3d %-20s %-10f %-30s %-3s %-30s\n",
                lexeme,
                lineNumber,
                tokenName,
                valueIfFloat,
                parent,
                "yes",
                "----");
    }
  } else {
    // Process non-leaf node (non-terminal)
    // Try to derive line number from children
    int derivedLineNumber = -1;

    // Find first terminal child with a valid line number
    struct NaryTreeNode* lineTrav = parseTable->nodeType.NL.child;
    while (lineTrav != NULL && derivedLineNumber == -1) {
      if (lineTrav->isLeaf == 1 && lineTrav->nodeType.L.TOKEN != NULL &&
          lineTrav->nodeType.L.TOKEN->lineNum > 0) {
        derivedLineNumber = lineTrav->nodeType.L.TOKEN->lineNum;
        break;
      } else if (lineTrav->isLeaf == 0) {
        // Try to find in non-leaf child's children recursively
        struct NaryTreeNode* childTrav = lineTrav->nodeType.NL.child;
        while (childTrav != NULL && derivedLineNumber == -1) {
          if (childTrav->isLeaf == 1 && childTrav->nodeType.L.TOKEN != NULL &&
              childTrav->nodeType.L.TOKEN->lineNum > 0) {
            derivedLineNumber = childTrav->nodeType.L.TOKEN->lineNum;
            break;
          }
          childTrav = childTrav->next;
        }
      }
      lineTrav = lineTrav->next;
    }

    struct NaryTreeNode* trav = parseTable->nodeType.NL.child;

    // Process children first (pre-order traversal)
    if (trav != NULL) {
      printParseTreeHelper(parseTable->nodeType.NL.child, f);
      trav = trav->next;
    }

    // Initialize lexeme with dashes
    char lexeme[30];
    memset(lexeme, ' ', 29);
    lexeme[29] = '\0';
    strncpy(lexeme, "----", 4);

    // Initialize parent field with spaces
    char parent[30];
    memset(parent, ' ', 29);
    parent[29] = '\0';

    // Fill parent field if not root
    if (parseTable->parent != NULL) {
      char* obtainedParent = getNonTerminal(parseTable->parent->nodeType.NL.enumID);
      if (obtainedParent != NULL) { strncpy(parent, obtainedParent, strlen(obtainedParent)); }
    } else {
      strncpy(parent, "ROOT", 4);
    }

    // Get current node symbol (non-terminal name)
    char* nodeSymbol = getNonTerminal(parseTable->nodeType.NL.enumID);

    // format: lexeme lineno tokenName valueIfNumber parentNodeSymbol isLeafNode NodeSymbol
    if (derivedLineNumber == -1) {
      fprintf(f, "%-30s %-3s %-20s %-10s %-30s %-3s %-30s\n", lexeme, "---", "----", "----", parent, "no", nodeSymbol);
    } else {
      fprintf(f,
              "%-30s %-3d %-20s %-10s %-30s %-3s %-30s\n",
              lexeme,
              derivedLineNumber,
              "----",
              "----",
              parent,
              "no",
              nodeSymbol);
    }

    // Process remaining siblings
    while (trav != NULL) {
      printParseTreeHelper(trav, f);
      trav = trav->next;
    }
  }
}

void writeParseTreeToFile(struct ParseTree* parseTable, char* outputFile) {
  if (parseTable == NULL) {
    fprintf(stderr, "Error: NULL parse tree in writeParseTreeToFile\n");
    return;
  }

  FILE* f;
  if (outputFile == NULL)
    f = stdout;
  else
    f = fopen(outputFile, "wb");

  if (f == NULL) {
    fprintf(stderr, "Error opening the outfile %s\n", outputFile);
    return;
  }

  printParseTreeHelper(parseTable->root, f);

  if (f != stdout) fclose(f);
}