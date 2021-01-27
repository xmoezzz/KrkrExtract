%{
/*---------------------------------------------------------------------------*/
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/* tjs.y */
/* TJS2 bison input file */


#include <malloc.h>


#include "tjsInterCodeGen.h"
#include "tjsScriptBlock.h"
#include "tjsError.h"
#include "tjsArray.h"
#include "tjsDictionary.h"

#define YYMALLOC	::malloc
#define YYREALLOC	::realloc
#define YYFREE		::free

/* param */
#define YYPARSE_PARAM pm
#define YYLEX_PARAM pm

/* script block */
#define sb ((tTJSScriptBlock*)pm)

/* current context */
#define cc (sb->GetCurrentContext())

/* current node */
#define cn (cc->GetCurrentNode())

/* lexical analyzer */
#define lx (sb->GetLexicalAnalyzer())


NS_TJS_START

/* yylex/yyerror prototype decl */
#define YYLEX_PROTO_DECL int yylex(YYSTYPE *yylex, void *pm);

int __yyerror(char * msg, void *pm);


#define yyerror(msg) __yyerror(msg, pm);

%}

%pure_parser

%expect 2

%union{
	tjs_int			num;
	tTJSExprNode *		np;
}


%token
	T_COMMA					","
	T_EQUAL					"="
	T_AMPERSANDEQUAL		"&="
	T_VERTLINEEQUAL			"|="
	T_CHEVRONEQUAL			"^="
	T_MINUSEQUAL			"-="
	T_PLUSEQUAL				"+="
	T_PERCENTEQUAL			"%="
	T_SLASHEQUAL			"/="
	T_BACKSLASHEQUAL		"\\="
	T_ASTERISKEQUAL			"*="
	T_LOGICALOREQUAL		"||="
	T_LOGICALANDEQUAL		"&&="
	T_RBITSHIFTEQUAL		">>>="
	T_LARITHSHIFTEQUAL		"<<="
	T_RARITHSHIFTEQUAL		">>="
	T_QUESTION				"?"
	T_LOGICALOR				"||"
	T_LOGICALAND			"&&"
	T_VERTLINE				"|"
	T_CHEVRON				"^"
	T_AMPERSAND				"&"
	T_NOTEQUAL				"!="
	T_EQUALEQUAL			"=="
	T_DISCNOTEQUAL			"!=="
	T_DISCEQUAL				"==="
	T_SWAP					"<->"
	T_LT					"<"
	T_GT					">"
	T_LTOREQUAL				"<="
	T_GTOREQUAL				">="
	T_RARITHSHIFT			">>"
	T_LARITHSHIFT			"<<"
	T_RBITSHIFT				">>>"
	T_PERCENT				"%"
	T_SLASH					"/"
	T_BACKSLASH				"\\"
	T_ASTERISK				"*"
	T_EXCRAMATION			"!"
	T_TILDE					"~"
	T_DECREMENT				"--"
	T_INCREMENT				"++"
	T_NEW					"new"
	T_DELETE				"delete"
	T_TYPEOF				"typeof"
	T_PLUS					"+"
	T_MINUS					"-"
	T_SHARP					"#"
	T_DOLLAR				"$"
	T_ISVALID				"isvalid"
	T_INVALIDATE			"invalidate"
	T_INSTANCEOF			"instanceof"
	T_LPARENTHESIS			"("
	T_DOT					"."
	T_LBRACKET				"["
	T_THIS					"this"
	T_SUPER					"super"
	T_GLOBAL				"global"
	T_RBRACKET				"]"
	T_CLASS					"class"
	T_RPARENTHESIS			")"
	T_COLON					":"
	T_SEMICOLON				";"
	T_LBRACE				"{"
	T_RBRACE				"}"
	T_CONTINUE				"continue"
	T_FUNCTION				"function"
	T_DEBUGGER				"debugger"
	T_DEFAULT				"default"
	T_CASE					"case"
	T_EXTENDS				"extends"
	T_FINALLY				"finally"
	T_PROPERTY				"property"
	T_PRIVATE				"private"
	T_PUBLIC				"public"
	T_PROTECTED				"protected"
	T_STATIC				"static"
	T_RETURN				"return"
	T_BREAK					"break"
	T_EXPORT				"export"
	T_IMPORT				"import"
	T_SWITCH				"switch"
	T_IN					"in"
	T_INCONTEXTOF			"incontextof"
	T_FOR					"for"
	T_WHILE					"while"
	T_DO					"do"
	T_IF					"if"
	T_VAR					"var"
	T_CONST					"const"
	T_ENUM					"enum"
	T_GOTO					"goto"
	T_THROW					"throw"
	T_TRY					"try"
	T_SETTER				"setter"
	T_GETTER				"getter"
	T_ELSE					"else"
	T_CATCH					"catch"
	T_OMIT					"..."
	T_SYNCHRONIZED			"synchronized"
	T_WITH					"with"
	T_INT					"int"
	T_REAL					"real"
	T_STRING				"string"
	T_OCTET					"octet"

	T_FALSE					"false"
	T_NULL					"null"
	T_TRUE					"true"
	T_VOID					"void"
	T_NAN					"NaN"
	T_INFINITY				"Infinity"

	T_UPLUS
	T_UMINUS
	T_EVAL
	T_POSTDECREMENT
	T_POSTINCREMENT
	T_IGNOREPROP
	T_PROPACCESS
	T_ARG
	T_EXPANDARG
	T_INLINEARRAY
	T_ARRAYARG
	T_INLINEDIC
	T_DICELM
	T_WITHDOT

	T_THIS_PROXY
	T_WITHDOT_PROXY

