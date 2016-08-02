%{
#pragma warning(disable: 4005)
#include <string>

#include "grammar.hpp"

#define YY_USER_ACTION loc.columns(yyleng);

#define YY_DECL yy::parser::symbol_type yylex(yyscan_t yyscanner, yy::location &loc)

#define MAKE(x) yy::parser::make_ ## x(loc)
#define MAKE_VALUE(x, v) yy::parser::make_ ## x((v), loc)
%}

%option nounistd
%option yylineno
%option nounput
%option batch
%option never-interactive
%option reentrant
%option noyywrap


identifier_old_no_integer [_a-zA-Z][_a-zA-Z0-9]*

identifier      [_a-zA-Z0-9]+
quote1          "\'"[^'\\]*"\'"
quote2          "\""[^"\\]*"\""


%%

%{
    // Code run each time yylex is called.
    loc.step();
%}

#.*/\n                  ; // ignore comments

[ \t]+                  loc.step();
\r                      loc.step();
\n                      {
                            loc.lines(yyleng);
                            loc.step();
                        }

";"                     return MAKE(SEMICOLON);
":"                     return MAKE(COLON);
"("                     return MAKE(L_BRACKET);
")"                     return MAKE(R_BRACKET);
"{"                     return MAKE(L_CURLY_BRACKET);
"}"                     return MAKE(R_CURLY_BRACKET);
"["                     return MAKE(L_SQUARE_BRACKET);
"]"                     return MAKE(R_SQUARE_BRACKET);
","                     return MAKE(COMMA);
"\."                    return MAKE(POINT);
"\+"                    return MAKE(PLUS);
"->"                    return MAKE(R_ARROW);
"="                     return MAKE(EQUAL);

and|elif|global|or|assert|else|if|except|pass|break|import|print|exec|in|raise|continue|finally|is|return|for|lambda|try|del|from|not|while return MAKE_VALUE(KEYWORD, yytext);
class					return MAKE(CLASS);

{identifier}			return MAKE_VALUE(ID, yytext);

{quote1}				|
{quote2}				return MAKE_VALUE(STRING, yytext);

.                       { /*driver.error(loc, "invalid character");*/ return MAKE(ERROR_SYMBOL); }
<<EOF>>                 return MAKE(EOQ);

%%
