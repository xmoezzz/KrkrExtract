; -*- Mode: asm; tab-width: 12; -*-
; this is a part of TVP (KIRIKIRI) software source.
; see other sources for license.
; (C)2001-2009 W.Dee <dee@kikyou.info> and contributors

; TLG6 chroma/luminosity decoder
; This does reordering, color correlation filter, MED/AVG

%ifndef			GEN_CODE

%include		"nasm.nah"



externdef		TVPTLG6GolombBitLengthTable
globaldef		TVPTLG6DecodeLine_mmx_a
globaldef		TVPTLG6DecodeLine_sse_a

%define		TVP_TLG6_W_BLOCK_SIZE		8

;--------------------------------------------------------------------
	start_const_aligned
;--------------------------------------------------------------------
g_mask			dd		0x0000ff00, 0x0000ff00
b_mask			dd		0x000000ff, 0x000000ff
r_mask			dd		0x00ff0000, 0x00ff0000
a_mask			dd		0xff000000, 0xff000000
g_d_mask		dd		0x0000fe00, 0x0000fe00
r_d_mask		dd		0x00fe0000, 0x00fe0000
b_d_mask		dd		0x000000fe, 0x000000fe
avg_mask_fe		dd		0xfefefefe, 0xfefefefe
avg_mask_01		dd		0x01010101, 0x01010101

;--------------------------------------------------------------------
	end_const_aligned
;--------------------------------------------------------------------


;--------------------------------------------------------------------
	segment_code
;--------------------------------------------------------------------
%define		GEN_CODE
;--------------------------------------------------------------------
;;[function_replace_by TVPCPUType & TVP_CPU_HAS_MMX] TVPTLG6DecodeLine
;;void, TVPTLG6DecodeLine_mmx_a, (tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *input, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir)
%define FUNCTION_LABEL TVPTLG6DecodeLine_mmx_a
	%include "tlg6_chroma.nas"

;--------------------------------------------------------------------
;;[function_replace_by TVPCPUType & TVP_CPU_HAS_SSE && TVPCPUType & TVP_CPU_HAS_MMX && TVPCPUType & TVP_CPU_HAS_EMMX] TVPTLG6DecodeLine
;;void, TVPTLG6DecodeLine_sse_a, (tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *input, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir)
%define			USE_SSE
%define FUNCTION_LABEL TVPTLG6DecodeLine_sse_a
	%include "tlg6_chroma.nas"


%else
;--------------------------------------------------------------------
;		macros
;--------------------------------------------------------------------
%unmacro		do_med	1
%macro		do_med	1
	movq	mm2,	mm6		;// mm2 as c
	movq	mm6,	mm1

	%ifndef	USE_SSE
	movq	mm3,	mm0
	movq	mm5,	mm1
	psubusb	mm3,	mm1
	psubusb	mm5,	mm0
	paddb	mm3,	mm1		;// mm3 = max_a_b
	psubb	mm1,	mm5		;// mm1 = min_a_b
	movq	mm0,	mm3
	paddb	%1 ,	mm3
	psubusb	mm0,	mm2
	psubb	mm3,	mm0		;// mm3 = max_a_b<c?max_a_b:c
	paddb	%1 ,	mm1
	psubusb	mm3,	mm1
	paddb	mm3,	mm1		;// mm3 = min_a_b<mm0?mm0:min_a_b
	psubb	%1 ,	mm3
	movq	mm0,	%1 
	%else
	movq	mm3,	mm0
	pmaxub	mm0,	mm1		;// mm0 = max_a_b
	pminub	mm1,	mm3		;// mm1 = min_a_b
	paddb	%1 ,	mm0
	pminub	mm0,	mm2		;// mm0 = max_a_b<c?max_a_b:c
	paddb	%1 ,	mm1
	pmaxub	mm0,	mm1		;// mm0 = min_a_b<mm0?mm0:min_a_b
	psubb	%1 ,	mm0
	movq	mm0,	%1 
	%endif
%endmacro

%unmacro		do_avg	1
%macro		do_avg	1
	movq	mm2,	mm6		;// mm2 as c
	movq	mm6,	mm1

	%ifndef	USE_SSE
	movq	mm3,	mm0
	movq	mm5,	mm1
	pand	mm3,	[avg_mask_fe]
	pand	mm5,	[avg_mask_fe]
	psrlq	mm3,	1
	psrlq	mm5,	1
	por	mm0,	mm1
	paddb	mm3,	mm5
	pand	mm0,	[avg_mask_01]
	paddb	%1 ,	mm0
	paddb	%1 ,	mm3
	movq	mm0,	%1 
	%else
	pavgb	mm0,	mm1
	paddb	%1,	mm0
	movq	mm0,	%1
	%endif
