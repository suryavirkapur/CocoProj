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

int                             initializeGrammar();
struct Grammar*                 extractGrammar();
struct NonTerminalRuleRecords** initializeNonTerminalRecords();
struct Rule*                    initializeRule(struct SymbolList* sl, int ruleCount);
int                             findInNonTerminalMap(char* str);
int                             findInTerminalMap(char* str);
char*                           getTerminal(int enumId);
char*                           getNonTerminal(int enumId);

extern struct Grammar*                 parsedGrammar;
extern struct NonTerminalRuleRecords** nonTerminalRuleRecords;

extern bool nonTerminalProcessed[];
extern int  symbolVectorSize;

extern bool syntaxErrorOccurred;
extern bool lexicalErrorOccurred;

extern char* TerminalMap[];
extern char* NonTerminalMap[];

#endif