open(FH, $ARGV[0]);


@all = <FH>;


$all = join('', @all);


$all =~ s/\bchar\b/wchar_t/g;
$all =~ s/#include <unistd\.h>//g;
$all =~ s/\bunsigned wchar_t\b/wchar_t/g;
$all =~ s/\bisatty\b/std::isatty/g;
$all =~ s/yy_fatal_error\( msg \)/yy_fatal_error( L##msg )/g;
$all =~ s/\#line (\d+) \"tjs.yy.c\"/\#line $1 \"tjs.yy.cpp\"/g;
$all =~ s/\#line (\d+) \"tjs.l\"/\#line $1 \"..\\bison-flex\\tjs.l\"/g;


open(FH,">".$ARGV[0]);
print FH "#include \"tjsCommHead.h\"\n";

print FH $all;