%endmacro

%unmacro			do_filter_med_f		0
%macro			do_filter_med_f		0
%%loop:
	movq	mm4,	[edx + eax]

	filter_insts

	movq	mm7,	mm4

	movd	mm1,	[esi]		;// mm1 as b

	do_med	mm7

	movd	[edi],	mm7

	psrlq	mm4,	32

	add	edx,	[%$step]

	movd	mm1,	[esi+4]		;// mm1 as b

	do_med	mm4

	movd	[edi+4],	mm4

	dec	ebx

	lea	edi,	[edi+8]
	lea	esi,	[esi+8]
	jnz	%%loop

	add	edx,	[%$skipblockbytes]

	dec	dword [%$block_count]
	jnz	.dl3_loop
	jmp	.dl3_exit
%endmacro

%unmacro			do_filter_med_b		0
%macro			do_filter_med_b		0
%%loop:
	movq	mm4,	[edx + eax - 4]

	filter_insts

	movq	mm7,	mm4
	psrlq	mm7,	32

	movd	mm1,	[esi]		;// mm1 as b

	do_med	mm7

	movd	[edi],	mm7

	add	edx,	[%$step]

	movd	mm1,	[esi+4]		;// mm1 as b

	do_med	mm4

	movd	[edi+4],	mm4

	dec	ebx

	lea	edi,	[edi+8]
	lea	esi,	[esi+8]
	jnz	%%loop

	add	edx,	[%$skipblockbytes]

	dec	dword [%$block_count]
	jnz	.dl3_loop
	jmp	.dl3_exit
%endmacro

%unmacro			do_filter_avg_f		0
%macro			do_filter_avg_f		0
%%loop:
	movq	mm4,	[edx + eax]

	filter_insts

	movq	mm7,	mm4

	movd	mm1,	[esi]		;// mm1 as b

	do_avg	mm7

	movd	[edi],	mm7

	psrlq	mm4,	32

	add	edx,	[%$step]

	movd	mm1,	[esi+4]		;// mm1 as b

	do_avg	mm4

	movd	[edi+4],	mm4

	dec	ebx

	lea	edi,	[edi+8]
	lea	esi,	[esi+8]
	jnz	%%loop

	add	edx,	[%$skipblockbytes]

	dec	dword [%$block_count]
	jnz	.dl3_loop
	jmp	.dl3_exit
%endmacro

%unmacro			do_filter_avg_b		0
%macro			do_filter_avg_b		0
%%loop:
	movq	mm4,	[edx + eax - 4]

	filter_insts

	movq	mm7,	mm4
	psrlq	mm7,	32

	movd	mm1,	[esi]		;// mm1 as b

	do_avg	mm7

	movd	[edi],	mm7

	add	edx,	[%$step]

	movd	mm1,	[esi+4]		;// mm1 as b

	do_avg	mm4

	movd	[edi+4],	mm4

	dec	ebx

	lea	edi,	[edi+8]
	lea	esi,	[esi+8]
	jnz	%%loop

	add	edx,	[%$skipblockbytes]

	dec	dword [%$block_count]
	jnz	.dl3_loop
	jmp	.dl3_exit
%endmacro


;--------------------------------------------------------------------
	segment_code
