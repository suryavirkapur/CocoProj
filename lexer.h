/*
Group No. 46
- Suryavir Kapur (2022A7PS0293U)
- Ronit Dhansoia (2022A7PS0168U)
- Anagh Goyal (2022A7PS0177U)
- Harshwardhan Sugam (2022A7PS0114P)
*/

#include "lexerDef.h"

void   setupBuffers(int f);
void   setupLexer(int f);
void   removeComments(char* testCaseFile, char* cleanFile);
int    setupInputStream();
char   nextChar();
void   retract(int amt);
Token* fillToken(Token* t, TokenName tokenName, char* lexeme, int lineNumber, int isNumber, tVal* value);
Token* getToken();
