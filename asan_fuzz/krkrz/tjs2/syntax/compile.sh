#!/bin/sh
alias copy=cp
echo tjs.y :
perl CR.pl tjs.y
bison -dv tjs.y
perl postbison.pl tjs.tab.c
perl wrapTJSns.pl tjs.tab.h
echo copying ...
echo tjspp.y :
perl CR.pl tjspp.y
bison -dv --name-prefix=pp tjspp.y
perl postbison.pl tjspp.tab.c
perl wrapTJSns.pl tjspp.tab.h
echo copying ...
copy tjs.tab.c ../tjs.tab.cpp
copy tjs.tab.h ../tjs.tab.h
copy tjspp.tab.c ../tjspp.tab.cpp
copy tjspp.tab.h ../tjspp.tab.h
echo tjsdate.y :
perl CR.pl tjsdate.y
bison -dv --name-prefix=dp tjsdate.y
perl postbison.pl tjsdate.tab.c
perl wrapTJSns.pl tjsdate.tab.h
echo copying ...
copy tjsdate.tab.c ../tjsdate.tab.cpp
copy tjsdate.tab.h ../tjsdate.tab.h
perl create_word_map.pl > ..\tjsDateWordMap.cc