;--------------------------------------------------------------------
proc_start	FUNCTION_LABEL
	proc_arg	4,	prevline
	proc_arg	4,	curline
	proc_arg	4,	width
	proc_arg	4,	block_count
	proc_arg	4,	filtertypes
	proc_arg	4,	skipblockbytes
	proc_arg	4,	input
	proc_arg	4,	initialp
	proc_arg	4,	oddskip
	proc_arg	4,	dir
	local_var	4,	step
	local_var	4,	phase

	end_local_vars

	test	byte [%$dir],	1
	IF	nz
		; forward processing
	mov	dword [%$$step],	8
	mov	eax,	[%$$skipblockbytes]
	lea	eax,	[eax*4-(TVP_TLG6_W_BLOCK_SIZE)*4]
	mov	[%$$skipblockbytes],	eax
	ELSE
		; backward processing
	mov	dword [%$$step],	-8
	mov	eax,	[%$$skipblockbytes]
	lea	eax,	[eax*4+(TVP_TLG6_W_BLOCK_SIZE)*4]
	mov	[%$$skipblockbytes],	eax
	mov	eax,	[%$$input]
	add	eax,	byte (TVP_TLG6_W_BLOCK_SIZE - 1)*4
	mov	[%$$input],	eax
	ENDIF

	%if   TVP_TLG6_W_BLOCK_SIZE == 16
	shl	dword [%$oddskip],	(4+2)	
	%elif TVP_TLG6_W_BLOCK_SIZE == 8
	shl	dword [%$oddskip],	(3+2)
	%else
	%error	TVP_TLG6_W_BLOCK_SIZE must be 16 or 8
	%endif

	xor	edx,	edx
	mov	[%$phase],	edx
	mov	edi,	[%$curline]
	mov	esi,	[%$prevline]
	mov	eax,	[%$initialp]
	movd	mm0,	eax
	movd	mm6,	eax
	mov	eax,	[%$input]
	and	ebx,	byte ~0x0f
	add	eax,	[%$oddskip]

.dl3_loop:
	mov	ecx,	dword [%$filtertypes]
	movzx	ecx,	byte [ecx]		;// ecx as current filter type
	mov	ebx,	(TVP_TLG6_W_BLOCK_SIZE>>1)		;// ebx as counter
	xor	[%$phase],	byte 1
	IF	z
		; odd phase
	add	eax,	[%$$oddskip]
	ELSE
		; even phase
	sub	eax,	[%$$oddskip]
	ENDIF
	inc	dword [%$filtertypes]
	test	byte [%$dir],	1
	IF	nz
		; forward processing
	jmp	dword [ecx*4 + .dl3_filter_med_f_jump_table]
	ELSE
		; backward processing
	jmp	dword [ecx*4 + .dl3_filter_med_b_jump_table]
	ENDIF

	align	4
.dl3_filter_med_f_jump_table:
	dd	.dl3_filter_med_f_0
	dd	.dl3_filter_avg_f_0

	dd	.dl3_filter_med_f_1
	dd	.dl3_filter_avg_f_1

	dd	.dl3_filter_med_f_2
	dd	.dl3_filter_avg_f_2

	dd	.dl3_filter_med_f_3
	dd	.dl3_filter_avg_f_3

	dd	.dl3_filter_med_f_4
	dd	.dl3_filter_avg_f_4

	dd	.dl3_filter_med_f_5
	dd	.dl3_filter_avg_f_5

	dd	.dl3_filter_med_f_6
	dd	.dl3_filter_avg_f_6

	dd	.dl3_filter_med_f_7
	dd	.dl3_filter_avg_f_7

	dd	.dl3_filter_med_f_8
	dd	.dl3_filter_avg_f_8

	dd	.dl3_filter_med_f_9
	dd	.dl3_filter_avg_f_9

	dd	.dl3_filter_med_f_10
	dd	.dl3_filter_avg_f_10

	dd	.dl3_filter_med_f_11
	dd	.dl3_filter_avg_f_11

	dd	.dl3_filter_med_f_12
	dd	.dl3_filter_avg_f_12

	dd	.dl3_filter_med_f_13
	dd	.dl3_filter_avg_f_13

	dd	.dl3_filter_med_f_14
	dd	.dl3_filter_avg_f_14

	dd	.dl3_filter_med_f_15
	dd	.dl3_filter_avg_f_15

.dl3_filter_med_b_jump_table:
	dd	.dl3_filter_med_b_0
	dd	.dl3_filter_avg_b_0

	dd	.dl3_filter_med_b_1
	dd	.dl3_filter_avg_b_1

	dd	.dl3_filter_med_b_2
	dd	.dl3_filter_avg_b_2

	dd	.dl3_filter_med_b_3
	dd	.dl3_filter_avg_b_3

	dd	.dl3_filter_med_b_4
	dd	.dl3_filter_avg_b_4

	dd	.dl3_filter_med_b_5
	dd	.dl3_filter_avg_b_5

	dd	.dl3_filter_med_b_6
	dd	.dl3_filter_avg_b_6

	dd	.dl3_filter_med_b_7
	dd	.dl3_filter_avg_b_7

	dd	.dl3_filter_med_b_8
	dd	.dl3_filter_avg_b_8

	dd	.dl3_filter_med_b_9
	dd	.dl3_filter_avg_b_9

	dd	.dl3_filter_med_b_10
	dd	.dl3_filter_avg_b_10

	dd	.dl3_filter_med_b_11
	dd	.dl3_filter_avg_b_11

	dd	.dl3_filter_med_b_12
	dd	.dl3_filter_avg_b_12

	dd	.dl3_filter_med_b_13
	dd	.dl3_filter_avg_b_13

	dd	.dl3_filter_med_b_14
	dd	.dl3_filter_avg_b_14

	dd	.dl3_filter_med_b_15
	dd	.dl3_filter_avg_b_15

	nop
	nop
	nop
	nop
	nop

		; ------------------------------------------------------------ 0
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	%endmacro

	loop_align
