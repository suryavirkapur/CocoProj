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
  Terminal             TERMINAL;
  enum NonTerminalEnum NON_TERMINAL;
};

struct Symbol {
  union SymbolType symType;
  int              isTerminal;
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
  struct SymbolList* symbols;
  int                ruleNum;
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
  int                  enumID;
  int                  numChildren;
  int                  ruleNum;
  struct NaryTreeNode* child;
};

struct LeafNode {
  int    enumID;
  Token* TOKEN;
};

union NodeType {
  struct NonLeafNode NL;
  struct LeafNode    L;
};

struct NaryTreeNode {
  union NodeType       nodeType;
  int                  isLeaf;
  struct NaryTreeNode* parent;
  struct NaryTreeNode* next;
};

struct ParseTree {
  struct NaryTreeNode* root;
};

struct StackNode {
  struct NaryTreeNode* treeNode;
  struct StackNode*    next;
};

struct Stack {
  struct StackNode* HEAD;
  int               numNodes;
};

// symbol ops
struct Symbol*     initializeSymbol(char* symbol);
struct SymbolList* initializeSymbolList();
void               addToSymbolList(struct SymbolList* symList, struct Symbol* s);
char*              appendToSymbol(char* str, char c);
char*              copyLexeme(char* str);

// parse tree ops
struct ParseTree*    initializeParseTree();
struct NaryTreeNode* createLeafNode(int mapIndex);
struct NaryTreeNode* createNonLeafNode(int mapIndex);
struct NaryTreeNode* createNode(int isTerminal, union SymbolType type, struct NaryTreeNode* parent);
void                 addRuleToParseTree(struct NaryTreeNode* ntn, struct Rule* rule);

// stack ops
struct StackNode*    createStackNode(struct NaryTreeNode* ntn);
void                 push(struct Stack* st, struct NaryTreeNode* ntn);
struct NaryTreeNode* top(struct Stack* st);
void                 pop(struct Stack* st);
struct Stack*        initializeStack(struct ParseTree* parseTable);
void                 pushTreeChildren(struct Stack* st, struct NaryTreeNode* ntn);

#endif