#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "dataStructures.h"

// Function declarations for grammar operations
int initializeGrammar();
struct Grammar* extractGrammar();
struct NonTerminalRuleRecords** initializeNonTerminalRecords();
struct Rule* initializeRule(struct SymbolList* sl, int ruleCount);
int findInNonTerminalMap(char* str);
int findInTerminalMap(char* str);
char* getTerminal(int enumId);
char* getNonTerminal(int enumId);
void printSymbol(struct Symbol* ls);
void printRule(struct Rule* r);
void printGrammarStructure();
void printNonTerminalRuleRecords();
void verifyGrammar();
void verifyNTRR();

// External variables
extern struct Grammar* g;
extern struct NonTerminalRuleRecords** ntrr;
extern int checkIfDone[];
extern int vectorSize;

extern int syntaxErrorFlag;
extern int lexicalErrorFlag;

extern char* TerminalID[];
extern char* NonTerminalID[];

#endif // GRAMMAR_H