.dl3_filter_med_f_0:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_0:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_0:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_0:
	do_filter_avg_b

		; ------------------------------------------------------------ 1
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	movq	mm3,	mm4
	pand	mm2,	[g_mask]
	pand	mm3,	[g_mask]
	psllq	mm2,	8
	psrlq	mm3,	8
	paddb	mm4,	mm2
	paddb	mm4,	mm3
	%endmacro

	loop_align
.dl3_filter_med_f_1:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_1:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_1:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_1:
	do_filter_avg_b

		; ------------------------------------------------------------ 2
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_2:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_2:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_2:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_2:
	do_filter_avg_b

		; ------------------------------------------------------------ 3
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_3:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_3:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_3:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_3:
	do_filter_avg_b

		; ------------------------------------------------------------ 4
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psrlq	mm2,	16
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_4:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_4:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_4:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_4:
	do_filter_avg_b

		; ------------------------------------------------------------ 5
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psrlq	mm2,	16
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_5:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_5:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_5:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_5:
	do_filter_avg_b

		; ------------------------------------------------------------ 6
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_6:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_6:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_6:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_6:
	do_filter_avg_b

		; ------------------------------------------------------------ 7
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_7:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_7:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_7:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_7:
	do_filter_avg_b

		; ------------------------------------------------------------ 8
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psllq	mm2,	8
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_8:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_8:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_8:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_8:
	do_filter_avg_b

		; ------------------------------------------------------------ 9
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psllq	mm2,	16
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_9:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_9:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_9:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_9:
	do_filter_avg_b

		; ------------------------------------------------------------ 10
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	movq	mm3,	mm4
	psrlq	mm2,	8
	psrlq	mm3,	16
	pand	mm2,	[g_mask]
	pand	mm3,	[b_mask]
	paddb	mm4,	mm2
	paddb	mm4,	mm3
	%endmacro

	loop_align
.dl3_filter_med_f_10:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_10:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_10:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_10:
	do_filter_avg_b

		; ------------------------------------------------------------ 11
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	movq	mm3,	mm4
	psllq	mm2,	8
	psllq	mm3,	16
	pand	mm2,	[g_mask]
	pand	mm3,	[r_mask]
	paddb	mm4,	mm2
	paddb	mm4,	mm3
	%endmacro

	loop_align
.dl3_filter_med_f_11:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_11:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_11:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_11:
	do_filter_avg_b

		; ------------------------------------------------------------ 12
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psllq	mm2,	16
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_12:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_12:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_12:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_12:
	do_filter_avg_b

		; ------------------------------------------------------------ 13
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psllq	mm2,	16
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_13:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_13:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_13:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_13:
	do_filter_avg_b

		; ------------------------------------------------------------ 14
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[g_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psrlq	mm2,	8
	pand	mm2,	[b_mask]
	paddb	mm4,	mm2
	movq	mm2,	mm4
	psllq	mm2,	16
	pand	mm2,	[r_mask]
	paddb	mm4,	mm2
	%endmacro

	loop_align
.dl3_filter_med_f_14:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_14:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_14:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_14:
	do_filter_avg_b

		; ------------------------------------------------------------ 15
	%unmacro	filter_insts	0
	%macro	filter_insts	0
	movq	mm2,	mm4
	movq	mm3,	mm4
	psllq	mm2,	(8+1)
	psllq	mm3,	(16+1)
	pand	mm2,	[g_d_mask]
	pand	mm3,	[r_d_mask]
	paddb	mm4,	mm2
	paddb	mm4,	mm3
	%endmacro

	loop_align
.dl3_filter_med_f_15:
	do_filter_med_f

	loop_align
.dl3_filter_med_b_15:
	do_filter_med_b

	loop_align
.dl3_filter_avg_f_15:
	do_filter_avg_f

	loop_align
.dl3_filter_avg_b_15:
	do_filter_avg_b

.dl3_exit:
	emms

proc_end

%endif
