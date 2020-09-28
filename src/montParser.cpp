#include "montParser.h"
#include <string>
#include <iostream>

#define PARSEFAIL(str) {ptr->putback(lexer); appendErrorInfo(str, ptr->row, ptr->column); delete ptr; return false;}

using std::string;
using std::ostream;
using std::endl;
using std::cout;

const bool DEBUG = true;
const bool SHOW_ROW_LINE = true;

MontLog MontParser::logger = MontLog();

typedef MontNodePtr Mnp; 

MontNode::~MontNode(){
    for (int i=0;i<children.size();i++) 
        if (children[i]!=nullptr) delete children[i];
    children.clear();
}

void MontNode::putback(MontLexer& lexer){
    int s = children.size();
    for (int i=s-1;i>=0;i--) {
        children[i]->putback(lexer);
        delete children[i];
    }
    children.clear();
}

bool MontNode::appendErrorInfo(string str, int row, int column){
    return MontParser::appendErrorInfo(str, row, column);
}

void MontTokenNode::putback(MontLexer& lexer) {
    lexer.putback(token);
}

void MontNode::addChildren(Mnp p){
    // if (children.size()==0) {row=p->row; column=p->column;}
    children.push_back(p);
}

bool MontNode::tryParse(MontLexer& lexer, TokenKind tk) {
    if (DEBUG) cout << "try parse " << lexer.peek() << endl;
    Token load = lexer.nextToken();
    if (load.tokenKind == tk) {
        if (DEBUG) cout << "ok parsed token " << load << endl;
        addChildren(new MontTokenNode(load)); return true;
    } else {
        lexer.putback(load); return false;
    }
}

bool MontNode::isValueToken(Token& token){
    TokenKind tk = token.tokenKind;
    return (tk==TK_CHAR_VALUE || tk==TK_INT_VALUE);
}

bool MontNode::tryParseValue(MontLexer& lexer) {
    if (DEBUG) cout << "try parse value " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_VALUE;
    Token token = lexer.peek();
    if (isValueToken(token)) ptr->tryParse(lexer, token.tokenKind);
    else PARSEFAIL("Value: Expect value token.");
    if (DEBUG) cout << "ok parsed value" << endl;
    addChildren(ptr); return true;
}

bool MontNode::isUnaryOperatorToken(Token& t){
    return (t.tokenKind == TK_EXCLAMATION ||
        t.tokenKind == TK_TILDE ||
        t.tokenKind == TK_MINUS);
}

bool MontNode::tryParseUnary(MontLexer& lexer) {
    if (DEBUG) cout << "try parse unary " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_UNARY;
    Token token = lexer.peek();
    if (!isUnaryOperatorToken(token)) {
        ptr->expansion = NE_UNARY_VALUE;
        if (!ptr->tryParseValue(lexer)) 
            PARSEFAIL("Unary: Expect value token.");
    } else {
        ptr->expansion = NE_UNARY_OPERATION;
        if (!ptr->tryParse(lexer, token.tokenKind) || !ptr->tryParseUnary(lexer)) 
            PARSEFAIL("Unary: Expect unary.");
    }
    if (DEBUG) cout << "ok parsed unary" << endl;
    addChildren(ptr); return true;
}

bool MontNode::tryParseExpression(MontLexer& lexer) {
    if (DEBUG) cout << "try parse expression " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_EXPRESSION;
    if (!ptr->tryParseUnary(lexer)) PARSEFAIL("Expression: Not valid unary.");
    if (DEBUG) cout << "ok parsed expression" << endl;
    addChildren(ptr); return true;
}

bool MontNode::isTypeToken(Token& token){
    TokenKind tk = token.tokenKind;
    return (tk==TK_CHAR || tk==TK_INT);
}

bool MontNode::tryParseType(MontLexer& lexer) {
    if (DEBUG) cout << "try parse type " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_TYPE;
    Token token = lexer.peek();
    if (isTypeToken(token)) ptr->tryParse(lexer, token.tokenKind);
    else PARSEFAIL("Type: Expect type name.");
    if (DEBUG) cout << "ok parsed type" << endl;
    addChildren(ptr); return true;
}

