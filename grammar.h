#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <stdbool.h>
#include "dataStructures.h"

int initializeGrammar();
struct Grammar* extractGrammar();
struct NonTerminalRuleRecords** initializeNonTerminalRecords();
struct Rule* initializeRule(struct SymbolList* sl, int ruleCount);
int findInNonTerminalMap(char* str);
int findInTerminalMap(char* str);
char* getTerminal(int enumId);
char* getNonTerminal(int enumId);


extern struct Grammar* parsedGrammar;
extern struct NonTerminalRuleRecords** nonTerminalRuleRecords;

extern bool nonTerminalProcessed[];
extern int symbolVectorSize;

extern bool syntaxErrorOccurred;
extern bool lexicalErrorOccurred;

extern char* TerminalID[];
extern char* NonTerminalID[];

#endif 