%token <num>		T_CONSTVAL
%token <num>		T_SYMBOL
%token <num>		T_REGEXP
%token <num>		T_VARIANT


%type <np>
	expr comma_expr assign_expr cond_expr logical_or_expr
	logical_and_expr inclusive_or_expr exclusive_or_expr and_expr identical_expr
	compare_expr shift_expr add_sub_expr mul_div_expr mul_div_expr_and_asterisk
	unary_expr incontextof_expr priority_expr factor_expr call_arg call_arg_list
	func_expr_def func_call_expr expr_no_comma inline_array array_elm inline_dic dic_elm
	const_inline_array const_inline_dic

%%


/* the program */
program
	: global_list
;

/* global definitions */
global_list
	:										{ sb->PushContextStack(TJS_W("global"),
												ctTopLevel); }
	  def_list								{ sb->PopContextStack(); }
;

/* definition list */
def_list
	:
	| def_list block_or_statement
	| def_list error ";"					{ if(sb->CompileErrorCount>20)
												YYABORT;
											  else yyerrok; }
;

/* a block or a single statement */
block_or_statement
	: statement
	| block
;

/* a statement */
statement
	: ";"
	| expr ";"								{ cc->CreateExprCode($1); }
	| if
	| if_else
	| while
	| do_while
	| for
	| "break" ";"							{ cc->DoBreak(); }
	| "continue" ";"						{ cc->DoContinue(); }
	| "debugger" ";"						{ cc->DoDebugger(); }
	| variable_def
	| func_def
	| property_def
	| class_def
	| return
	| switch
	| with
	| case
	| try
	| throw
;

/* a block */
block
	: "{"									{ cc->EnterBlock(); }
	  def_list
	  "}"									{ cc->ExitBlock(); }
;

/* a while loop */
while
	: "while"								{ cc->EnterWhileCode(false); }
	  "(" expr ")"							{ cc->CreateWhileExprCode($4, false); }
	  block_or_statement					{ cc->ExitWhileCode(false); }
;

/* a do-while loop */
do_while
	: "do"									{ cc->EnterWhileCode(true); }
	  block_or_statement
	  "while"
	  "(" expr ")"							{ cc->CreateWhileExprCode($6, true); }
	  ";"									{ cc->ExitWhileCode(true); }
;

/* an if statement */
if
	: "if" "("								{ cc->EnterIfCode(); }
	  expr									{ cc->CreateIfExprCode($4); }
	  ")" block_or_statement				{ cc->ExitIfCode(); }
;

