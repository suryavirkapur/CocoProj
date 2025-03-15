#ifndef PARSE_H
#define PARSE_H

#include "dataStructures.h"
#include "firstFollow.h"
#include "grammar.h"
#include <stdio.h>

struct ParsingTable* initializeParsingTable();
void                 createParseTable(struct FirstAndFollow* firstAndFollowSets, struct ParsingTable* pt);
struct ParseTree*
     parseInputSourceCode(char* testcaseFile, struct ParsingTable* pTable, struct FirstAndFollow* firstAndFollowSets);
void writeParseTreeToFile(struct ParseTree* pt, char* outfile);
void printParseTreeHelper(struct NaryTreeNode* pt, FILE* f);
int  getErrorStatus();
int  isSynchronizingToken(TokenName token);

#endif
