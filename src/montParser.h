#ifndef MONTPARSER_H
#define MONTPARSER_H

#include <vector>
#include <string>
#include <iostream>
#include "montLexer.h"
#include "montLog.h"
#include <assert.h>

using std::vector;
using std::string;

class MontNode;
typedef MontNode* MontNodePtr;
class MontConceiver;

enum MontNodeKind {
    NK_ROOT,
    NK_PROGRAM,
    NK_FUNCTION,
    NK_CODEBLOCK,
    NK_STATEMENT, 
    NK_EXPRESSION,
    NK_LOGICAL_OR,
    NK_LOGICAL_AND,
    NK_EQUALITY,
    NK_RELATIONAL,
    NK_ADDITIVE,
    NK_MULTIPLICATIVE,
    NK_PRIMARY, 
    NK_UNARY, 
    NK_TYPE,
    NK_VALUE,
    NK_TOKEN,
    NK_DECLARATION, 
    NK_ASSIGNMENT, 
    NK_BLOCKITEM,
    NK_IF, 
    NK_CONDITIONAL, 
    NK_FOR,
    NK_WHILE,
    NK_PARAMETERS,
    NK_EXPRLIST,
    NK_POSTFIX,
    NK_UNDEFINED,
    NK_GLOBDECL, 
    NK_EMPTY, // 占位符，例如 for 的 expr 可能为空。
};

enum MontNodeExpansion {
    NE_NONE,                  // 0
    NE_STATEMENT_EXPRESSION,
    NE_STATEMENT_RETURN,
    NE_STATEMENT_CODEBLOCK,
    NE_STATEMENT_EMPTY,
    NE_STATEMENT_IF, 
    NE_STATEMENT_FOR,
    NE_STATEMENT_WHILE,
    NE_STATEMENT_BREAK,
    NE_STATEMENT_CONTINUE,
    NE_UNARY_POSTFIX,        // 10
    NE_UNARY_OPERATION,
    NE_UNARY_CAST,
    NE_PRIMARY_VALUE,
    NE_PRIMARY_PAREN,
    NE_PRIMARY_IDENTIFIER,
    NE_MULTIPLICATIVE_LEAF,
    NE_MULTIPLICATIVE_INNER,
    NE_ADDITIVE_LEAF,
    NE_ADDITIVE_INNER,
    NE_LOR_LEAF,             // 20
    NE_LOR_INNER,
    NE_LAND_LEAF,
    NE_LAND_INNER,
    NE_EQUALITY_INNER,
    NE_EQUALITY_LEAF,
    NE_RELATIONAL_LEAF,
    NE_RELATIONAL_INNER,
    NE_DECLARATION_SIMPLE,
    NE_DECLARATION_INIT,
    NE_DECLARATION_ARRAY,
    NE_GLOBDECL_SIMPLE,
    NE_GLOBDECL_INIT,
    NE_GLOBDECL_ARRAY,
    NE_ASSIGNMENT_VALUE,     
    NE_ASSIGNMENT_ASSIGN,
    NE_BLOCKITEM_STATEMENT,
    NE_BLOCKITEM_DECLARATION,
    NE_IF_SIMPLE,
    NE_IF_ELSE,
    NE_CONDITIONAL_LEAF,
    NE_CONDITIONAL_INNER,
    NE_FOR_EXPRESSION,
    NE_FOR_DECLARATION,
    NE_WHILE_STANDARD,        
    NE_WHILE_DO,
    NE_FUNCTION_DECLARATION,
    NE_FUNCTION_DEFINITION,
    NE_POSTFIX_PRIMARY,
    NE_POSTFIX_CALL,
    NE_POSTFIX_ARRAY, 
    NE_TYPE_BASIC,
    NE_TYPE_POINTER
};

enum MontBasicType {
    DT_VOID,
    DT_INT,
    DT_CHAR,
    DT_BOOL,
    DT_POINTER,
    DT_ARRAY, 
};

struct MontType {
    MontBasicType basic;
    MontType* pointer;
    bool lvalue; 
    int size;
    int arraylength;
    MontType(MontBasicType basic) : basic(basic), pointer(nullptr), lvalue(false) {
        if (basic == DT_VOID) size = 0;
        else size = 4;
    }
    MontType(): basic(DT_VOID), pointer(nullptr), lvalue(false) {size=0;}
    MontType(MontType* ptr): pointer(ptr), lvalue(false) {
        basic = DT_POINTER; size=4; arraylength = 0;
    }
    MontType(MontType* ptr, int length): basic(DT_ARRAY), pointer(ptr), lvalue(false), arraylength(length) {
        size = ptr->size * length;
    }
    MontType(const MontType& r) {
        basic = r.basic;
        lvalue = r.lvalue;
        size = r.size;
        arraylength = r.arraylength;
        if (r.pointer != nullptr) 
            pointer = new MontType(*r.pointer);
        else pointer = nullptr;
    }
    ~MontType(){if (pointer) delete pointer; pointer=nullptr;}
    friend std::ostream& operator << (std::ostream& out, MontType& type);
    MontType& operator =(const MontType& r){
        basic = r.basic;
        lvalue = r.lvalue;
        size = r.size;
        arraylength = r.arraylength;
        if (r.pointer != nullptr) 
            pointer = new MontType(*r.pointer);
        else pointer = nullptr;
        return *this;
    }
    // 不考虑左值与否
    bool operator ==(const MontType& r){
        if (basic == DT_POINTER || r.basic == DT_POINTER) 
            return (basic == DT_POINTER) && (r.basic==DT_POINTER) && (*pointer == *r.pointer);
        if (basic == DT_ARRAY || r.basic == DT_ARRAY) 
            return (basic == DT_ARRAY) && (r.basic==DT_ARRAY) && (*pointer == *r.pointer) && (arraylength == r.arraylength);
        if (basic == DT_VOID && r.basic != DT_VOID) return false;
        if (basic != DT_VOID && r.basic == DT_VOID) return false;
        return true;
    }
    bool operator !=(const MontType& r){
        return !(*this==r);
    }
    void setLvalue(bool l){lvalue=l;}
    bool isPointer(){return basic == DT_POINTER;}
    bool isVoid(){return basic == DT_VOID;}
    bool isBool(){return basic == DT_BOOL;}
    bool isInt(){return basic == DT_INT || basic == DT_CHAR;}
    bool isArray(){return basic == DT_ARRAY;}
    bool isBroadptr(){return basic == DT_ARRAY || basic == DT_POINTER; }
};

