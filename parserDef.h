#ifndef PARSE_DEF_
#define PARSE_DEF_

#include "lexerDef.h"

typedef enum NonTerminal {
    program,
    mainFunction,
    otherFunctions,
    function,
    input_par,
    output_par,
    parameter_list,
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
} NonTerminal;

typedef TokenName Terminal;

typedef struct FirstAndFollow {
    int** FIRST;
    int** FOLLOW;
} FirstAndFollow;

typedef union SymbolType {
    Terminal TERMINAL;
    NonTerminal NON_TERMINAL;
} SymbolType;

typedef struct Symbol {
    SymbolType TYPE;
    int IS_TERMINAL;
    struct Symbol* next;
} Symbol;

typedef struct SymbolList {
    Symbol* HEAD_SYMBOL;
    Symbol* TAIL_SYMBOL;
    int RULE_LENGTH;
} SymbolList;

typedef struct NonTerminalRuleRecords {
    int start;
    int end;
} NonTerminalRuleRecords;

typedef struct Rule {
    SymbolList* SYMBOLS;
    int RULE_NO;
} Rule;

typedef struct Grammar {
    int GRAMMAR_RULES_SIZE;
    Rule** GRAMMAR_RULES;
} Grammar;

typedef struct ParsingTable {
    int** entries;
} ParsingTable;

typedef struct NaryTreeNode NaryTreeNode;

typedef struct NonLeafNode {
    int ENUM_ID;
    int NUMBER_CHILDREN;
    int RULE_NO;
    NaryTreeNode* child;
} NonLeafNode;

typedef struct LeafNode {
    int ENUM_ID;
    Token* TK;
} LeafNode;

typedef union NodeType {
    NonLeafNode NL;
    LeafNode L;
} NodeType;

typedef struct NaryTreeNode {
    NodeType NODE_TYPE;
    int IS_LEAF_NODE;
    struct NaryTreeNode* parent;
    struct NaryTreeNode* next;
} NaryTreeNode;

typedef struct ParseTree {
    NaryTreeNode* root;
} ParseTree;

typedef struct StackNode {
    NaryTreeNode* TREE_NODE;
    struct StackNode* next;
} StackNode;

typedef struct Stack {
    StackNode* HEAD;
    int NUM_NODES;
} Stack;

#endif