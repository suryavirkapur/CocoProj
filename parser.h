/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

#ifndef PARSER_H
#define PARSER_H

#include "dataStructures.h"
#include "firstFollow.h"
#include "grammar.h"
#include "parse.h"

struct Grammar*        extractGrammar();
struct FirstAndFollow* computeFirstAndFollowSets(struct Grammar* parsedGrammar);
void                   createParseTable(struct FirstAndFollow* firstAndFollowSets, struct ParsingTable* pt);
struct ParseTree*
     parseInputSourceCode(char* testcaseFile, struct ParsingTable* pTable, struct FirstAndFollow* firstAndFollowSets);
void writeParseTreeToFile(struct ParseTree* pt, char* outputFile);
int  getErrorStatus();

#endif