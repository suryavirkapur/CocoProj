#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

#define BUFFER_SIZE 4096

char* buffPair[2];
int bufferToBeLoaded = 0;
int currentBuffer = 1;
int inputExhausted = 0;
int fp;
char* lexemeBegin = NULL;
char* forward = NULL;
int dfa_state = 0;
int lineCount = 1;
int nextCharacterReadAfterRetraction = 0;

typedef struct Node {
    TokenName TOKEN_NAME;
    char* LEXEME;
    struct Node* next;
} Node;

typedef struct KeywordNode {
    Node* keywords;
} KeywordNode;

typedef struct KeywordTable {
    KeywordNode* KEYWORDS;
} KeywordTable;

KeywordTable* kt;
#define NUMBER_KEYWORDS 29

int hashFunction(char* str) {
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c;
    return (hash % NUMBER_KEYWORDS);
}

Node* addToList(Node* ls, TokenName tn, char* lexeme) {
    if(ls == NULL) {
        Node* n = (Node*)malloc(sizeof(Node));
        n->LEXEME = lexeme;
        n->TOKEN_NAME = tn;
        n->next = NULL;
        return n;
    }
    Node* n = (Node*)malloc(sizeof(Node));
    n->LEXEME = lexeme;
    n->TOKEN_NAME = tn;
    n->next = ls;
    return n;
}

void addEntry(KeywordTable* kt, TokenName tn, char* lexeme) {
    int hash = hashFunction(lexeme);
    kt->KEYWORDS[hash].keywords = addToList(kt->KEYWORDS[hash].keywords, tn, lexeme);
}

Node* lookUp(KeywordTable* kt, char* lexeme) {
    int hash = hashFunction(lexeme);
    Node* trav = kt->KEYWORDS[hash].keywords;
    while(trav != NULL) {
        if(strcmp(lexeme, trav->LEXEME) == 0)
            return trav;
        trav = trav->next;
    }
    return NULL;
}

KeywordTable* initializeTable() {
    KeywordTable* kt = (KeywordTable*)malloc(sizeof(KeywordTable));
    kt->KEYWORDS = (KeywordNode*)malloc(NUMBER_KEYWORDS * sizeof(KeywordNode));
    
    for(int i = 0; i < NUMBER_KEYWORDS; i++) {
        kt->KEYWORDS[i].keywords = NULL;
    }
    
    addEntry(kt, TK_WITH, "with");
    addEntry(kt, TK_PARAMETERS, "parameters");
    addEntry(kt, TK_END, "end");
    addEntry(kt, TK_WHILE, "while");
    addEntry(kt, TK_TYPE, "type");
    addEntry(kt, TK_MAIN, "_main");
    addEntry(kt, TK_GLOBAL, "global");
    addEntry(kt, TK_PARAMETER, "parameter");
    addEntry(kt, TK_LIST, "list");
    addEntry(kt, TK_INPUT, "input");
    addEntry(kt, TK_OUTPUT, "output");
    addEntry(kt, TK_INT, "int");
    addEntry(kt, TK_REAL, "real");
    addEntry(kt, TK_ENDWHILE, "endwhile");
    addEntry(kt, TK_IF, "if");
    addEntry(kt, TK_THEN, "then");
    addEntry(kt, TK_ENDIF, "endif");
    addEntry(kt, TK_READ, "read");
    addEntry(kt, TK_WRITE, "write");
    addEntry(kt, TK_RETURN, "return");
    addEntry(kt, TK_CALL, "call");
    addEntry(kt, TK_RECORD, "record");
    addEntry(kt, TK_ENDRECORD, "endrecord");
    addEntry(kt, TK_ELSE, "else");
    addEntry(kt, TK_DEFINETYPE, "definetype"); 
    addEntry(kt, TK_AS, "as");
    addEntry(kt, TK_DEFINETYPE, "definetype");
    addEntry(kt, TK_UNION, "union");
    addEntry(kt, TK_ENDUNION, "endunion");
    
    return kt;
}

void initializeBuffers(int f) {
    fp = f;
    buffPair[0] = (char*)malloc(BUFFER_SIZE * sizeof(char));
    buffPair[1] = (char*)malloc(BUFFER_SIZE * sizeof(char));
    lexemeBegin = NULL;
    forward = NULL;
    currentBuffer = 1;
    bufferToBeLoaded = 0;
    inputExhausted = 0;
    lineCount = 1;
    nextCharacterReadAfterRetraction = 0;
    dfa_state = 0;
}

