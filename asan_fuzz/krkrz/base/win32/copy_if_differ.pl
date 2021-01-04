;# this is a part of TVP (KIRIKIRI) software source.
;# see other sources for license.
;# (C)2001-2009 W.Dee <dee@kikyou.info> and contributors

;# copy $ARGV[0] to $ARGV[1] if the content differs
use Digest::MD5  qw(md5 md5_hex md5_base64);

sub md5_file
{
    my $file = $_[0];
    if(!open(FILE, $file)) {
    	return "";
    }
    binmode(FILE);

    my $md5 = Digest::MD5->new;
    while (<FILE>) {
        $md5->add($_);
    }
    close(FILE);
    my $digest = $md5->b64digest;
    return $digest;
}

sub copy_file
{
	my $src = $_[0];
	my $dest = $_[1];
	print "Copying $src to $dest ...\n";
    open(SRC, $src) or die "Can't open '$src': $!";
    open(DEST, ">$dest") or die "Can't open '$dest': $!";
	binmode SRC;
	binmode DEST;

	while(<SRC>) {
		print DEST $_;
	}

	close(SRC);
	close(DEST);
}

$md5_1 = md5_file($ARGV[0]);
$md5_2 = md5_file($ARGV[1]);

copy_file($ARGV[0], $ARGV[1]) if($md5_1 ne $md5_2);
