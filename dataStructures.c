/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

#include "dataStructures.h"
#include "grammar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* copyLexeme(char* str) {
    int   len = strlen(str);
    char* lex = (char*)malloc(sizeof(char) * (len + 1));
    for (int i = 0; i < len; i++) lex[i] = str[i];
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

    int   len       = strlen(str);
    char* strConcat = (char*)malloc(sizeof(char) * (len + 2));
    if (strConcat == NULL) {
        printf("Error: Memory allocation failed in appendToSymbol\n");
        return str; // Return original string if allocation fails
    }

    strcpy(strConcat, str);
    strConcat[len]     = c;
    strConcat[len + 1] = '\0';

    free(str); // Free the original string to prevent memory leak
    return strConcat;
}

struct Symbol* initializeSymbol(char* symbol) {
    if (symbol == NULL) {
        printf("Error: NULL symbol passed to initializeSymbol\n");
        return NULL;
    }

    struct Symbol* s = (struct Symbol*)malloc(sizeof(struct Symbol));
    if (s == NULL) {
        printf("Error: Memory allocation failed in initializeSymbol\n");
        return NULL;
    }

    int idNonTerminal, idTerminal;

    // First check if it's a terminal (TK_* tokens)
    if (strncmp(symbol, "TK_", 3) == 0) {
        idTerminal = findInTerminalMap(symbol);
        if (idTerminal != -1) {
            s->TYPE.TERMINAL = idTerminal;
            s->IS_TERMINAL   = 1;
        } else {
            printf("Error: Unknown terminal symbol: %s\n", symbol);
            free(s); // Clean up allocated memory
            return NULL;
        }
    } else {
        // Check if it's a non-terminal
        idNonTerminal = findInNonTerminalMap(symbol);
        if (idNonTerminal != -1) {
            s->TYPE.NON_TERMINAL = idNonTerminal;
            s->IS_TERMINAL       = 0;
        } else {
            printf("Error: Unknown symbol: %s\n", symbol);
            free(s); // Clean up allocated memory
            return NULL;
        }
    }

    s->next = NULL;
    return s;
}

struct SymbolList* initializeSymbolList() {
    struct SymbolList* sl = (struct SymbolList*)malloc(sizeof(struct SymbolList));
    sl->HEAD_SYMBOL       = NULL;
    sl->TAIL_SYMBOL       = NULL;
    sl->RULE_LENGTH       = 0;
    return sl;
}

void addToSymbolList(struct SymbolList* ls, struct Symbol* s) {
    struct Symbol* h = ls->HEAD_SYMBOL;
    if (h == NULL) {
        ls->HEAD_SYMBOL = s;
        ls->TAIL_SYMBOL = s;
        ls->RULE_LENGTH = 1;
        return;
    }
    ls->TAIL_SYMBOL->next = s;
    ls->TAIL_SYMBOL       = s;
    ls->RULE_LENGTH += 1;
}

struct NaryTreeNode* createLeafNode(int enumId) {
    struct NaryTreeNode* ntn = (struct NaryTreeNode*)malloc(sizeof(struct NaryTreeNode));
    ntn->IS_LEAF_NODE        = 1;
    ntn->NODE_TYPE.L.ENUM_ID = enumId;
    ntn->next                = NULL;
    return ntn;
}

struct NaryTreeNode* createNonLeafNode(int enumId) {
    struct NaryTreeNode* ntn          = (struct NaryTreeNode*)malloc(sizeof(struct NaryTreeNode));
    ntn->IS_LEAF_NODE                 = 0;
    ntn->NODE_TYPE.NL.ENUM_ID         = enumId;
    ntn->NODE_TYPE.NL.NUMBER_CHILDREN = 0;
    ntn->next                         = NULL;
    ntn->NODE_TYPE.NL.child           = NULL;
    return ntn;
}

struct NaryTreeNode* createNode(int isTerminal, union SymbolType type, struct NaryTreeNode* parent) {
    struct NaryTreeNode* ntn;
    if (isTerminal == 1) {
        ntn         = createLeafNode(type.TERMINAL);
        ntn->parent = parent;
    } else {
        ntn         = createNonLeafNode(type.NON_TERMINAL);
        ntn->parent = parent;
    }
    return ntn;
}

struct ParseTree* initializeParseTree() {
    struct ParseTree* pt = (struct ParseTree*)malloc(sizeof(struct ParseTree));
    pt->root             = createNonLeafNode(program);
    pt->root->parent     = NULL;
    return pt;
}

