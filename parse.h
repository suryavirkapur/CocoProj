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

struct ParsingTable* createParsingTable();
void createParseTable(struct FirstAndFollow* firstAndFollowSets, struct ParsingTable* parseTable);
struct ParseTree* parseSourceCode(char* testcaseFile, struct ParsingTable* pTable, struct FirstAndFollow* firstAndFollowSets);
void writeParseTreeToFile(struct ParseTree* parseTable, char* outputFile);
void printParseTable(struct NaryTreeNode* parseTable, FILE* f);
int isSynchronizingToken(TokenName token);

#endif