/* an if-else statement */
if_else
	: if "else"								{ cc->EnterElseCode(); }
	  block_or_statement					{ cc->ExitElseCode(); }
;

/* a for loop */
for
	: "for" "("
	  for_first_clause ";"
	  for_second_clause ";"
	  for_third_clause ")"
	  block_or_statement					{ cc->ExitForCode(); }
;


/* the first clause of a for statement */
for_first_clause
	: /* empty */							{ cc->EnterForCode(); }
	|										{ cc->EnterForCode(); }
	  variable_def_inner
	| expr									{ cc->EnterForCode();
											  cc->CreateExprCode($1); }
;

/* the second clause of a for statement */
for_second_clause
	: /* empty */							{ cc->CreateForExprCode(NULL); }
	| expr									{ cc->CreateForExprCode($1); }
;

/* the third clause of a for statement */
for_third_clause
	: /* empty */							{ cc->SetForThirdExprCode(NULL); }
	| expr									{ cc->SetForThirdExprCode($1); }
;

/* variable definition */
variable_def
	: variable_def_inner ";"
;

variable_def_inner
	: "var" variable_id_list
	| "const" variable_id_list
		/* const: note that current version does not
		   actually disallow re-assigning new value */
;

/* list for the variable definition */
variable_id_list
	: variable_id
	| variable_id_list "," variable_id
;

/* a variable id and an optional initializer expression */
variable_id
	: T_SYMBOL variable_type				{ cc->AddLocalVariable(
												lx->GetString($1)); }
	| T_SYMBOL variable_type "=" expr_no_comma	{ cc->InitLocalVariable(
											  lx->GetString($1), $4); }
;

/* a variable type. It is not currently effect. Ignore types. */
variable_type
	: /* empty */
	| ":" T_SYMBOL
	| ":" T_VOID
	| ":" T_INT
	| ":" T_REAL
	| ":" T_STRING
	| ":" T_OCTET


/* a function definition */
func_def
	: "function" T_SYMBOL					{ sb->PushContextStack(
												lx->GetString($2),
											  ctFunction);
											  cc->EnterBlock();}
	  func_decl_arg_opt variable_type
	  block									{ cc->ExitBlock(); sb->PopContextStack(); }
;

/* a function expression definition */
func_expr_def
	: "function"							{ sb->PushContextStack(
												TJS_W("(anonymous)"),
											  ctExprFunction);
											  cc->EnterBlock(); }
	  func_decl_arg_opt variable_type
	  block									{ cc->ExitBlock();
											  tTJSVariant v(cc);
											  sb->PopContextStack();
											  $$ = cc->MakeNP0(T_CONSTVAL);
											  $$->SetValue(v); }
;

/* the argument definition of a function definition */
func_decl_arg_opt
	: /* empty */
	| "(" func_decl_arg_collapse ")"
	| "(" func_decl_arg_list ")"
	| "(" func_decl_arg_at_least_one "," func_decl_arg_collapse ")"
;

/* the argument list */
func_decl_arg_list
	: /* empty */
	| func_decl_arg_at_least_one
;

func_decl_arg_at_least_one
	: func_decl_arg
	| func_decl_arg_at_least_one "," func_decl_arg
;

func_decl_arg
	: T_SYMBOL variable_type				{ cc->AddFunctionDeclArg(
												lx->GetString($1), NULL); }
	| T_SYMBOL variable_type "=" expr_no_comma	{ cc->AddFunctionDeclArg(
												lx->GetString($1), $4); }
;

func_decl_arg_collapse
	: "*"									{ cc->AddFunctionDeclArgCollapse(
												NULL); }
	| T_SYMBOL variable_type "*"			{ cc->AddFunctionDeclArgCollapse(
												lx->GetString($1)); }
/*
	These are currently not supported
	| T_SYMBOL "*" "=" inline_array			{ ; }
	| T_SYMBOL "*=" inline_array			{ ; }
*/
;

/* a property handler definition */
property_def
	: "property" T_SYMBOL
	  "{"									{ sb->PushContextStack(
												lx->GetString($2),
												ctProperty); }
	  property_handler_def_list
	  "}"									{ sb->PopContextStack(); }