void addRuleToParseTree(struct NaryTreeNode* ntn, struct Rule* r) {
    if (ntn == NULL || r == NULL) {
        printf("Error: NULL pointer passed to addRuleToParseTree\n");
        return;
    }

    if (ntn->IS_LEAF_NODE == 1) {
        printf("TERMINALS CANNOT HAVE CHILDREN! \n");
        return;
    }

    int                  numberChild = 0;
    struct Symbol*       trav        = r->SYMBOLS->HEAD_SYMBOL->next;
    struct NaryTreeNode* childHead   = NULL;
    struct NaryTreeNode* childTrav   = NULL;

    while (trav != NULL) {
        struct NaryTreeNode* newNode = createNode(trav->IS_TERMINAL, trav->TYPE, ntn);
        if (newNode == NULL) {
            printf("Error: Failed to create tree node\n");
            return;
        }

        if (childHead == NULL) {
            childHead = newNode;
            childTrav = childHead;
        } else {
            childTrav->next = newNode;
            childTrav       = childTrav->next;
        }
        numberChild++;
        trav = trav->next;
    }

    ntn->NODE_TYPE.NL.RULE_NO         = r->RULE_NO;
    ntn->NODE_TYPE.NL.child           = childHead;
    ntn->NODE_TYPE.NL.NUMBER_CHILDREN = numberChild;
}

void printNaryTree(struct NaryTreeNode* nt) {
    if (nt->IS_LEAF_NODE == 1) {
        printf("%s ", getTerminal(nt->NODE_TYPE.L.ENUM_ID));
        return;
    }

    printf("%s\n", getNonTerminal(nt->NODE_TYPE.NL.ENUM_ID));

    struct NaryTreeNode* childTrav = nt->NODE_TYPE.NL.child;
    while (childTrav != NULL) {
        if (childTrav->IS_LEAF_NODE == 1)
            printf("%s ", getTerminal(childTrav->NODE_TYPE.L.ENUM_ID));
        else
            printf("%s ", getNonTerminal(childTrav->NODE_TYPE.NL.ENUM_ID));

        childTrav = childTrav->next;
    }

    printf("\n");

    childTrav = nt->NODE_TYPE.NL.child;
    while (childTrav != NULL) {
        if (childTrav->IS_LEAF_NODE == 0) printNaryTree(childTrav);
        childTrav = childTrav->next;
    }
}

void printTree(struct ParseTree* pt) {
    struct NaryTreeNode* nt = pt->root;
    printNaryTree(nt);
}

int getParseTreeNodeCount() {
    return 0;
}

int getParseTreeMemory() {
    return 0;
}

struct StackNode* createStackNode(struct NaryTreeNode* ntn) {
    struct StackNode* stn = (struct StackNode*)malloc(sizeof(struct StackNode));
    stn->TREE_NODE        = ntn;
    stn->next             = NULL;
    return stn;
}

struct NaryTreeNode* top(struct Stack* st) {
    if (st->HEAD == NULL)
        return NULL;
    else
        return st->HEAD->TREE_NODE;
}

void push(struct Stack* st, struct NaryTreeNode* ntn) {
    struct StackNode* stn  = createStackNode(ntn);
    struct StackNode* head = st->HEAD;

    if (head == NULL) {
        st->HEAD = stn;
        st->NUM_NODES++;
        return;
    }

    stn->next = head;
    st->HEAD  = stn;
    st->NUM_NODES++;
    return;
}

void pop(struct Stack* st) {
    struct StackNode* head = st->HEAD;

    if (head == NULL) return;

    st->HEAD = st->HEAD->next;
    st->NUM_NODES--;
}

void pushTreeChildren(struct Stack* st, struct NaryTreeNode* ntn) {
    if (ntn == NULL) return;
    pushTreeChildren(st, ntn->next);
    push(st, ntn);
}

struct Stack* initializeStack(struct ParseTree* pt) {
    struct Stack* st = (struct Stack*)malloc(sizeof(struct Stack));
    st->HEAD         = NULL;
    st->NUM_NODES    = 0;

    union SymbolType sType;
    sType.TERMINAL           = TK_DOLLAR;
    struct NaryTreeNode* ntn = createNode(1, sType, NULL);
    push(st, ntn);
    push(st, pt->root);
    return st;
}