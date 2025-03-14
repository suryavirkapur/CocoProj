#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse.h"
#include "lexer.h"
#include "constants.h"
#include <stdbool.h>

struct ParsingTable* initializeParsingTable() {
    struct ParsingTable* pt = (struct ParsingTable*)malloc(sizeof(struct ParsingTable));
    if (pt == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for parsing table\n");
        return NULL;
    }
    
    pt->entries = (int**)malloc(NUM_NONTERMINALS * sizeof(int*));
    if (pt->entries == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for parsing table entries\n");
        free(pt);
        return NULL;
    }
    
    for (int i = 0; i < NUM_NONTERMINALS; i++) {
        pt->entries[i] = (int*)calloc(NUM_TERMINALS, sizeof(int));
        if (pt->entries[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for parsing table row %d\n", i);
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(pt->entries[j]);
            }
            free(pt->entries);
            free(pt);
            return NULL;
        }
    }
    
    return pt;
}

void createParseTable(struct FirstAndFollow* firstAndFollowSets, struct ParsingTable* pt) {
    if (firstAndFollowSets == NULL || pt == NULL || parsedGrammar == NULL) {
        fprintf(stderr, "Error: NULL parameter in createParseTable\n");
        return;
    }
    
    // Iterate through all grammar rules
    for (int i = 1; i <= NUM_GRAMMAR_RULES; i++) {
        struct Rule* r = parsedGrammar->GRAMMAR_RULES[i];
        if (r == NULL || r->SYMBOLS == NULL || r->SYMBOLS->HEAD_SYMBOL == NULL) {
            fprintf(stderr, "Error: Invalid rule at index %d\n", i);
            continue;
        }
        
        int lhsNonTerminal = r->SYMBOLS->HEAD_SYMBOL->TYPE.NON_TERMINAL;
        struct Symbol* rhsHead = r->SYMBOLS->HEAD_SYMBOL->next;
        struct Symbol* trav = rhsHead;
        int epsilonGenerated = 1;
        
        // Process RHS of the rule
        while (trav != NULL) {
            if (trav->IS_TERMINAL == 1) {
                if (trav->TYPE.TERMINAL != TK_EPS) {
                    // If first symbol is terminal, add rule to table
                    epsilonGenerated = 0;
                    pt->entries[lhsNonTerminal][trav->TYPE.TERMINAL] = r->RULE_NO;
                    break;
                } else {
                    // Epsilon rule
                    epsilonGenerated = 1;
                    break;
                }
            } else {
                // If first symbol is non-terminal, add rule for each terminal in FIRST set
                for (int j = 0; j < NUM_TERMINALS; j++) {
                    if (firstAndFollowSets->FIRST[trav->TYPE.NON_TERMINAL][j] == 1) {
                        pt->entries[lhsNonTerminal][j] = r->RULE_NO;
                    }
                }
                
                // Check if this non-terminal can derive epsilon
                if (firstAndFollowSets->FIRST[trav->TYPE.NON_TERMINAL][TK_EPS] == 0) {
                    epsilonGenerated = 0;
                    break;
                }
            }
            trav = trav->next;
        }
        
        // If RHS can derive epsilon, add rule for each terminal in FOLLOW set
        if (epsilonGenerated) {
            for (int j = 0; j < NUM_TERMINALS; j++) {
                if (firstAndFollowSets->FOLLOW[lhsNonTerminal][j] == 1) {
                    pt->entries[lhsNonTerminal][j] = r->RULE_NO;
                }
            }
        }
    }
}

int isSynchronizingToken(TokenName token) {
    return (token == TK_SEM || token == TK_ENDRECORD || token == TK_ENDUNION ||
            token == TK_ENDIF || token == TK_ENDWHILE || token == TK_ELSE ||
            token == TK_END || token == TK_CL || token == TK_SQR);
}

Token* skipComments(Token* token) {
    while (token != NULL && token->TOKEN_NAME == TK_COMMENT) {
        token = getToken();
    }
    return token;
}

