/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/
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
void writeParseTreeToFile(struct ParseTree* pt, char* outputFile);
void printParseTreeHelper(struct NaryTreeNode* pt, FILE* f);
int  getErrorStatus();
int  isSynchronizingToken(TokenName token);

#endif
