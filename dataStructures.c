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
      s->symType.TERMINAL = idTerminal;
      s->isTerminal   = 1;
    } else {
      printf("Error: Unknown terminal symbol: %s\n", symbol);
      free(s); // Clean up allocated memory
      return NULL;
    }
  } else {
    // Check if it's a non-terminal
    idNonTerminal = findInNonTerminalMap(symbol);
    if (idNonTerminal != -1) {
      s->symType.NON_TERMINAL = idNonTerminal;
      s->isTerminal       = 0;
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

void addToSymbolList(struct SymbolList* symList, struct Symbol* s) {
  struct Symbol* h = symList->HEAD_SYMBOL;
  if (h == NULL) {
    symList->HEAD_SYMBOL = s;
    symList->TAIL_SYMBOL = s;
    symList->RULE_LENGTH = 1;
    return;
  }
  symList->TAIL_SYMBOL->next = s;
  symList->TAIL_SYMBOL       = s;
  symList->RULE_LENGTH += 1;
}

struct NaryTreeNode* createLeafNode(int enumId) {
  struct NaryTreeNode* ntn = (struct NaryTreeNode*)malloc(sizeof(struct NaryTreeNode));
  ntn->isLeaf        = 1;
  ntn->nodeType.L.enumID = enumId;
  ntn->next                = NULL;
  return ntn;
}

struct NaryTreeNode* createNonLeafNode(int enumId) {
  struct NaryTreeNode* ntn          = (struct NaryTreeNode*)malloc(sizeof(struct NaryTreeNode));
  ntn->isLeaf                 = 0;
  ntn->nodeType.NL.enumID         = enumId;
  ntn->nodeType.NL.numChildren = 0;
  ntn->next                         = NULL;
  ntn->nodeType.NL.child           = NULL;
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
  struct ParseTree* parseTable = (struct ParseTree*)malloc(sizeof(struct ParseTree));
  parseTable->root             = createNonLeafNode(program);
  parseTable->root->parent     = NULL;
  return parseTable;
}

void addRuleToParseTree(struct NaryTreeNode* ntn, struct Rule* rule) {
  if (ntn == NULL || rule == NULL) {
    printf("Error: NULL pointer passed to addRuleToParseTree\n");
    return;
  }

  if (ntn->isLeaf == 1) {
    printf("TERMINALS CANNOT HAVE CHILDREN! \n");
    return;
  }

  int                  numberChild = 0;
  struct Symbol*       trav        = rule->symbols->HEAD_SYMBOL->next;
  struct NaryTreeNode* childHead   = NULL;
  struct NaryTreeNode* childTrav   = NULL;

  while (trav != NULL) {
    struct NaryTreeNode* newNode = createNode(trav->isTerminal, trav->symType, ntn);
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

  ntn->nodeType.NL.ruleNum         = rule->ruleNum;
  ntn->nodeType.NL.child           = childHead;
  ntn->nodeType.NL.numChildren = numberChild;
}


struct StackNode* createStackNode(struct NaryTreeNode* ntn) {
  struct StackNode* stn = (struct StackNode*)malloc(sizeof(struct StackNode));
  stn->treeNode        = ntn;
  stn->next             = NULL;
  return stn;
}

struct NaryTreeNode* top(struct Stack* st) {
  if (st->HEAD == NULL)
    return NULL;
  else
    return st->HEAD->treeNode;
}

void push(struct Stack* st, struct NaryTreeNode* ntn) {
  struct StackNode* stn  = createStackNode(ntn);
  struct StackNode* head = st->HEAD;

  if (head == NULL) {
    st->HEAD = stn;
    st->numNodes++;
    return;
  }

  stn->next = head;
  st->HEAD  = stn;
  st->numNodes++;
  return;
}

void pop(struct Stack* st) {
  struct StackNode* head = st->HEAD;

  if (head == NULL) return;

  st->HEAD = st->HEAD->next;
  st->numNodes--;
}

void pushTreeChildren(struct Stack* st, struct NaryTreeNode* ntn) {
  if (ntn == NULL) return;
  pushTreeChildren(st, ntn->next);
  push(st, ntn);
}

struct Stack* initializeStack(struct ParseTree* parseTable) {
  struct Stack* st = (struct Stack*)malloc(sizeof(struct Stack));
  st->HEAD         = NULL;
  st->numNodes    = 0;

  union SymbolType sType;
  sType.TERMINAL           = TK_DOLLAR;
  struct NaryTreeNode* ntn = createNode(1, sType, NULL);
  push(st, ntn);
  push(st, parseTable->root);
  return st;
}