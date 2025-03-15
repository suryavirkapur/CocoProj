/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "dataStructures.h"
#include <stdbool.h>

int                             setupGrammar();
struct Grammar*                 extractGrammar();


struct Rule*                    setupRule(struct SymbolList* sl, int ruleCount);


char*                           getTerminal(int mapIndex);
char*                           getNonTerminal(int mapIndex);

int                             findInNonTerminalMap(char* str);
int                             findInTerminalMap(char* str);


extern struct Grammar*                 parsedGrammar;
extern struct NonTerminalRuleRecords** nonTerminalRuleRecords;

extern bool nonTerminalProcessed[];
extern int  symbolVectorSize;

extern bool syntaxErrorOccurred;
extern bool lexicalErrorOccurred;

extern char* TerminalMap[];
extern char* NonTerminalMap[];

#endif