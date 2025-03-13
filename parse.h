#ifndef PARSE_H
#define PARSE_H

#include "dataStructures.h"
#include "grammar.h"
#include "firstFollow.h"

// Function declarations for parsing operations
struct ParsingTable* initializeParsingTable();
void createParseTable(struct FirstAndFollow* fafl, struct ParsingTable* pt);
struct ParseTree* parseInputSourceCode(char* testcaseFile, struct ParsingTable* pTable, struct FirstAndFollow* fafl);
void printParseTree(struct ParseTree* pt, char* outfile);
void printParseTreeHelper(struct NaryTreeNode* pt, FILE* f);
void printParseTable(struct ParsingTable* pt);
int getErrorStatus();
int isSynchronizingToken(TokenName token);

#endif 