;

property_handler_def_list
	: property_handler_setter
	| property_handler_getter
	| property_handler_setter property_handler_getter
	| property_handler_getter property_handler_setter
;

property_handler_setter
	: "setter" "(" T_SYMBOL variable_type ")"	{ sb->PushContextStack(
												TJS_W("(setter)"),
												ctPropertySetter);
											  cc->EnterBlock();
											  cc->SetPropertyDeclArg(
												lx->GetString($3)); }
	  block									{ cc->ExitBlock();
											  sb->PopContextStack(); }
;

property_handler_getter
	: property_getter_handler_head			{ sb->PushContextStack(
												TJS_W("(getter)"),
												ctPropertyGetter);
											  cc->EnterBlock(); }
	  block									{ cc->ExitBlock();
											  sb->PopContextStack(); }
;

property_getter_handler_head
	: "getter" "(" ")" variable_type
	| "getter" variable_type
;


/* a class definition */
class_def
	: "class" T_SYMBOL						{ sb->PushContextStack(
												lx->GetString($2),
												ctClass); }
	  class_extender
	  block									{ sb->PopContextStack(); }
;

class_extender
	:
	| "extends" expr_no_comma				{ cc->CreateExtendsExprCode($2, true); }
	| "extends" expr_no_comma ","			{ cc->CreateExtendsExprCode($2, false); }
	  extends_list
;

extends_list
	: extends_name
	| extends_list "," extends_name
;

extends_name
	: expr_no_comma							{ cc->CreateExtendsExprCode($1, false); }
;

/* a return statement */
return
	: "return" ";"							{ cc->ReturnFromFunc(NULL); }
	| "return" expr ";"						{ cc->ReturnFromFunc($2); }
;


/* a switch statement */
switch
	: "switch" "("
	  expr ")"								{ cc->EnterSwitchCode($3); }
	  block									{ cc->ExitSwitchCode(); }
;

/* a with statement */
with
	: "with" "("
	  expr ")"								{ cc->EnterWithCode($3); }
	  block_or_statement					{ cc->ExitWithCode(); }
;

/* case: */
case
	: "case" expr ":"						{ cc->ProcessCaseCode($2); }
	| "default" ":"							{ cc->ProcessCaseCode(NULL); }
;

/* a structured exception handling */
try
	: "try"									{ cc->EnterTryCode(); }
	  block_or_statement
	  catch
	  block_or_statement					{ cc->ExitTryCode(); }
;

catch
	: "catch"								{ cc->EnterCatchCode(NULL); }
	| "catch" "(" ")"						{ cc->EnterCatchCode(NULL); }
	| "catch" "(" T_SYMBOL ")"				{ cc->EnterCatchCode(
												lx->GetString($3)); }
;

/* a throw statement */
throw
	: "throw" expr ";"						{ cc->ProcessThrowCode($2); }
;

/* an expression */
expr_no_comma
	: assign_expr								{ $$ = $1; }
;

expr
	: comma_expr								{ $$ = $1; }
	| comma_expr "if" expr						{ $$ = cc->MakeNP2(T_IF, $1, $3); }
;


comma_expr
	: assign_expr								{ $$ = $1; }
	| comma_expr "," assign_expr				{ $$ = cc->MakeNP2(T_COMMA, $1, $3); }
;