class MontNode {
    friend class MontConceiver;
protected:
    MontNodeKind kind;
    MontNodeExpansion expansion;
    vector<MontNodePtr> children;
    MontType datatype; // 在conceiver中得到它的值。
    // static string errorInfo;
    int row, column;
    // 用于生成栈帧，表示该树节点对应代码中所需要声明局部变量的大小（一定是4的倍数），
    // 例如 if () {A} else {B} 中 memorySize 应当是 A B 中所声明局部变量空间中的较大值。
    int memorySize; 
    static bool isValueInteger(MontNode* ptr, int* size);
public:
    void setLvalue(bool f){datatype.setLvalue(f);}
    bool isLvalue(){return datatype.lvalue;}
    MontNode(MontLexer& lexer){
        children = vector<MontNodePtr>();kind = NK_UNDEFINED;expansion = NE_NONE;
        lexer.peek(); row = lexer.getCurrentRow(); column = lexer.getCurrentColumn();
        memorySize = 0; datatype = DT_VOID;
    }
    MontNode(){
        children = vector<MontNodePtr>();kind = NK_UNDEFINED;expansion = NE_NONE;
        row = column = 0; memorySize = 0; datatype = DT_VOID;
        //lexer.peek(); row = lexer.getCurrentRow(); column = lexer.getCurrentColumn();
    }
    static void tryStart();
    static void tryEnd();
    void copyRC(MontNode& from) {row=from.row; column=from.column;}
    MontNodeKind getKind(){return kind;}
    void setKind(MontNodeKind k){kind=k;}
    virtual ~MontNode();
    virtual void putback(MontLexer& lexer);
    static bool appendErrorInfo(string str, int row, int column);
    //void trySetRC(MontLexer& lexer){lexer.peek(); if (children.size()==0) row=lexer.getCurrentRow(), column=lexer.getCurrentColumn();}
    void addChildren(MontNodePtr ptr); 
    bool tryParse(MontLexer& lexer, TokenKind tk);
    bool tryParseEmpty(MontLexer& lexer);
    static bool isValueToken(Token& t);
    bool tryParseValue(MontLexer& lexer);
    static bool isUnaryOperatorToken(Token& t);
    bool tryParseUnary(MontLexer& lexer);
    static bool isEqualityOperatorToken(Token& t);
    static bool isRelationalOperatorToken(Token& t);
    bool tryParseEquality(MontLexer& lexer);
    bool tryParseRelational(MontLexer& lexer);
    bool tryParseLogicalOr(MontLexer& lexer);
    bool tryParseLogicalAnd(MontLexer& lexer);
    static bool isAdditiveOperatorToken(Token& t);
    bool tryParseAdditive(MontLexer& lexer);
    static bool isMultiplicativeOperatorToken(Token& t);
    bool tryParseMultiplicative(MontLexer& lexer);
    bool tryParsePrimary(MontLexer& lexer);
    bool tryParseExpression(MontLexer& lexer); 
    static bool isTypeToken(Token& t);
    bool tryParseType(MontLexer& lexer);
    bool tryParseStatement(MontLexer& lexer);
    bool tryParseCodeblock(MontLexer& lexer);
    bool tryParseFunction(MontLexer& lexer); 
    bool tryParseProgram(MontLexer& lexer);
    bool tryParseDeclaration(MontLexer& lexer);
    bool tryParseAssignment(MontLexer& lexer);
    bool tryParseBlockitem(MontLexer& lexer);
    bool tryParseIf(MontLexer& lexer);
    bool tryParseConditional(MontLexer& lexer);
    bool tryParseFor(MontLexer& lexer);
    bool tryParseWhile(MontLexer& lexer);
    bool tryParseParameters(MontLexer& lexer);
    bool tryParseExprlist(MontLexer& lexer);
    bool tryParsePostfix(MontLexer& lexer);
    bool tryParseGlobdecl(MontLexer& lexer);
    void output(string tab, bool lastchild, std::ostream& stream);
};

// store nothing in children.
class MontTokenNode : public MontNode {
private: 
    TokenKind requirement;
    Token token;
public:
    MontTokenNode(Token tk){
        requirement = tk.tokenKind; token = tk; kind = NK_TOKEN;
        row = tk.row; column = tk.column; memorySize = 0;
    }
    void putback(MontLexer& lexer) override;
    Token getToken(){return token;}
};

class MontParser {
    friend class MontConceiver;
private:
    static MontLog logger; 
    MontNodePtr program;
public:
    static bool outputType; 
    MontParser();
    bool parse(MontLexer& lexer);
    friend std::ostream& operator << (std::ostream& stream, MontParser& parser);
    static bool appendErrorInfo(string str, int row, int column){
        logger.log("(" + std::to_string(row) + ":" + std::to_string(column) + ") " 
            + str); 
        return false;
    }
    static string getErrorInfo(){return logger.get();}
    static void resetErrorInfo(){logger.clear();}
    static void tryStart(){logger.trystart();}
    static void tryEnd(){logger.tryend();}
};

#endif