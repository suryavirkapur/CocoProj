#ifndef LEX_DEF_
#define LEX_DEF_

typedef enum TokenName {
    TK_ASSIGNOP,    // <---
    TK_COMMENT,     // %
    TK_FIELDID,     // [a-z][a-z]*
    TK_ID,          // [b-d][2-7][b-d]*[2-7]*
    TK_NUM,         // [0-9][0-9]*
    TK_RNUM,        // Real numbers (both formats)
    TK_FUNID,       // _[a-z|A-Z][a-z|A-Z]*[0-9]*
    TK_RUID,        // #[a-z][a-z]*
    TK_WITH,        // with
    TK_PARAMETERS,  // parameters
    TK_END,         // end
    TK_WHILE,       // while
    TK_UNION,       // union
    TK_ENDUNION,    // endunion
    TK_DEFINETYPE,  // definetype
    TK_AS,          // as
    TK_TYPE,        // type
    TK_MAIN,        // _main
    TK_GLOBAL,      // global
    TK_PARAMETER,   // parameter
    TK_LIST,        // list
    TK_SQL,         // [
    TK_SQR,         // ]
    TK_INPUT,       // input
    TK_OUTPUT,      // output
    TK_INT,         // int
    TK_REAL,        // real
    TK_COMMA,       // ,
    TK_SEM,         // ;
    TK_COLON,       // :
    TK_DOT,         // .
    TK_ENDWHILE,    // endwhile
    TK_OP,          // (
    TK_CL,          // )
    TK_IF,          // if
    TK_THEN,        // then
    TK_ENDIF,       // endif
    TK_READ,        // read
    TK_WRITE,       // write
    TK_RETURN,      // return
    TK_PLUS,        // +
    TK_MINUS,       // -
    TK_MUL,         // *
    TK_DIV,         // /
    TK_CALL,        // call
    TK_RECORD,      // record
    TK_ENDRECORD,   // endrecord
    TK_ELSE,        // else
    TK_AND,         // &&&
    TK_OR,          // @@@
    TK_NOT,         // ~
    TK_LT,          // 
    TK_LE,          // <=
    TK_EQ,          // ==
    TK_GT,          // >
    TK_GE,          // >=
    TK_NE,          // !=
    TK_ERR,         // Error token
    TK_EPS,         // Epsilon (empty production)
    TK_DOLLAR,      // End marker for parsing stack
    TK_EOF          // End of file marker
} TokenName;

typedef union Value {
    int INT_VALUE;
    float FLOAT_VALUE;
} Value;

typedef struct Token {
    TokenName TOKEN_NAME;
    char* LEXEME;
    int LINE_NO;
    int IS_NUMBER;
    Value* VALUE;
} Token;

#endif