assign_expr
	: cond_expr									{ $$ = $1; }
	| cond_expr "<->" assign_expr				{ $$ = cc->MakeNP2(T_SWAP, $1, $3); }
	| cond_expr "=" assign_expr					{ $$ = cc->MakeNP2(T_EQUAL, $1, $3); }
	| cond_expr "&=" assign_expr				{ $$ = cc->MakeNP2(T_AMPERSANDEQUAL, $1, $3); }
	| cond_expr "|=" assign_expr				{ $$ = cc->MakeNP2(T_VERTLINEEQUAL, $1, $3); }
	| cond_expr "^=" assign_expr				{ $$ = cc->MakeNP2(T_CHEVRONEQUAL, $1, $3); }
	| cond_expr "-=" assign_expr				{ $$ = cc->MakeNP2(T_MINUSEQUAL, $1, $3); }
	| cond_expr "+=" assign_expr				{ $$ = cc->MakeNP2(T_PLUSEQUAL, $1, $3); }
	| cond_expr "%=" assign_expr				{ $$ = cc->MakeNP2(T_PERCENTEQUAL, $1, $3); }
	| cond_expr "/=" assign_expr				{ $$ = cc->MakeNP2(T_SLASHEQUAL, $1, $3); }
	| cond_expr "\\=" assign_expr				{ $$ = cc->MakeNP2(T_BACKSLASHEQUAL, $1, $3); }
	| cond_expr "*=" assign_expr				{ $$ = cc->MakeNP2(T_ASTERISKEQUAL, $1, $3); }
	| cond_expr "||=" assign_expr				{ $$ = cc->MakeNP2(T_LOGICALOREQUAL, $1, $3); }
	| cond_expr "&&=" assign_expr				{ $$ = cc->MakeNP2(T_LOGICALANDEQUAL, $1, $3); }
	| cond_expr ">>=" assign_expr				{ $$ = cc->MakeNP2(T_RARITHSHIFTEQUAL, $1, $3); }
	| cond_expr "<<=" assign_expr				{ $$ = cc->MakeNP2(T_LARITHSHIFTEQUAL, $1, $3); }
	| cond_expr ">>>=" assign_expr				{ $$ = cc->MakeNP2(T_RBITSHIFTEQUAL, $1, $3); }
;


cond_expr
	: logical_or_expr							{ $$ = $1; }
	| logical_or_expr "?"
		cond_expr ":"
		cond_expr								{ $$ = cc->MakeNP3(T_QUESTION, $1, $3, $5); }
;



logical_or_expr
	: logical_and_expr							{ $$ = $1; }
	| logical_or_expr "||" logical_and_expr		{ $$ = cc->MakeNP2(T_LOGICALOR, $1, $3); }
;

logical_and_expr
	: inclusive_or_expr							{ $$ = $1; }
	| logical_and_expr "&&"
	  inclusive_or_expr							{ $$ = cc->MakeNP2(T_LOGICALAND, $1, $3); }
;

inclusive_or_expr
	: exclusive_or_expr							{ $$ = $1; }
	| inclusive_or_expr "|" exclusive_or_expr	{ $$ = cc->MakeNP2(T_VERTLINE, $1, $3); }
;

exclusive_or_expr
	: and_expr									{ $$ = $1; }
	| exclusive_or_expr "^" and_expr			{ $$ = cc->MakeNP2(T_CHEVRON, $1, $3); }
;

and_expr
	: identical_expr							{ $$ = $1; }
	| and_expr "&" identical_expr				{ $$ = cc->MakeNP2(T_AMPERSAND, $1, $3); }
;

identical_expr
	: compare_expr								{ $$ = $1; }
	| identical_expr "!=" compare_expr			{ $$ = cc->MakeNP2(T_NOTEQUAL, $1, $3); }
	| identical_expr "==" compare_expr			{ $$ = cc->MakeNP2(T_EQUALEQUAL, $1, $3); }
	| identical_expr "!==" compare_expr			{ $$ = cc->MakeNP2(T_DISCNOTEQUAL, $1, $3); }
	| identical_expr "===" compare_expr			{ $$ = cc->MakeNP2(T_DISCEQUAL, $1, $3); }
;

compare_expr
	: shift_expr								{ $$ = $1; }
	| compare_expr "<" shift_expr				{ $$ = cc->MakeNP2(T_LT, $1, $3); }
	| compare_expr ">" shift_expr				{ $$ = cc->MakeNP2(T_GT, $1, $3); }
	| compare_expr "<=" shift_expr				{ $$ = cc->MakeNP2(T_LTOREQUAL, $1, $3); }
	| compare_expr ">=" shift_expr				{ $$ = cc->MakeNP2(T_GTOREQUAL, $1, $3); }
;

