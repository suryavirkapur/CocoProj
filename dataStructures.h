#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "lexerDef.h"

// Non-terminal symbol enumeration
enum NonTerminal {
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

// Union to represent a symbol type (terminal or non-terminal)
union SymbolType {
    Terminal TERMINAL;
    enum NonTerminal NON_TERMINAL;
};

// Symbol structure for grammar rules
struct Symbol {
    union SymbolType TYPE;
    int IS_TERMINAL;
    struct Symbol* next;
};

// List of symbols for grammar rules
struct SymbolList {
    struct Symbol* HEAD_SYMBOL;
    struct Symbol* TAIL_SYMBOL;
    int RULE_LENGTH;
};

// Tracks rule ranges for non-terminals
struct NonTerminalRuleRecords {
    int start;
    int end;
};

// Grammar rule structure
struct Rule {
    struct SymbolList* SYMBOLS;
    int RULE_NO;
};

// The complete grammar
struct Grammar {
    int GRAMMAR_RULES_SIZE;
    struct Rule** GRAMMAR_RULES;
};

// First and Follow sets
struct FirstAndFollow {
    int** FIRST;
    int** FOLLOW;
};

// Parse table structure
struct ParsingTable {
    int** entries;
};

// Forward declaration for tree node
struct NaryTreeNode;

// Non-leaf node in the parse tree
struct NonLeafNode {
    int ENUM_ID;
    int NUMBER_CHILDREN;
    int RULE_NO;
    struct NaryTreeNode* child;
};

// Leaf node in the parse tree
struct LeafNode {
    int ENUM_ID;
    Token* TK;
};

// Union for node type
union NodeType {
    struct NonLeafNode NL;
    struct LeafNode L;
};

// Tree node structure
struct NaryTreeNode {
    union NodeType NODE_TYPE;
    int IS_LEAF_NODE;
    struct NaryTreeNode* parent;
    struct NaryTreeNode* next;
};

// Parse tree structure
struct ParseTree {
    struct NaryTreeNode* root;
};

// Stack node for parsing
struct StackNode {
    struct NaryTreeNode* TREE_NODE;
    struct StackNode* next;
};

// Stack for parsing
struct Stack {
    struct StackNode* HEAD;
    int NUM_NODES;
};

// Function declarations for data structure operations
struct Symbol* initializeSymbol(char* symbol);
struct SymbolList* initializeSymbolList();
void addToSymbolList(struct SymbolList* ls, struct Symbol* s);
char* appendToSymbol(char* str, char c);
char* copyLexeme(char* str);

// Parse tree operations
struct ParseTree* initializeParseTree();
struct NaryTreeNode* createLeafNode(int enumId);
struct NaryTreeNode* createNonLeafNode(int enumId);
struct NaryTreeNode* createNode(int isTerminal, union SymbolType type, struct NaryTreeNode* parent);
void addRuleToParseTree(struct NaryTreeNode* ntn, struct Rule* r);
void printTree(struct ParseTree* pt);
void printNaryTree(struct NaryTreeNode* nt);
int getParseTreeNodeCount();
int getParseTreeMemory();

// Stack operations
struct StackNode* createStackNode(struct NaryTreeNode* ntn);
void push(struct Stack* st, struct NaryTreeNode* ntn);
struct NaryTreeNode* top(struct Stack* st);
void pop(struct Stack* st);
struct Stack* initializeStack(struct ParseTree* pt);
void pushTreeChildren(struct Stack* st, struct NaryTreeNode* ntn);

#endif