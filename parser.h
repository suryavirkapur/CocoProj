#include "parserDef.h"

int initialiseGrammar();
Grammar* extractGrammar();
Grammar* extractGrammarNewFormat();
FirstAndFollow* computeFirstAndFollowSets(Grammar* g);
void populateFirst(int** firstVector, Grammar* g);
void calculateFirst(int** firstVector, int enum_id);
void populateFollow(int** followBitVector, int** firstSet, Grammar* g);
void populateFollowTillStable(int** followVector, int** firstVector, Grammar* g);
ParsingTable* initialiseParsingTable();
void createParseTable(FirstAndFollow* fafl, ParsingTable* pt);
ParseTree* parseInputSourceCode(char* testcaseFile, ParsingTable* pTable, FirstAndFollow* fafl);
void printParseTree(ParseTree* pt, char* outfile);
void printParseTreeHelper(NaryTreeNode* pt, FILE* f);

void initialiseCheckIfDone();

int findInNonTerminalMap(char* str);
int findInTerminalMap(char* str);

NonTerminalRuleRecords** intialiseNonTerminalRecords();
Rule* initialiseRule(SymbolList* sl, int ruleCount);

Symbol* intialiseSymbol(char* symbol);
SymbolList* initialiseSymbolList();
void addToSymbolList(SymbolList* ls, Symbol* s);

char* getTerminal(int enumId);
char* getNonTerminal(int enumId);
char* appendToSymbol(char* str, char c);
char* copyLexeme(char* str);

void printSymbol(Symbol* ls);
void printRule(Rule* r);
void printGrammarStructure();
void printNonTerminalRuleRecords();
void printFirstSets(FirstAndFollow* fafl);
void printFollowSets(FirstAndFollow* fafl);
void printParseTable(ParsingTable* pt);
int getErrorStatus();

ParseTree* initialiseParseTree();
NaryTreeNode* createLeafNode(int enumId);
NaryTreeNode* createNonLeafNode(int enumId);
NaryTreeNode* createNode(int isTerminal, SymbolType type, NaryTreeNode* parent);
void addRuleToParseTree(NaryTreeNode* ntn, Rule* r);
void printTree(ParseTree* pt);
void printNaryTree(NaryTreeNode* nt);
int getParseTreeNodeCount();
int getParseTreeMemory();

StackNode* createStackNode(NaryTreeNode* ntn);
void push(Stack* st, NaryTreeNode* ntn);
NaryTreeNode* top(Stack* st);
void pop(Stack* st);
Stack* initialiseStack(ParseTree* pt);
void pushTreeChildren(Stack* st, NaryTreeNode* ntn);