void initializeLexer(int f) {
    initializeBuffers(f);
    kt = initializeTable();
}

int getInputStream() {
    if(inputExhausted != 0)
        return EOF;
    
    memset(buffPair[bufferToBeLoaded], EOF, BUFFER_SIZE);
    
    int res = read(fp, buffPair[bufferToBeLoaded], BUFFER_SIZE);
    if(res == 0 || res < BUFFER_SIZE) {
        inputExhausted = 1;
    }
    
    bufferToBeLoaded = 1 - bufferToBeLoaded;
    currentBuffer = 1 - currentBuffer;
    if(res == -1) {
        printf("Error: Input Buffers failed to be loaded\n");
        return -1;
    }
    return res;
}

char nextChar() {
    if(lexemeBegin == NULL && forward == NULL) {
        int res = getInputStream();
        if(res == -1) {
            return EOF;
        }
        lexemeBegin = buffPair[currentBuffer];
        forward = buffPair[currentBuffer];
        char* curr_forward = forward;
        forward++;
        
        if(nextCharacterReadAfterRetraction == 0 && *curr_forward == '\n') {
            lineCount++;
        }
        
        if(nextCharacterReadAfterRetraction == 1)
            nextCharacterReadAfterRetraction = 0;
            
        return *curr_forward;
    }
    
    char* curr_forward = forward;
    
    if(curr_forward - buffPair[currentBuffer] == BUFFER_SIZE-1) {
        int res = getInputStream();
        if(res == -1) {
            return EOF;
        }
        forward = buffPair[currentBuffer];
    }
    else if(*curr_forward == EOF) {
        return EOF;
    }
    else
        forward++;
        
    if(nextCharacterReadAfterRetraction == 0 && *curr_forward == '\n') {
        lineCount++;
    }
    
    if(nextCharacterReadAfterRetraction == 1)
        nextCharacterReadAfterRetraction = 0;
        
    return *curr_forward;
}

char* copyString(char* start, char* end) {
    char* store = (char*)malloc((end-start+2) * sizeof(char));
    char* trav = start;
    int i = 0;
    
    while(trav < end) {
        store[i] = *trav;
        ++trav;
        ++i;
    }
    
    store[i] = '\0';
    return store;
}

void accept() {
    lexemeBegin = forward;
}

void retract(int amt) {
    while(amt > 0) {
        --forward;
        --amt;
    }
    
    nextCharacterReadAfterRetraction = 1;
}

int rangeMatch(char ch, char start, char end) {
    if(ch >= start && ch <= end)
        return 1;
    return 0;
}

int singleMatch(char ch, char chToEqual) {
    if(ch == chToEqual)
        return 1;
    return 0;
}

Token* populateToken(Token* t, TokenName tokenName, char* lexeme, int lineNumber, int isNumber, Value* value) {
    t->TOKEN_NAME = tokenName;
    t->LINE_NO = lineNumber;
    t->IS_NUMBER = isNumber;
    t->LEXEME = lexeme;
    t->VALUE = value;
    return t;
}

int stringToInteger(char* str) {
    int num;
    sscanf(str, "%d", &num);
    return num;
}

float stringToFloat(char* str) {
    float f;
    sscanf(str, "%f", &f);
    return f;
}