// Helper function to create a copy of a token
Token* createTokenCopy(Token* original) {
    if (original == NULL) return NULL;
    
    Token* copy = (Token*)malloc(sizeof(Token));
    if (copy == NULL) return NULL;
    
    copy->LEXEME = copyLexeme(original->LEXEME);
    copy->LINE_NO = original->LINE_NO;
    copy->TOKEN_NAME = original->TOKEN_NAME;
    copy->IS_NUMBER = original->IS_NUMBER;
    copy->VALUE = original->VALUE;
    
    return copy;
}

// Helper function to create a synthetic token for error recovery
Token* createSyntheticToken(int tokenType, int lineNo) {
    Token* synthetic = (Token*)malloc(sizeof(Token));
    if (synthetic == NULL) return NULL;
    
    synthetic->LEXEME = "ERROR_SYNTHETIC";
    synthetic->LINE_NO = lineNo;
    synthetic->TOKEN_NAME = tokenType;
    synthetic->IS_NUMBER = 0;
    synthetic->VALUE = NULL;
    
    return synthetic;
}

// Simplified error reporting functions
void reportLexicalError(Token* token) {
    fprintf(stderr, "Lexical error at line %d: Invalid token '%s'\n", 
            token->LINE_NO, token->LEXEME);
}

void reportSyntaxError(Token* found, struct NaryTreeNode* expected) {
    fprintf(stderr, "Syntax error at line %d: Expected %s, found %s\n", 
            found->LINE_NO, getToken(expected->NODE_TYPE.L.ENUM_ID), 
            getToken(found->TOKEN_NAME));
}

void reportNonTerminalError(Token* token, int nonTerminal) {
    fprintf(stderr, "Syntax error at line %d: Unexpected token %s for non-terminal %s\n", 
            token->LINE_NO, getToken(token->TOKEN_NAME), 
            getNonTerminal(nonTerminal));
}

int isInFollow(struct FirstAndFollow* firstAndFollowSets, int nonTerminal, int token) {
    // Check if token is in the FOLLOW set of the non-terminal
    if (firstAndFollowSets != NULL && firstAndFollowSets->FOLLOW != NULL) {
        // Assuming FOLLOW is a 2D array where:
        // - First dimension is the non-terminal ID
        // - Second dimension contains the tokens in the FOLLOW set
        //   with a sentinel value (like 0 or -1) marking the end
        int i = 0;
        while (firstAndFollowSets->FOLLOW[nonTerminal][i] != 0 && firstAndFollowSets->FOLLOW[nonTerminal][i] != -1) {
            if (firstAndFollowSets->FOLLOW[nonTerminal][i] == token) {
                return 1;
            }
            i++;
        }
    }
    return 0;
}

struct ParseTree* parseInputSourceCode(char* sourceFile, struct ParsingTable* pTable, struct FirstAndFollow* firstAndFollowSets) {
    if (sourceFile == NULL || pTable == NULL || firstAndFollowSets == NULL) {
        fprintf(stderr, "Error: NULL parameter in parseInputSourceCode\n");
        return NULL;
    }
    
    int fd = open(sourceFile, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: Unable to open source file %s\n", sourceFile);
        return NULL;
    }
    
    // Initialize components
    initializeLexer(fd);
    struct ParseTree* pt = initializeParseTree();
    struct Stack* st = initializeStack(pt);
    
    // Error tracking
    bool hasLexicalError = false;
    bool hasSyntaxError = false;
    
    // Get first token, skipping comments
    Token* inputToken = getToken();
    while (inputToken != NULL && inputToken->TOKEN_NAME == TK_COMMENT) {
        inputToken = getToken();
    }
    
    if (inputToken == NULL) {
        printf("Empty input file or only comments\n");
        close(fd);
        return pt;
    }
    