shift_expr
	: add_sub_expr								{ $$ = $1; }
	| shift_expr ">>" add_sub_expr				{ $$ = cc->MakeNP2(T_RARITHSHIFT, $1, $3); }
	| shift_expr "<<" add_sub_expr				{ $$ = cc->MakeNP2(T_LARITHSHIFT, $1, $3); }
	| shift_expr ">>>" add_sub_expr				{ $$ = cc->MakeNP2(T_RBITSHIFT, $1, $3); }
;


add_sub_expr
	: mul_div_expr								{ $$ = $1; }
	| add_sub_expr "+" mul_div_expr				{ $$ = cc->MakeNP2(T_PLUS, $1, $3); }
	| add_sub_expr "-" mul_div_expr				{ $$ = cc->MakeNP2(T_MINUS, $1, $3); }
;

mul_div_expr
	: unary_expr								{ $$ = $1; }
	| mul_div_expr "%" unary_expr				{ $$ = cc->MakeNP2(T_PERCENT, $1, $3); }
	| mul_div_expr "/" unary_expr				{ $$ = cc->MakeNP2(T_SLASH, $1, $3); }
	| mul_div_expr "\\" unary_expr				{ $$ = cc->MakeNP2(T_BACKSLASH, $1, $3); }
	| mul_div_expr_and_asterisk unary_expr		{ $$ = cc->MakeNP2(T_ASTERISK, $1, $2); }
;

mul_div_expr_and_asterisk
	: mul_div_expr "*"							{ $$ = $1; }
;

unary_expr
	: incontextof_expr							{ $$ = $1; }
	| "!" unary_expr							{ $$ = cc->MakeNP1(T_EXCRAMATION, $2); }
	| "~" unary_expr							{ $$ = cc->MakeNP1(T_TILDE, $2); }
	| "--" unary_expr							{ $$ = cc->MakeNP1(T_DECREMENT, $2); }
	| "++" unary_expr							{ $$ = cc->MakeNP1(T_INCREMENT, $2); }
	| "new" func_call_expr						{ $$ = $2; $$->SetOpecode(T_NEW); }
	| "invalidate" unary_expr					{ $$ = cc->MakeNP1(T_INVALIDATE, $2); }
	| "isvalid" unary_expr						{ $$ = cc->MakeNP1(T_ISVALID, $2); }
	| incontextof_expr "isvalid"				{ $$ = cc->MakeNP1(T_ISVALID, $1); }
	| "delete" unary_expr						{ $$ = cc->MakeNP1(T_DELETE, $2); }
	| "typeof" unary_expr						{ $$ = cc->MakeNP1(T_TYPEOF, $2); }
	| "#" unary_expr							{ $$ = cc->MakeNP1(T_SHARP, $2); }
	| "$" unary_expr							{ $$ = cc->MakeNP1(T_DOLLAR, $2); }
	| "+" unary_expr							{ $$ = cc->MakeNP1(T_UPLUS, $2); }
	| "-" unary_expr							{ $$ = cc->MakeNP1(T_UMINUS, $2); }
	| "&" unary_expr							{ $$ = cc->MakeNP1(T_IGNOREPROP, $2); }
	| "*" unary_expr							{ $$ = cc->MakeNP1(T_PROPACCESS, $2); }
	| incontextof_expr "instanceof" unary_expr	{ $$ = cc->MakeNP2(T_INSTANCEOF, $1, $3); }
	| "(" "int" ")" unary_expr					{ $$ = cc->MakeNP1(T_INT, $4); }
	| "int" unary_expr							{ $$ = cc->MakeNP1(T_INT, $2); }
	| "(" "real" ")" unary_expr					{ $$ = cc->MakeNP1(T_REAL, $4); }
	| "real" unary_expr							{ $$ = cc->MakeNP1(T_REAL, $2); }
	| "(" "string" ")" unary_expr				{ $$ = cc->MakeNP1(T_STRING, $4); }
	| "string" unary_expr						{ $$ = cc->MakeNP1(T_STRING, $2); }
;

incontextof_expr
	: priority_expr								{ $$ = $1; }
	| priority_expr "incontextof"
		incontextof_expr						{ $$ = cc->MakeNP2(T_INCONTEXTOF, $1, $3); }
;

