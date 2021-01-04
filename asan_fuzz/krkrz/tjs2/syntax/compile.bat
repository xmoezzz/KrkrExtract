echo tjs.y :
perl CR.pl tjs.y
bison -dv -o tjs.tab.cpp tjs.y
perl postbison.pl tjs.tab.cpp
perl wrapTJSns.pl tjs.tab.hpp
echo copying ...
echo tjspp.y :
perl CR.pl tjspp.y
bison -dv --name-prefix=pp -o tjspp.tab.cpp tjspp.y
perl postbison.pl tjspp.tab.cpp
perl wrapTJSns.pl tjspp.tab.hpp
echo copying ...
copy tjs.tab.cpp ..\tjs.tab.cpp
copy tjs.tab.hpp ..\tjs.tab.hpp
copy tjspp.tab.cpp ..\tjspp.tab.cpp
copy tjspp.tab.hpp ..\tjspp.tab.hpp
echo tjsdate.y :
perl CR.pl tjsdate.y
bison -dv --name-prefix=dp -o tjsdate.tab.cpp tjsdate.y
perl postbison.pl tjsdate.tab.cpp
perl wrapTJSns.pl tjsdate.tab.hpp
echo copying ...
copy tjsdate.tab.cpp ..\tjsdate.tab.cpp
copy tjsdate.tab.hpp ..\tjsdate.tab.hpp
call gen_wordtable.bat
del *.cpp
del *.hpp
del *.output
