open(FH, $ARGV[0]);

binmode FH;

@all = <FH>;


$all = join('', @all);


if($all =~ /\r/)
{
	print "unifying the return code ...\n";
	$all =~ s/\r\n/\n/g;
	$all =~ s/\r/\n/g;
	open(FH,">".$ARGV[0]);
	binmode FH;
	print FH $all;
}