    // Main parsing loop
    while (1) {
        if (inputToken == NULL) {
            break;
        }
        
        // Skip comments
        if (inputToken->TOKEN_NAME == TK_COMMENT) {
            inputToken = getToken();
            continue;
        }
        
        // Handle lexical errors
        if (inputToken->TOKEN_NAME == TK_ERR) {
            reportLexicalError(inputToken);
            hasLexicalError = true;
            inputToken = getToken();
            continue;
        }
        
        // Check stack
        struct NaryTreeNode* stackTop = top(st);
        if (stackTop == NULL) {
            printf("Error: Stack is empty\n");
            break;
        }
        
        // Case 1: Terminal on top of the stack
        if (stackTop->IS_LEAF_NODE == 1) {
            // End of parsing
            if (stackTop->NODE_TYPE.L.ENUM_ID == TK_DOLLAR) {
                if (inputToken == NULL || inputToken->TOKEN_NAME == TK_EOF) {
                    break;  // Successful parse
                }
                // Handle extra tokens if needed
                inputToken = getToken();
                continue;
            }
            
            // Terminal matches input token
            if (inputToken->TOKEN_NAME == stackTop->NODE_TYPE.L.ENUM_ID) {
                // Store token in parse tree
                stackTop->NODE_TYPE.L.TOKEN = createTokenCopy(inputToken);
                if (stackTop->NODE_TYPE.L.TOKEN == NULL) {
                    fprintf(stderr, "Error: Memory allocation failed for token\n");
                    close(fd);
                    return pt;
                }
                
                // Consume token and pop stack
                pop(st);
                inputToken = getToken();
                while (inputToken != NULL && inputToken->TOKEN_NAME == TK_COMMENT) {
                    inputToken = getToken();
                }
                continue;
            } 
            // Terminal does not match input token - syntax error
            else {
                reportSyntaxError(inputToken, stackTop);
                hasSyntaxError = true;
                
                // Create synthetic token for error recovery
                stackTop->NODE_TYPE.L.TOKEN = createSyntheticToken(stackTop->NODE_TYPE.L.ENUM_ID, inputToken->LINE_NO);
                
                // Recovery: Pop terminal without consuming input
                pop(st);
                continue;
            }
        }
        // Case 2: Non-terminal on top of the stack
        else {
            int ruleNumber = pTable->entries[stackTop->NODE_TYPE.NL.ENUM_ID][inputToken->TOKEN_NAME];
            
            // Rule exists in parsing table
            if (ruleNumber != 0) {
                struct Rule* r = parsedGrammar->GRAMMAR_RULES[ruleNumber];
                addRuleToParseTree(stackTop, r);
                
                pop(st);
                
                // Push children onto stack in reverse order (skip pushing epsilon)
                struct NaryTreeNode* childNode = stackTop->NODE_TYPE.NL.child;
                if (!(childNode->IS_LEAF_NODE == 1 && childNode->NODE_TYPE.L.ENUM_ID == TK_EPS)) {
                    pushTreeChildren(st, childNode);
                }
            }
            // No rule in parsing table - syntax error
            else {
                reportNonTerminalError(inputToken, stackTop->NODE_TYPE.NL.ENUM_ID);
                hasSyntaxError = true;
                
                // General error recovery strategy: panic mode
                // Skip tokens until we find one that can be derived from this non-terminal
                // or pop the non-terminal if we can't find such a token
                if (isInFollow(firstAndFollowSets, stackTop->NODE_TYPE.NL.ENUM_ID, inputToken->TOKEN_NAME)) {
                    // If token is in FOLLOW, pop the non-terminal (empty derivation)
                    pop(st);
                } else {
                    // Skip the current token and try again
                    inputToken = getToken();
                    while (inputToken != NULL && inputToken->TOKEN_NAME == TK_COMMENT) {
                        inputToken = getToken();
                    }
                }
            }
        }
    }
    
    // Report parsing status
    if (!hasLexicalError && !hasSyntaxError) {
        printf("\nSuccessfully parsed the input\n");
    } else {
        printf("\nParsing unsuccessful\n");
    }
    
    close(fd);
    return pt;
}


