
# メッセージ類のヘッダーファイルなどを生成します。
use utf8;
use Win32::OLE qw(in with);
use Win32::OLE::Const 'Microsoft Office .* Object Library';
$Win32::OLE::Warn = 3;
use Cwd;
use Encode qw/encode decode/;
use File::Spec;
use warnings;
use strict;

{
	my $jp_res_filename = 'string_table_jp.rc';
	my $en_res_filename = 'string_table_en.rc';
	my $chs_res_filename = 'string_table_chs.rc';
	my $res_header_filename = 'string_table_resource.h';
	my $tjs_error_header = 'tjsErrorInc.h';
	my $tvp_mes_header = 'MsgIntfInc.h';
	my $tvp_mes_win32_header = 'MsgImpl.h';
	my $tvp_mes_load = 'MsgLoad.cpp';
	my $excelfile = '\Messages.xlsx';
	my $excel;
	my @mes_id;
	my @res_id;
	my @mes_jp;
	my @mes_en;
	my @mes_chs;
	my @mes_opt;
	my @tarminate;

	# Excel起動
	binmode(STDERR,'encoding(cp932)');
	Win32::OLE->Option(CP=>Win32::OLE::CP_UTF8, Warn=>3 );
	eval { $excel = Win32::OLE->GetActiveObject('Excel.Application') };
	die "Excel not installed" if $@;
	unless ( defined $excel )
	{
		$excel = Win32::OLE->new('Excel.Application', sub {$_[0]->Quit;})
			or die "Can not start Excel";
	}

	# 非表示
	# $excel->{ DisplayAlerts } = 'False';
	$excel->{ Visible }       = 0;

	# ワークシートを開く
	my $workbook  = $excel->Workbooks->Open( getcwd . $excelfile );
	# 3シート読み込む
	for( my $i = 1; $i < 4; $i++ ) {
		my $worksheet = $workbook->Worksheets($i);

		# 2行目から読む(1行目はタイトル)
		my $row = 2;
		my $MesId;
		do {
			$MesId = $worksheet->Cells($row, 1)->{Value};
			my $MesJp = $worksheet->Cells($row, 2)->{Value};
			my $MesEn = $worksheet->Cells($row, 3)->{Value};
			my $MesOpt = $worksheet->Cells($row, 4)->{Value};
			my $MesChs = $worksheet->Cells($row, 5)->{Value};
			# リソースファイル用にエスケープする
			if( defined $MesJp ) {
				#$MesJp = decode( 'cp932', $MesJp );
				#$MesJp = encode( 'UTF-8', $MesJp );
				$MesJp =~ s/(\\")/""/g;
				$MesJp =~ s/(\\')/'/g;
			}
			if( defined $MesEn ) {
				$MesEn =~ s/(\\")/""/g;
				$MesEn =~ s/(\\')/'/g;
			}
			if (defined $MesChs) {
				$MesChs =~ s/(\\")/""/g;
				$MesChs =~ s/(\\')/'/g;
			}
			if( defined $MesId ) {
				my $ResId = $MesId;
				$ResId =~ s/^(TVP)/$1_/;
				$ResId =~ s/^(TJS)/$1_/;
				$ResId =~ s/([a-z])([A-Z])/$1_$2/g;
				$ResId = uc $ResId;
				if( defined $MesOpt ) {
					$ResId .= "_".$MesOpt;
				}
				$ResId = "IDS_".$ResId;
				push @mes_id, $MesId;
				push @res_id, $ResId;
				push @mes_jp, $MesJp;
				push @mes_en, $MesEn;
				push @mes_opt, $MesOpt;
				push @mes_chs, $MesChs;
			}
			$row++;
		} while( defined $MesId );
		my $tarlength = @mes_id;
		push @tarminate, $tarlength;
	}

	# String Table のリソースを出力
	open( FHJP, ">:raw:encoding(UTF-16LE):crlf:utf8", "$jp_res_filename" ) or die;
	open( FHEN, ">:raw:encoding(UTF-16LE):crlf:utf8", "$en_res_filename" ) or die;
	open( FHCHS, ">:raw:encoding(UTF-16LE):crlf:utf8", "$chs_res_filename" ) or die;
	open FHH, ">$res_header_filename" or die;

	open FHEH, ">$tjs_error_header" or die;
	open FHMH, ">$tvp_mes_header" or die;
	open FHMWH, ">$tvp_mes_win32_header" or die;
	open FHCPP, ">$tvp_mes_load" or die;

	print FHJP "\x{FEFF}"; #BOMを出力
	print FHJP "STRINGTABLE\n";
	print FHJP "BEGIN\n";
	print FHEN "\x{FEFF}"; #BOMを出力
	print FHEN "STRINGTABLE\n";
	print FHEN "BEGIN\n";
	print FHCHS "\x{FEFF}"; #BOMを出力
	print FHCHS "STRINGTABLE\n";
	print FHCHS "BEGIN\n";

	print FHH  <<'HEADER';
// generated from gentext.pl Messages.xlsx
#ifndef __STRING_TABLE_RESOURCE_H__
#define __STRING_TABLE_RESOURCE_H__
HEADER

	print FHEH  <<'HEADER';
// generated from gentext.pl Messages.xlsx
#ifndef __TJS_ERROR_INC_H__
#define __TJS_ERROR_INC_H__
HEADER

	print FHMH  <<'HEADER';
// generated from gentext.pl Messages.xlsx
#ifndef __MSG_INTF_INC_H__
#define __MSG_INTF_INC_H__
HEADER

	print FHMWH  <<'HEADER';
// generated from gentext.pl Messages.xlsx
#ifndef MsgImplH
#define MsgImplH

#include "tjsMessage.h"
#include "MsgIntf.h"

#ifndef TVP_MSG_DECL
	#define TVP_MSG_DECL(name, msg) extern tTJSMessageHolder name;
	#define TVP_MSG_DECL_NULL(name) extern tTJSMessageHolder name;
#endif
//---------------------------------------------------------------------------
// Message Strings
//---------------------------------------------------------------------------
HEADER

	print FHCPP <<'CPPSRC';
// generated from gentext.pl Messages.xlsx
#include "tjsCommHead.h"
#include "tjsError.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "string_table_resource.h"

static bool IS_LOAD_MESSAGE = false;
CPPSRC

	my $length = @mes_id;
	my $maxlen = 24;
	my $mesmaxlen = 1024;
	for( my $i = 0; $i < $length; $i++ ) {
		my $len = length $res_id[$i];
		if( ($len+1) > $maxlen ) { $maxlen = ($len+1); }
		$len = length $mes_jp[$i];
		if( ($len+1) > $mesmaxlen ) { $mesmaxlen = ($len+1); }
		$len = length $mes_en[$i];
		if( ($len+1) > $mesmaxlen ) { $mesmaxlen = ($len+1); }
		$len = length $mes_chs[$i];
		if( ($len+1) > $mesmaxlen ) { $mesmaxlen = ($len+1); }
	}
	print FHCPP "static const int MAX_MESSAGE_LENGTH = $mesmaxlen;\n";
	print FHCPP "enum {\n";

	for( my $i = 0; $i < $length; $i++ ) {
		my $len = length $res_id[$i];
		my $line = "    ".$res_id[$i];
		my $header_res = "#define ".$res_id[$i];
		for( my $j = $len; $j < $maxlen; $j++ ) {
			$line .= " ";
			$header_res .= " ";
		}
		my $jpline = $line . "\"".$mes_jp[$i]."\"\n";
		my $enline = $line . "\"".$mes_en[$i]."\"\n";
		my $chsline = $line . "\"".$mes_chs[$i]."\"\n";
		print FHJP $jpline;
		print FHEN $enline;
		print FHCHS $chsline;
		my $id = $i + 10000; # 10000以降の番号に割り当てておく
		print FHH $header_res.$id."\n";
		if( !defined $mes_opt[$i] ) {
			if( $i < $tarminate[0] ) {
				print FHEH "TJS_MSG_DECL_NULL(".$mes_id[$i].")\n";
			} elsif( $i < $tarminate[1] ) {
				print FHMH "TVP_MSG_DECL_NULL(".$mes_id[$i].")\n";
			} else {
				print FHMWH "TVP_MSG_DECL_NULL(".$mes_id[$i].")\n";
			}
		}
		my $enumid = $res_id[$i];
		$enumid =~ s/^(IDS_)//;
		$enumid = "NUM_".$enumid;
		print FHCPP "\t".$enumid.",\n";
	}
	print FHCPP "\tNUM_MESSAGE_MAX\n";
	print FHCPP "};\n";
	print FHCPP "const tjs_char* RESOURCE_MESSAGE[NUM_MESSAGE_MAX];\n";
	print FHCPP "const int RESOURCE_IDS[NUM_MESSAGE_MAX] = {\n";
	for( my $i = 0; $i < $length; $i++ ) {
		print FHCPP "\t".$res_id[$i].",\n";
	}
	print FHCPP <<'CPPSRC';
};
void TVPLoadMessage() {
	if( IS_LOAD_MESSAGE ) return;
	IS_LOAD_MESSAGE = true;
	tjs_char buffer[MAX_MESSAGE_LENGTH];
	HINSTANCE hInstance = ::GetModuleHandle(0);
	for( int i = 0; i < NUM_MESSAGE_MAX; i++ ) {
		int len = ::LoadString( hInstance, RESOURCE_IDS[i], buffer, MAX_MESSAGE_LENGTH );
		if( len <= 0 ) {
			TVPThrowExceptionMessage( TJS_W("Message Load Error!") );
		}
		tjs_char* work = new tjs_char[len+1];
		wcscpy_s( work, len+1, buffer );
		RESOURCE_MESSAGE[i] = work;
	}
CPPSRC
	my $is_opt = 0;
	for( my $i = 0; $i < $length; $i++ ) {
		my $enumid = $res_id[$i];
		$enumid =~ s/^(IDS_)//;
		$enumid = "NUM_".$enumid;
		if( defined $mes_opt[$i] ) {
			if( $mes_opt[$i] eq "CRLF" ) {
				print FHCPP "#ifdef TJS_TEXT_OUT_CRLF\n";
				print FHCPP "\t".$mes_id[$i].".AssignMessage( RESOURCE_MESSAGE[".$enumid."] );\n";
				print FHCPP "#else\n";
				$is_opt = 1;
			} elsif( $mes_opt[$i] eq "ANSI" ) {
				print FHCPP "#ifdef TVP_TEXT_READ_ANSI_MBCS\n";
				print FHCPP "\t".$mes_id[$i].".AssignMessage( RESOURCE_MESSAGE[".$enumid."] );\n";
				print FHCPP "#else\n";
				$is_opt = 1;
			}
		} else {
			print FHCPP "\t".$mes_id[$i].".AssignMessage( RESOURCE_MESSAGE[".$enumid."] );\n";
			if( $is_opt == 1 ) {
				print FHCPP "#endif\n";
				$is_opt = 0;
			}
		}
	}
	print FHCPP <<'CPPSRC';
}
const tjs_char* TVPGetMessage( tjs_int id ) {
	if( id >= 0 && id < NUM_MESSAGE_MAX ) {
		return RESOURCE_MESSAGE[id];
	} else {
		return NULL;
	}
}
static void TVPFreeMessages() {
	for( int i = 0; i < NUM_MESSAGE_MAX; i++ ) {
		delete[] RESOURCE_MESSAGE[i];
	}
}
static tTVPAtExit
	TVPUninitMessageLoad(TVP_ATEXIT_PRI_RELEASE, TVPFreeMessages);
CPPSRC

	close FHCPP;

	print FHJP "END\n";
	close FHJP;

	print FHEN "END\n";
	close FHEN;

	print FHCHS "END\n";
	close FHCHS;
	
	print FHH "#endif\n";
	close FHH;

	print FHEH "#endif\n";
	close FHEH;

	print FHMH "#endif\n";
	close FHMH;

	print FHMWH "#endif\n";
	close FHMWH;

	# Excel終了
	$workbook->Close();
	$excel->Quit();

	exit;
}
