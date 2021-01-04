%{
/*---------------------------------------------------------------------------*/
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/* tjspp.y */
/* TJS2 conditional compiling control's conditional expression parser */

#include <malloc.h>

#include "tjs.h"
#include "tjsCompileControl.h"

#define YYMALLOC	::malloc
#define YYREALLOC	::realloc
#define YYFREE		::free


/* param */
#define YYPARSE_PARAM pm
#define YYLEX_PARAM pm

NS_TJS_START

/* yylex/yyerror prototype decl */
#define YYLEX_PROTO_DECL int pplex(YYSTYPE *yylex, void *pm);

int _pperror(char * msg, void *pm)
{
	(void)msg;
	(void)pm;
    return 0;
}

#define pperror(msg) _pperror(msg, pm);


#define tjs (((tTJSPPExprParser*)pm)->GetTJS())
#define ep ((tTJSPPExprParser*)pm)
%}

%pure_parser

%union
{
	tjs_int32		val;
	tjs_int			nv;
}


%token
	PT_LPARENTHESIS				"("
	PT_RPARENTHESIS				")"
	PT_ERROR

	PT_COMMA					","
	PT_EQUAL					"="
	PT_NOTEQUAL					"!="
	PT_EQUALEQUAL				"=="
	PT_LOGICALOR				"||"
	PT_LOGICALAND				"&&"
	PT_VERTLINE					"|"
	PT_CHEVRON					"^"
	PT_AMPERSAND				"&"
	PT_LT						"<"
	PT_GT						">"
	PT_LTOREQUAL				"<="
	PT_GTOREQUAL				">="
	PT_PLUS						"+"
	PT_MINUS					"-"
	PT_ASTERISK					"*"
	PT_SLASH					"/"
	PT_PERCENT					"%"
	PT_EXCLAMATION				"!"

	PT_UN

%token <nv>		PT_SYMBOL
%token <val>	PT_NUM

%type <val>		expr


%left	","
%left	"||"
%left	"&&"
%left	"|"
%left	"^"
%left	"&"
%left   "="
%left	"!=" "=="
%left	"<" ">" "<=" ">="
%left	"+" "-"
%left	"%" "/" "*"
%right	"!" PT_UN

%%

input
	: expr						{ ep->Result = $1; }
;

expr
	: expr "," expr				{ $$ = $3; }
	| PT_SYMBOL "=" expr		{ tjs->SetPPValue(ep->GetString($1), $3); $$ = $3; }
	| expr "!="	expr			{ $$ = $1 != $3; }
	| expr "==" expr			{ $$ = $1 == $3; }
	| expr "||" expr			{ $$ = $1 || $3; }
	| expr "&&" expr			{ $$ = $1 && $3; }
	| expr "|" expr				{ $$ = $1 | $3; }
	| expr "^" expr				{ $$ = $1 ^ $3; }
	| expr "&" expr				{ $$ = $1 & $3; }
	| expr "<" expr				{ $$ = $1 < $3; }
	| expr ">" expr				{ $$ = $1 > $3; }
	| expr ">=" expr			{ $$ = $1 >= $3; }
	| expr "<=" expr			{ $$ = $1 <= $3; }
	| expr "+" expr				{ $$ = $1 + $3; }
	| expr "-" expr				{ $$ = $1 - $3; }
	| expr "%" expr				{ $$ = $1 % $3; }
    | expr "*" expr				{ $$ = $1 * $3; }
	| expr "/" expr				{ if($3==0) { YYABORT; } else { $$ = $1 / $3; } }
	| "!" expr					{ $$ = ! $2; }
	| "+" expr %prec PT_UN		{ $$ = + $2; }
    | "-" expr %prec PT_UN		{ $$ = - $2; }
	| "(" expr ")"				{ $$ = $2; }
	| PT_NUM					{ $$ = $1; }
	| PT_SYMBOL					{ $$ = tjs->GetPPValue(ep->GetString($1)); }
;

%%