void printParseTreeHelper(struct NaryTreeNode* pt, FILE* f) {
    if (pt == NULL)
        return;
        
    if (pt->IS_LEAF_NODE == 1) {
        // process leaf node 
        // should be a terminal
        int tokenEnumID = pt->NODE_TYPE.L.ENUM_ID;
        
        // initialize lexeme with spaces
        char lexeme[30];
        memset(lexeme, ' ', 29);
        lexeme[29] = '\0';
        
        // Fill lexeme field
        if (tokenEnumID != TK_EPS) {
            strncpy(lexeme, pt->NODE_TYPE.L.TOKEN->LEXEME, strlen(pt->NODE_TYPE.L.TOKEN->LEXEME));
        } else {
            strncpy(lexeme, "EPSILON", 7);
        }
        
        // Get token value and line number
        int lineNumber = -1;
        int isNumber = 0;
        int valueIfInt = 0;
        float valueIfFloat = 0.0;
        
        if (tokenEnumID != TK_EPS) {
            lineNumber = pt->NODE_TYPE.L.TOKEN->LINE_NO;
            isNumber = pt->NODE_TYPE.L.TOKEN->IS_NUMBER;
            if (isNumber == 1)
                valueIfInt = pt->NODE_TYPE.L.TOKEN->VALUE->INT_VALUE;
            else if (isNumber == 2)
                valueIfFloat = pt->NODE_TYPE.L.TOKEN->VALUE->FLOAT_VALUE;
        }
        
        // Initialize token name with spaces
        char tokenName[20];
        memset(tokenName, ' ', 19);
        tokenName[19] = '\0';
        
        // Fill token name field
        char* obtainedTokenName = getTerminal(pt->NODE_TYPE.L.ENUM_ID);
        strncpy(tokenName, obtainedTokenName, strlen(obtainedTokenName));
        
        // Initialize parent field with spaces
        char parent[30];
        memset(parent, ' ', 29);
        parent[29] = '\0';
        
        // Fill parent field
        char* obtainedParent = getNonTerminal(pt->parent->NODE_TYPE.NL.ENUM_ID);
        strncpy(parent, obtainedParent, strlen(obtainedParent));
        
        // Print based on token type
        if (tokenEnumID == TK_EPS || isNumber == 0)
            fprintf(f, "%s      %d    %s %s %s %s %s\n", 
                    lexeme, lineNumber, tokenName, "----   ", parent, "yes", "----");
        else if (isNumber == 1)
            fprintf(f, "%s      %d    %s %d          %s %s %s\n", 
                    lexeme, lineNumber, tokenName, valueIfInt, parent, "yes", "----");
        else
            fprintf(f, "%s      %d    %s %f    %s %s %s\n", 
                    lexeme, lineNumber, tokenName, valueIfFloat, parent, "yes", "----");
    }
    else {
        // Process non-leaf node (non-terminal)
        struct NaryTreeNode* trav = pt->NODE_TYPE.NL.child;
        
        // Process children first (pre-order traversal)
        if (trav != NULL) {
            printParseTreeHelper(pt->NODE_TYPE.NL.child, f);
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
        if (pt->parent != NULL) {
            char* obtainedParent = getNonTerminal(pt->parent->NODE_TYPE.NL.ENUM_ID);
            strncpy(parent, obtainedParent, strlen(obtainedParent));
        } else {
            strncpy(parent, "NULL", 4);
        }
        
        // Print non-terminal node
        fprintf(f, "%s      %d    %s %s %s %s %s\n", 
                lexeme, -1, "-----------        ", "----   ", 
                parent, "no", getNonTerminal(pt->NODE_TYPE.NL.ENUM_ID));
        
        // Process remaining siblings
        while (trav != NULL) {
            printParseTreeHelper(trav, f);
            trav = trav->next;
        }
    }
}

void writeParseTreeToFile(struct ParseTree* pt, char* outputFile) {
    if (pt == NULL) {
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
    
    printParseTreeHelper(pt->root, f);
    
    if (f != stdout)
        fclose(f);
}

int getErrorStatus() {
    return (lexicalErrorOccurred || syntaxErrorOccurred);
}