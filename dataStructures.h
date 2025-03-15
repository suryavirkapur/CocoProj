#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "lexerDef.h"

// non-terminal symbol enum
// make sure you keep this in sync with
// non terminal ids in grammar.h file
// and in lexerDef.h
enum NonTerminalEnum {
  program,
  mainFunction,
  otherFunctions,
  function,
  input_par,
  output_par,
  parameterList,
  dataType,
  primitiveDatatype,
  constructedDatatype,
  moreParameters,
  statementSequence,
  typeDefinitions,
  actualOrRedefined,
  typeDefinition,
  fieldDefinitions,
  fieldDefinition,
  fieldDataType,
  remainingFields,
  declarations,
  declaration,
  globalSpecifier,
  otherStmts,
  statement,
  assignmentStmt,
  singleOrRecId,
  constructedVariable,
  oneExpansion,
  moreExpansions,
  optionalFieldAccess,
  funCallStmt,
  outputParameters,
  inputParameters,
  iterativeStmt,
  conditionalStmt,
  elsePart,
  ioStmt,
  arithmeticExpression,
  expPrime,
  term,
  termPrime,
  factor,
  highPrecedenceOperators,
  lowPrecedenceOperators,
  booleanExpression,
  variable,
  logicalOp,
  relationalOp,
  returnStmt,
  optionalReturn,
  idList,
  remainingIdentifiers,
  typeAliasStatement,
  recordOrUnion
};

typedef TokenName Terminal;

union SymbolType {
  Terminal         TERMINAL;
  enum NonTerminalEnum NON_TERMINAL;
};

struct Symbol {
  union SymbolType TYPE;
  int              IS_TERMINAL;
  struct Symbol*   next;
};

struct SymbolList {
  struct Symbol* HEAD_SYMBOL;
  struct Symbol* TAIL_SYMBOL;
  int            RULE_LENGTH;
};

struct NonTerminalRuleRecords {
  int start;
  int end;
};

struct Rule {
  struct SymbolList* SYMBOLS;
  int                RULE_NO;
};

struct Grammar {
  int           GRAMMAR_RULES_SIZE;
  struct Rule** GRAMMAR_RULES;
};

struct FirstAndFollow {
  int** FIRST;
  int** FOLLOW;
};

struct ParsingTable {
  int** entries;
};

struct NaryTreeNode;

struct NonLeafNode {
  int                  ENUM_ID;
  int                  NUMBER_CHILDREN;
  int                  RULE_NO;
  struct NaryTreeNode* child;
};

struct LeafNode {
  int    ENUM_ID;
  Token* TOKEN;
};

union NodeType {
  struct NonLeafNode NL;
  struct LeafNode    L;
};

struct NaryTreeNode {
  union NodeType       NODE_TYPE;
  int                  IS_LEAF_NODE;
  struct NaryTreeNode* parent;
  struct NaryTreeNode* next;
};

struct ParseTree {
  struct NaryTreeNode* root;
};

struct StackNode {
  struct NaryTreeNode* TREE_NODE;
  struct StackNode*    next;
};

struct Stack {
  struct StackNode* HEAD;
  int               NUM_NODES;
};

// symbol ops
struct Symbol*     initializeSymbol(char* symbol);
struct SymbolList* initializeSymbolList();
void               addToSymbolList(struct SymbolList* ls, struct Symbol* s);
char*              appendToSymbol(char* str, char c);
char*              copyLexeme(char* str);

// parse tree ops
struct ParseTree*    initializeParseTree();
struct NaryTreeNode* createLeafNode(int enumId);
struct NaryTreeNode* createNonLeafNode(int enumId);
struct NaryTreeNode* createNode(int isTerminal, union SymbolType type, struct NaryTreeNode* parent);
void                 addRuleToParseTree(struct NaryTreeNode* ntn, struct Rule* r);
void                 printTree(struct ParseTree* pt);
void                 printNaryTree(struct NaryTreeNode* nt);
int                  getParseTreeNodeCount();
int                  getParseTreeMemory();

// stack ops
struct StackNode*    createStackNode(struct NaryTreeNode* ntn);
void                 push(struct Stack* st, struct NaryTreeNode* ntn);
struct NaryTreeNode* top(struct Stack* st);
void                 pop(struct Stack* st);
struct Stack*        initializeStack(struct ParseTree* pt);
void                 pushTreeChildren(struct Stack* st, struct NaryTreeNode* ntn);

#endif