priority_expr
	: factor_expr								{ $$ = $1; }
	| "(" expr ")"								{ $$ = $2; }
	| priority_expr "[" expr "]"				{ $$ = cc->MakeNP2(T_LBRACKET, $1, $3); }
	| func_call_expr							{ $$ = $1; }
	| priority_expr "."							{ lx->SetNextIsBareWord(); }
	  T_SYMBOL									{ tTJSExprNode * node = cc->MakeNP0(T_CONSTVAL);
												  node->SetValue(lx->GetValue($4));
												  $$ = cc->MakeNP2(T_DOT, $1, node); }
	| priority_expr "++"						{ $$ = cc->MakeNP1(T_POSTINCREMENT, $1); }
	| priority_expr "--"						{ $$ = cc->MakeNP1(T_POSTDECREMENT, $1); }
	| priority_expr	"!"							{ $$ = cc->MakeNP1(T_EVAL, $1); }
	| "."										{ lx->SetNextIsBareWord(); }
	  T_SYMBOL								    { tTJSExprNode * node = cc->MakeNP0(T_CONSTVAL);
												  node->SetValue(lx->GetValue($3));
												  $$ = cc->MakeNP1(T_WITHDOT, node); }
;


factor_expr
	: T_CONSTVAL								{ $$ = cc->MakeNP0(T_CONSTVAL);
												  $$->SetValue(lx->GetValue($1)); }
	| T_SYMBOL									{ $$ = cc->MakeNP0(T_SYMBOL);
												  $$->SetValue(tTJSVariant(
													lx->GetString($1))); }
	| "this"									{ $$ = cc->MakeNP0(T_THIS); }
	| "super"									{ $$ = cc->MakeNP0(T_SUPER); }
	| func_expr_def								{ $$ = $1; }
	| "global"									{ $$ = cc->MakeNP0(T_GLOBAL); }
	| "void"									{ $$ = cc->MakeNP0(T_VOID); }
	| inline_array								{ $$ = $1; }
	| inline_dic								{ $$ = $1; }
	| const_inline_array						{ $$ = $1; }
	| const_inline_dic							{ $$ = $1; }
	| "/="										{ lx->SetStartOfRegExp(); }
	  T_REGEXP									{ $$ = cc->MakeNP0(T_REGEXP);
												  $$->SetValue(lx->GetValue($3)); }
	| "/"										{ lx->SetStartOfRegExp(); }
	  T_REGEXP									{ $$ = cc->MakeNP0(T_REGEXP);
												  $$->SetValue(lx->GetValue($3)); }
;


/* an expression for function call */
func_call_expr
	: priority_expr "(" call_arg_list ")"		{ $$ = cc->MakeNP2(T_LPARENTHESIS, $1, $3); }
;

/* argument(s) for function call */
call_arg_list
	: "..."								{ $$ = cc->MakeNP0(T_OMIT); }
	| call_arg							{ $$ = cc->MakeNP1(T_ARG, $1); }
	| call_arg_list "," call_arg		{ $$ = cc->MakeNP2(T_ARG, $3, $1); }
;

call_arg
	: /* empty */						{ $$ = NULL; }
	| "*"								{ $$ = cc->MakeNP1(T_EXPANDARG, NULL); }
	| mul_div_expr_and_asterisk			{ $$ = cc->MakeNP1(T_EXPANDARG, $1); }
	| expr_no_comma						{ $$ = $1; }
;


/* an inline array object */
inline_array
	: "[" 								{ tTJSExprNode *node =
										  cc->MakeNP0(T_INLINEARRAY);
										  cc->PushCurrentNode(node); }
	  array_elm_list
	  "]"								{ $$ = cn; cc->PopCurrentNode(); }
;

/* an inline array's element list */
array_elm_list
	: array_elm							{ cn->Add($1); }
	| array_elm_list "," array_elm		{ cn->Add($3); }
;

/* an inline array's element */
array_elm
	: /* empty */						{ $$ = cc->MakeNP1(T_ARRAYARG, NULL); }
	| expr_no_comma						{ $$ = cc->MakeNP1(T_ARRAYARG, $1); }
;