Token* getToken() {
    dfa_state = 0;
    char c = 1;
    Token* t = (Token*)malloc(sizeof(Token));
    int errorType;

    while(1) {
        if(c == EOF)
            return NULL;

        switch(dfa_state) {
            case 0: {
                c = nextChar();
                if(singleMatch(c,'<')) {
                    dfa_state = 16;
                }
                else if(singleMatch(c,'#')) {
                    dfa_state = 52;
                }
                else if(rangeMatch(c,'b','d')) {
                    dfa_state = 35;
                }
                else if(singleMatch(c,'a') || rangeMatch(c,'e','z')) {
                    dfa_state = 40;
                }
                else if(singleMatch(c,'_')) {
                    dfa_state = 47;
                }
                else if(singleMatch(c,'[')) {
                    dfa_state = 7;
                }
                else if(singleMatch(c,']')) {
                    dfa_state = 8;
                }
                else if(singleMatch(c,',')) {
                    dfa_state = 9;
                }
                else if(singleMatch(c,';')) {
                    dfa_state = 12;
                }
                else if(singleMatch(c,':')) {
                    dfa_state = 11;
                }
                else if(singleMatch(c,'.')) {
                    dfa_state = 10;
                }
                else if(singleMatch(c,'(')) {
                    dfa_state = 5;
                }
                else if(singleMatch(c,')')) {
                    dfa_state = 6;
                }
                else if(singleMatch(c,'+')) {
                    dfa_state = 1;
                }
                else if(singleMatch(c,'-')) {
                    dfa_state = 2;
                }
                else if(singleMatch(c,'*')) {
                    dfa_state = 3;
                }
                else if(singleMatch(c,'/')) {
                    dfa_state = 4;
                }
                else if(singleMatch(c,'~')) {
                    dfa_state = 13;
                }
                else if(singleMatch(c,'!')) {
                    dfa_state = 14;
                }
                else if(singleMatch(c,'>')) {
                    dfa_state = 22;
                }
                else if(singleMatch(c,'=')) {
                    dfa_state = 25;
                }
                else if(singleMatch(c,'@')) {
                    dfa_state = 27;
                }
                else if(singleMatch(c,'&')) {
                    dfa_state = 30;
                }
                else if(singleMatch(c,'%')) {
                    dfa_state = 33;
                }
                else if(rangeMatch(c,'0','9')) {
                    dfa_state = 42;
                }
                else if(singleMatch(c,' ') || singleMatch(c,'\f') || singleMatch(c,'\r') || singleMatch(c,'\t') || singleMatch(c,'\v')) {
                    lexemeBegin++;
                    dfa_state = 0;
                }
                else if(singleMatch(c,'\n')) {
                    lexemeBegin++;
                    dfa_state = 0;
                }
                else if(singleMatch(c,EOF)) {
                    return NULL;
                }
                else {
                    printf("Line %d : Cannot recognize character %c \n" ,lineCount,c);
                    errorType = 6;
                    dfa_state = 55;
                }
                break;
            }
            case 1: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_PLUS,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 2: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_MINUS,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 3: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_MUL,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 4: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_DIV,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 5: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_OP,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 6: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_CL,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 7: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_SQL,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 8: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_SQR,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 9: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_COMMA,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 10: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_DOT,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 11: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_COLON,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 12: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_SEM,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 13: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_NOT,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 14: {
                c = nextChar();
                if(c == '=') {
                    dfa_state = 15;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for != ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 15: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_NE,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 16: {
                c = nextChar();
                if(c == '-') {
                    dfa_state = 17;
                }
                else if(c == '=') {
                    dfa_state = 20;
                }
                else {
                    dfa_state = 21;
                }
                break;
            }
            case 17: {
                c = nextChar();
                if(c == '-') {
                    dfa_state = 18;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for <--- ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 18: {
                c = nextChar();
                if(c == '-') {
                    dfa_state = 19;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for <--- ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 19: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_ASSIGNOP,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 20: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_LE,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 21: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);

                if(c == '\n')
                    populateToken(t,TK_LT,lex,lineCount-1,0,NULL);
                else
                    populateToken(t,TK_LT,lex,lineCount,0,NULL);

                accept();
                return t;
                break;
            }
            case 22: {
                c = nextChar();
                if(c == '=') {
                    dfa_state = 23;
                }
                else {
                    dfa_state = 24;
                }
                break;
            }
            case 23: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_GE,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 24: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);
                if(c == '\n')
                    populateToken(t,TK_GT,lex,lineCount-1,0,NULL);
                else
                    populateToken(t,TK_GT,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 25: {
                c = nextChar();
                if(c == '=') {
                    dfa_state = 26;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for == ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 26: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_EQ,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 27: {
                c = nextChar();
                if(c == '@') {
                    dfa_state = 28;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for @@@ ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 28: {
                c = nextChar();
                if(c == '@') {
                    dfa_state = 29;
                }
                else {
                    char* pattern = copyString(lexemeBegin,forward);
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for @@@ ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 29: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_OR,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 30: {
                c = nextChar();
                if(c == '&') {
                    dfa_state = 31;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for &&& ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 31: {
                c = nextChar();
                if(c == '&') {
                    dfa_state = 32;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for &&& ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 32: {
                char* lex = copyString(lexemeBegin,forward);
                populateToken(t,TK_AND,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 33: {
                c = nextChar();
                while(c != '\n' && c != EOF) {
                    c = nextChar();
                }
                dfa_state = 34;
                break;
            }
            case 34: {
                char* lex = copyString(lexemeBegin,forward);
                if(c == '\n')
                    populateToken(t,TK_COMMENT,lex,lineCount-1,0,NULL);
                else
                    populateToken(t,TK_COMMENT,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 35: {
                c = nextChar();
                if(rangeMatch(c,'2','7')) {
                    dfa_state = 36;
                }
                else if(rangeMatch(c,'a','z')) {
                    dfa_state = 40;
                }
                else {
                    dfa_state = 41;
                }
                break;
            }
            case 36: {
                c = nextChar();
                while(rangeMatch(c,'b','d'))
                    c = nextChar();

                if(rangeMatch(c,'2','7'))
                    dfa_state = 37;
                else {
                    dfa_state = 39;
                }
                break;
            }
            case 37: {
                c = nextChar();
                while(rangeMatch(c,'2','7'))
                    c = nextChar();

                if(!rangeMatch(c,'2','7') && !rangeMatch(c,'b','d')) {
                    dfa_state = 38;
                }
                else {
                    printf("Line %d : two identifers are not allowed back to back without a break ?\n" ,lineCount);
                    errorType = 5;
                    dfa_state = 55;
                }
                break;
            }
            case 38: {
                retract(1);
                int identifierLength = forward - lexemeBegin + 1;
                if(identifierLength < 2) {
                    printf("Line %d : Identifier length is less than 2\n" , lineCount);
                    errorType = 4;
                    dfa_state = 55;
                }
                else if(identifierLength > 20) {
                    printf("Line %d : Identifier length is more than 20\n" ,lineCount);
                    errorType = 4;
                    dfa_state = 55;
                }
                else {
                    char* lex = copyString(lexemeBegin,forward);
                    if(c == '\n')
                        populateToken(t,TK_ID,lex,lineCount-1,0,NULL);
                    else
                        populateToken(t,TK_ID,lex,lineCount,0,NULL);
                    accept();
                    return t;
                }
                break;
            }
            case 39: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);
                if(c == '\n')
                    populateToken(t,TK_ID,lex,lineCount-1,0,NULL);
                else
                    populateToken(t,TK_ID,lex,lineCount,0,NULL);

                accept();
                return t;
                break;
            }
            case 40: {
                c = nextChar();
                while(rangeMatch(c,'a','z'))
                    c = nextChar();

                dfa_state = 41;
                break;
            }
            case 41: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);
                Node* n = lookUp(kt,lex);
                if(n == NULL) {
                    if(c == '\n')
                        populateToken(t,TK_FIELDID,lex,lineCount-1,0,NULL);
                    else
                        populateToken(t,TK_FIELDID,lex,lineCount,0,NULL);
                }
                else {
                    if(c == '\n')
                        populateToken(t,n->TOKEN_NAME,lex,lineCount-1,0,NULL);
                    else
                        populateToken(t,n->TOKEN_NAME,lex,lineCount,0,NULL);
                }
                accept();
                return t;
                break;
            }
            case 42: {
                c = nextChar();
                while(rangeMatch(c,'0','9'))
                    c = nextChar();

                if(c == '.') {
                    dfa_state = 44;
                }
                else {
                    dfa_state = 43;
                }
                break;
            }
            case 43: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);
                Value* val = malloc(sizeof(Value));
                val->INT_VALUE = stringToInteger(lex);
                if(c == '\n')
                    populateToken(t,TK_NUM,lex,lineCount-1,1,val);
                else
                    populateToken(t,TK_NUM,lex,lineCount,1,val);
                accept();
                return t;
                break;
            }
            case 44: {
                c = nextChar();
                if(rangeMatch(c,'0','9')) {
                    dfa_state = 45;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a real number ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 45: {
                c = nextChar();
                if(rangeMatch(c,'0','9')) {
                    dfa_state = 46;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a real number ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 46: {
                char* lex = copyString(lexemeBegin,forward);
                Value* val = (Value*)malloc(sizeof(Value));
                val->FLOAT_VALUE = stringToFloat(lex);
                populateToken(t,TK_RNUM,lex,lineCount,2,val);
                accept();
                return t;
                break;
            }
            case 47: {
                c = nextChar();
                if(rangeMatch(c,'a','z') || rangeMatch(c,'A','Z')) {
                    dfa_state = 48;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a function ID ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 48: {
                c = nextChar();
                while(rangeMatch(c,'a','z') || rangeMatch(c,'A','Z'))
                    c = nextChar();

                if(rangeMatch(c,'0','9')) {
                    dfa_state = 49;
                }
                else {
                    dfa_state = 51;
                }
                break;
            }
            case 49: {
                c = nextChar();
                while(rangeMatch(c,'0','9'))
                    c = nextChar();

                dfa_state = 50;

                break;
            }
            case 50: {
                retract(1);
                int identifierLength = forward - lexemeBegin + 1;
                if(identifierLength > 30) {
                    printf("Line %d : Function identifier length exceeds 30 characters\n" ,lineCount);
                    errorType = 4;
                    dfa_state = 55;
                }
                else {
                    char* lex = copyString(lexemeBegin,forward);
                    if(c == '\n')
                        populateToken(t,TK_FUNID,lex,lineCount,0,NULL);
                    else
                        populateToken(t,TK_FUNID,lex,lineCount-1,0,NULL);
                    accept();
                    return t;
                }
                break;
            }
            case 51: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);
                Node* n = lookUp(kt,lex);
                if(n == NULL) {
                    if(c == '\n')
                        populateToken(t,TK_FUNID,lex,lineCount-1,0,NULL);
                    else
                        populateToken(t,TK_FUNID,lex,lineCount,0,NULL);
                }
                else {
                    if(c == '\n')
                        populateToken(t,n->TOKEN_NAME,lex,lineCount-1,0,NULL);
                    else
                        populateToken(t,n->TOKEN_NAME,lex,lineCount,0,NULL);
                }
                accept();
                return t;
                break;
            }
            case 52: {
                c = nextChar();
                if(rangeMatch(c,'a','z')) {
                    dfa_state = 53;
                }
                else {
                    char* pattern = copyString(lexemeBegin, forward-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a record ID ?\n" ,lineCount,pattern);
                    free(pattern);
                    errorType = 3;
                    dfa_state = 55;
                    retract(1);
                }
                break;
            }
            case 53: {
                c = nextChar();
                while(rangeMatch(c,'a','z'))
                    c = nextChar();

                dfa_state = 54;
                break;
            }
            case 54: {
                retract(1);
                char* lex = copyString(lexemeBegin,forward);
                if(c == '\n')
                    populateToken(t,TK_RUID,lex,lineCount-1,0,NULL);
                else
                    populateToken(t,TK_RUID,lex,lineCount,0,NULL);
                accept();
                return t;
                break;
            }
            case 55: {
                char* lex = copyString(lexemeBegin,forward);
                if(errorType == 3 && c == '\n')
                    populateToken(t,TK_ERR,lex,lineCount-1,errorType,NULL);
                else
                    populateToken(t,TK_ERR,lex,lineCount,errorType,NULL);
                accept();
                return t;
                break;
            }
        }
    }
}

void removeComments(char* testCaseFile, char* cleanFile) {
    int tcf = open(testCaseFile, O_RDONLY);
    initializeBuffers(tcf);
    int check = 0;
    char c;
    while((c = nextChar()) != EOF) {
        switch(check) {
            case 0: {
                if(c == ' ' || c == '\f' || c == '\r' || c == '\t' || c == '\v') {
                    write(1, &c, 1);
                    check = 0;
                }
                else if(c == '%') {
                    check = 3;
                }
                else if(c == '\n') {
                    write(1, &c, 1);
                    check = 0;
                }
                else {
                    write(1, &c, 1);
                    check = 2;
                }
                break;
            }
            case 2: {
                write(1, &c, 1);
                if(c == '\n')
                    check = 0;
                break;
            }
            case 3: {
                if(c == '\n') {
                    write(1, &c, 1);
                    check = 0;
                }
                break;
            }
        }
    }
    close(tcf);
}

void printBuffers() {
    char c;
    while((c = nextChar()) != EOF) {
        printf("%c ", c);
    }
    printf("\n");
}