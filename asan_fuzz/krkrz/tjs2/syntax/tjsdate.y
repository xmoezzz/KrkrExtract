%{
/*---------------------------------------------------------------------------*/
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/* tjsdate.y */
/* TJS2 date string parser */

#include <malloc.h>

#include "tjsTypes.h"
#include "tjsDateParser.h"

#define YYMALLOC	::malloc
#define YYREALLOC	::realloc
#define YYFREE		::free


/* param */
#define YYPARSE_PARAM pm
#define YYLEX_PARAM pm

NS_TJS_START

/* yylex/yyerror prototype decl */
int dplex(YYSTYPE *yylex, void *pm);

int dperror(char * msg)
{
    return 0;
}


#define dp ((tTJSDateParser*)pm)

%}

%pure_parser

%union
{
	tjs_int32					val;
}


%token				DP_AM DP_PM
%token <val>		DP_NUMBER
%token <val>		DP_MONTH
%token <val>		DP_WDAY
%token <val>		DP_TZ


%%

/*---------------------------------------------------------------------------*/
/* input                                                                     */
/*---------------------------------------------------------------------------*/



input
	: date_time_string
;


/*---------------------------------------------------------------------------*/
/* rules                                                                     */
/*---------------------------------------------------------------------------*/

date_time_string
	/* Sun, 3 May 2004 11:22:33 GMT +900 (JST) */
	:	wday_omittable
		DP_NUMBER
		month DP_NUMBER
		time
		tz_omittable									{	dp->SetMDay($2);
															dp->SetYear($4); }
	/* Sun, 3-May 2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER '-'
		month DP_NUMBER
		time
		tz_omittable									{	dp->SetMDay($2);
															dp->SetYear($5); }
	/* Sun, 3-May-2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER '-'
		month '-' DP_NUMBER
		time
		tz_omittable									{	dp->SetMDay($2);
															dp->SetYear($6); }
	/* Sun, May 3 2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		month
		DP_NUMBER DP_NUMBER
		time
		tz_omittable									{	dp->SetMDay($3);
															dp->SetYear($4); }
	/* Sun, May-3 2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		month '-'
		DP_NUMBER DP_NUMBER
		time
		tz_omittable									{	dp->SetMDay($4);
															dp->SetYear($5); }
	/* Sun, May-3-2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		month '-'
		DP_NUMBER '-' DP_NUMBER
		time
		tz_omittable									{	dp->SetMDay($4);
															dp->SetYear($6); }
	/* Sun, 3 May 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER
		month
		time
		DP_NUMBER
		tz_omittable									{	dp->SetMDay($2);
															dp->SetYear($5); }
	/* Sun, 3-May 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER '-'
		month
		time
		DP_NUMBER
		tz_omittable									{	dp->SetMDay($2);
															dp->SetYear($6); }
	/* Sun, May 3 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		month DP_NUMBER
		time
		DP_NUMBER
		tz_omittable									{	dp->SetMDay($3);
															dp->SetYear($5); }
	/* Sun, May-3 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		month '-' DP_NUMBER
		time
		DP_NUMBER
		tz_omittable									{	dp->SetMDay($4);
															dp->SetYear($6); }
	/* 2004/03/03 11:22:33 */
	|	wday_omittable
		DP_NUMBER hyphen_or_slash
		DP_NUMBER hyphen_or_slash
		DP_NUMBER
		time
		tz_omittable									{	dp->SetMonth($4-1);
															dp->SetYear($2);
															dp->SetMDay($6); }
;

wday
	:	DP_WDAY
;

wday_omittable
	:	wday ','
	|	wday
	|	/* empty */
;


month
	:	DP_MONTH										{ dp->SetMonth($1); }
;

hyphen_or_slash
	:	'-'
	|	'/'
;


time_sub_sec_omittable
	:	'.' DP_NUMBER									{ /* TODO: sub-seconds support */ }
	|	/* empty */
;

time_hms
	:	DP_NUMBER ':'
		DP_NUMBER ':'
		DP_NUMBER time_sub_sec_omittable
		{
			dp->SetHours($1);
			dp->SetMin($3);
			dp->SetSec($5);
		}
	|	DP_NUMBER ':'
		DP_NUMBER
		{
			dp->SetHours($1);
			dp->SetMin($3);
			dp->SetSec(0);
		}
;

am_or_pm
	:	DP_AM											{ dp->SetAMPM(false); }
	|	DP_PM											{ dp->SetAMPM(true); }
;

time
	:	time_hms
	|	am_or_pm time_hms
	|	time_hms am_or_pm
;

tz_name_omittable
	:	DP_TZ											{ dp->SetTimeZone($1); }
	|	/* empty */
;

tz_offset_omittable
	:	'+' DP_NUMBER									{ dp->SetTimeZoneOffset($2); }
	|	'-' DP_NUMBER									{ dp->SetTimeZoneOffset(-$2); }
	|	/* empty */
;



tz_desc_omittable
	:	'('												{ dp->SkipToRightParenthesis(); }
		')'
	|	/* empty */
;

tz_omittable
	:	tz_name_omittable
		tz_offset_omittable
		tz_desc_omittable
;

/*---------------------------------------------------------------------------*/
%%