/* an inline dictionary */
inline_dic
	: "%" "["							{ tTJSExprNode *node =
										  cc->MakeNP0(T_INLINEDIC);
										  cc->PushCurrentNode(node); }
	  dic_elm_list
	  dic_dummy_elm_opt
	  "]"								{ $$ = cn; cc->PopCurrentNode(); }
;


/* an inline dictionary's element list */
dic_elm_list
    : /* empty */
	| dic_elm							{ cn->Add($1); }
	| dic_elm_list "," dic_elm			{ cn->Add($3); }
;

/* an inline dictionary's element */
dic_elm
	: expr_no_comma "," expr_no_comma	{ $$ = cc->MakeNP2(T_DICELM, $1, $3); }
	| T_SYMBOL ":" expr_no_comma		{ tTJSVariant val(lx->GetString($1));
										  tTJSExprNode *node0 = cc->MakeNP0(T_CONSTVAL);
										  node0->SetValue(val);
										  $$ = cc->MakeNP2(T_DICELM, node0, $3); }
;

/* a dummy element at the tail of inline dictionary elements */
dic_dummy_elm_opt
	: /* empty */
	| ","
;



/* a constant inline array object */
const_inline_array
	: "(" "const" ")" "[" 				{ tTJSExprNode *node =
										  cc->MakeNP0(T_CONSTVAL);
										  iTJSDispatch2 * dsp = TJSCreateArrayObject();
										  node->SetValue(tTJSVariant(dsp, dsp));
										  dsp->Release();
										  cc->PushCurrentNode(node); }
	  const_array_elm_list_opt
	  "]"								{ $$ = cn; cc->PopCurrentNode(); }
;

const_array_elm_list_opt
	: /* empty */
	| const_array_elm_list
;


/* a constant inline array's element list */
const_array_elm_list
	: const_array_elm
	| const_array_elm_list ","
	  const_array_elm
;

/* a constant inline array's element */
const_array_elm
	: "-" T_CONSTVAL					{ cn->AddArrayElement(- lx->GetValue($2)); }
	| "+" T_CONSTVAL					{ cn->AddArrayElement(+ lx->GetValue($2)); }
	| T_CONSTVAL						{ cn->AddArrayElement(lx->GetValue($1)); }
	| "void"							{ cn->AddArrayElement(tTJSVariant());  }
	| const_inline_array				{ cn->AddArrayElement($1->GetValue()); }
	| const_inline_dic					{ cn->AddArrayElement($1->GetValue()); }
;

/* a constant inline dictionary */
const_inline_dic
	: "(" "const" ")" "%" "["			{ tTJSExprNode *node =
										  cc->MakeNP0(T_CONSTVAL);
										  iTJSDispatch2 * dsp = TJSCreateDictionaryObject();
										  node->SetValue(tTJSVariant(dsp, dsp));
										  dsp->Release();
										  cc->PushCurrentNode(node); }
	  const_dic_elm_list
	  "]"								{ $$ = cn; cc->PopCurrentNode(); }
;


/* a constant inline dictionary's element list */
const_dic_elm_list
    : /* empty */
	| const_dic_elm
	| const_dic_elm_list "," const_dic_elm
;

/* a constant inline dictionary's element */
const_dic_elm
	: T_CONSTVAL "," "-" T_CONSTVAL		{ cn->AddDictionaryElement(lx->GetValue($1), - lx->GetValue($4)); }
	| T_CONSTVAL "," "+" T_CONSTVAL		{ cn->AddDictionaryElement(lx->GetValue($1), + lx->GetValue($4)); }
	| T_CONSTVAL "," T_CONSTVAL			{ cn->AddDictionaryElement(lx->GetValue($1), lx->GetValue($3)); }
	| T_CONSTVAL "," "void"				{ cn->AddDictionaryElement(lx->GetValue($1), tTJSVariant()); }
	| T_CONSTVAL "," const_inline_array	{ cn->AddDictionaryElement(lx->GetValue($1), $3->GetValue()); }
	| T_CONSTVAL "," const_inline_dic	{ cn->AddDictionaryElement(lx->GetValue($1), $3->GetValue()); }
;



%%