bool MontNode::tryParseStatement(MontLexer& lexer) {
    if (DEBUG) cout << "try parse statement " << lexer.peek() << endl;
    Token peek = lexer.peek();
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_STATEMENT;
    if (isTypeToken(peek)) {
        ptr->expansion = NE_STATEMENT_VARDEFINE;
        if (!ptr->tryParseType(lexer) || !ptr->tryParse(lexer, TK_IDENTIFIER) || !ptr->tryParse(lexer, TK_SEMICOLON))
            PARSEFAIL("Statement: Illegal variable definition.");
        if (DEBUG) cout << "ok parsed statement vardefine" << endl;
        addChildren(ptr); return true;
    } else if (peek.tokenKind == TK_RETURN) {
        ptr->tryParse(lexer, TK_RETURN); ptr->expansion = NE_STATEMENT_RETURN;
        if (!ptr->tryParseExpression(lexer) || !ptr->tryParse(lexer, TK_SEMICOLON)) 
            PARSEFAIL("Statement: Illegal return statement.");
        if (DEBUG) cout << "ok parsed return statement" << endl;
        addChildren(ptr); return true;
    } else {
        ptr->expansion = NE_STATEMENT_EXPRESSION;
        if (!ptr->tryParseExpression(lexer) || !ptr->tryParse(lexer, TK_SEMICOLON)) 
            PARSEFAIL("Statement: Illegal expresion statement.");
        if (DEBUG) cout << "ok parsed expression statement" << endl;
        addChildren(ptr); return true;
    }
} 

bool MontNode::tryParseCodeblock(MontLexer& lexer){
    if (DEBUG) cout << "try parse codeblock " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_CODEBLOCK;
    if (!ptr->tryParse(lexer, TK_LBRACE)) 
        PARSEFAIL("Codeblock: LBrace expected.");
    while (true) {
        Token peek = lexer.peek();
        if (peek.tokenKind == TK_RBRACE) break;
        if (!ptr->tryParseStatement(lexer)) 
            PARSEFAIL("Codeblock: Illegal statement.");
    }
    if (DEBUG) cout << "ok parsed codeblock" << endl;
    ptr->tryParse(lexer, TK_RBRACE); 
    addChildren(ptr); return true;
}

bool MontNode::tryParseFunction(MontLexer& lexer) {
    if (DEBUG) cout << "try parse function " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_FUNCTION;
    if (!ptr->tryParseType(lexer) || !ptr->tryParse(lexer, TK_IDENTIFIER) ||
        !ptr->tryParse(lexer, TK_LPAREN) || !ptr->tryParse(lexer, TK_RPAREN) ||
        !ptr->tryParseCodeblock(lexer)) 
        PARSEFAIL("Function: Illegal function definition.");
    MontTokenNode* identifierNode = (MontTokenNode*)(ptr->children[1]);
    if (DEBUG) cout << "ok parsed function" << endl;
    addChildren(ptr); return true;
}

bool MontNode::tryParseProgram(MontLexer& lexer) {
    if (DEBUG) cout << "try parse program " << lexer.peek() << endl;
    Mnp ptr = new MontNode(lexer); ptr->kind = NK_PROGRAM;
    if (!ptr->tryParseFunction(lexer) || !ptr->tryParse(lexer, TK_EOF))
        PARSEFAIL("Program: Illegal program.");
    if (DEBUG) cout << "ok parsed program" << endl;
    addChildren(ptr); return true;
}

MontParser::MontParser(){
    program = new MontNode();
    program->setKind(NK_ROOT);
}

bool MontParser::parse(MontLexer& lexer) {
    bool result = program->tryParseProgram(lexer);
    return result;
}

ostream& operator <<(ostream& stream, MontParser& parser) {
    parser.program->output(0, stream);
    return stream;
}

void MontNode::output(int tabcount, ostream& out) {
    string sp = ""; for (int i=0;i<tabcount;i++) sp+="    ";
    out << sp;
    if (kind==NK_TOKEN) {
        MontTokenNode* n = (MontTokenNode*) this;
        out << (*n).getToken() << endl;
        return;
    }
    out << "(" << row << ":" << column << ") ";
    switch (kind) {
        case NK_ROOT:       out << "root"; break;
        case NK_PROGRAM:    out << "program"; break;
        case NK_FUNCTION:   out << "function"; break;
        case NK_CODEBLOCK:  out << "codeblock"; break;
        case NK_STATEMENT:  out << "statement"; break;
        case NK_EXPRESSION: out << "expression"; break;
        case NK_TYPE:       out << "type"; break;
        case NK_VALUE:      out << "value"; break;
        case NK_UNDEFINED:  out << "undefined"; break;
    }
    out << " {" << endl;
    for (auto child : children) 
        child->output(tabcount+1, out);
    out << sp << "}" << endl;
}