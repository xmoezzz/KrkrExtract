; -*- Mode: asm; tab-width: 12; -*-
; this is a part of TVP (KIRIKIRI) software source.
; see other sources for license.
; (C)2001-2009 W.Dee <dee@kikyou.info> and contributors

; TLG6 golomb code decoder

%ifndef			GEN_CODE

%include		"nasm.nah"


externdef		TVPTLG6GolombBitLengthTable
globaldef		TVPTLG6DecodeGolombValuesForFirst_a
globaldef		TVPTLG6DecodeGolombValues_a
globaldef		TVPTLG6DecodeGolombValuesForFirst_mmx_a
globaldef		TVPTLG6DecodeGolombValues_mmx_a
globaldef		TVPTLG6DecodeGolombValuesForFirst_emmx_a
globaldef		TVPTLG6DecodeGolombValues_emmx_a

%define		TVP_TLG6_GOLOMB_N_COUNT		4

;--------------------------------------------------------------------
	segment_data
;--------------------------------------------------------------------
	align 4
bit_mask_tbl:
	dd	(1<< 0)-1, (1<< 1)-1, (1<< 2)-1, (1<< 3)-1
	dd	(1<< 4)-1, (1<< 5)-1, (1<< 6)-1, (1<< 7)-1
	dd	(1<< 8)-1, (1<< 9)-1, (1<<10)-1, (1<<11)-1
	dd	(1<<12)-1, (1<<13)-1, (1<<14)-1, (1<<15)-1
	dd	(1<<16)-1, (1<<17)-1, (1<<18)-1, (1<<19)-1
	dd	(1<<20)-1, (1<<21)-1, (1<<22)-1, (1<<23)-1
	dd	(1<<24)-1, (1<<25)-1, (1<<26)-1, (1<<27)-1
	dd	(1<<28)-1, (1<<29)-1, (1<<30)-1, (1<<31)-1
	dd	(1<<32)-1
%if 0
bit_mask_neg_tbl:
	dd	~((1<< 0)-1), ~((1<< 1)-1), ~((1<< 2)-1), ~((1<< 3)-1)
	dd	~((1<< 4)-1), ~((1<< 5)-1), ~((1<< 6)-1), ~((1<< 7)-1)
	dd	~((1<< 8)-1), ~((1<< 9)-1), ~((1<<10)-1), ~((1<<11)-1)
	dd	~((1<<12)-1), ~((1<<13)-1), ~((1<<14)-1), ~((1<<15)-1)
	dd	~((1<<16)-1), ~((1<<17)-1), ~((1<<18)-1), ~((1<<19)-1)
	dd	~((1<<20)-1), ~((1<<21)-1), ~((1<<22)-1), ~((1<<23)-1)
	dd	~((1<<24)-1), ~((1<<25)-1), ~((1<<26)-1), ~((1<<27)-1)
	dd	~((1<<28)-1), ~((1<<29)-1), ~((1<<30)-1), ~((1<<31)-1)
	dd	~((1<<32)-1)
%endif

;--------------------------------------------------------------------
	segment_code
;--------------------------------------------------------------------
%define		GEN_CODE

;--------------------------------------------------------------------
%define		FOR_FIRST_COLOR_COMPONENT
;;[function_replace_by 1] TVPTLG6DecodeGolombValuesForFirst
;;void, TVPTLG6DecodeGolombValuesForFirst_a, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
%define FUNCTION_LABEL TVPTLG6DecodeGolombValuesForFirst_a
	%include "tlg6_golomb.nas"
%define		GEN_CODE
%undef		FOR_FIRST_COLOR_COMPONENT
;;[function_replace_by 1] TVPTLG6DecodeGolombValues
;;void, TVPTLG6DecodeGolombValues_a, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
%define FUNCTION_LABEL TVPTLG6DecodeGolombValues_a
	%include "tlg6_golomb.nas"

;--------------------------------------------------------------------
%define		FOR_FIRST_COLOR_COMPONENT
%define			USE_MMX
;;[function_replace_by TVPCPUType & TVP_CPU_HAS_MMX] TVPTLG6DecodeGolombValuesForFirst
;;void, TVPTLG6DecodeGolombValuesForFirst_mmx_a, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
%define FUNCTION_LABEL TVPTLG6DecodeGolombValuesForFirst_mmx_a
	%include "tlg6_golomb.nas"

%undef		FOR_FIRST_COLOR_COMPONENT
%define			USE_MMX
;;[function_replace_by TVPCPUType & TVP_CPU_HAS_MMX] TVPTLG6DecodeGolombValues
;;void, TVPTLG6DecodeGolombValues_mmx_a, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
%define FUNCTION_LABEL TVPTLG6DecodeGolombValues_mmx_a
	%include "tlg6_golomb.nas"

;--------------------------------------------------------------------
%define		FOR_FIRST_COLOR_COMPONENT
;;[function_replace_by TVPCPUType & TVP_CPU_HAS_EMMX && TVPCPUType & TVP_CPU_HAS_MMX] TVPTLG6DecodeGolombValuesForFirst
;;void, TVPTLG6DecodeGolombValuesForFirst_emmx_a, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
%define			USE_EMMX
%define FUNCTION_LABEL TVPTLG6DecodeGolombValuesForFirst_emmx_a
	%include "tlg6_golomb.nas"

%undef		FOR_FIRST_COLOR_COMPONENT
;;[function_replace_by TVPCPUType & TVP_CPU_HAS_EMMX && TVPCPUType & TVP_CPU_HAS_MMX] TVPTLG6DecodeGolombValues
;;void, TVPTLG6DecodeGolombValues_emmx_a, (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
%define			USE_EMMX
%define FUNCTION_LABEL TVPTLG6DecodeGolombValues_emmx_a
	%include "tlg6_golomb.nas"

;--------------------------------------------------------------------
%else
;--------------------------------------------------------------------
%unmacro	get_running_count 0
%macro	get_running_count 0

		; get running count
	or	eax,	byte -1
	mov	edi,	1
	shl	eax,	cl		; ebx = (-1) << bit_pos
	and	eax,	[esi]		; load dword bit_pool
	bsf	eax,	eax		; bit scan forward
	mov	edx,	eax
	sub	eax,	ecx		; eax = bit_count
	lea	ecx,	[edx+1]		; ecx as bit_pos (found bit position +1)
	mov	ebx,	ecx		; bit_pos normalize :

	IF	nz		; if bit_count != 0

		mov	edx,	[bit_mask_tbl+eax*4]		; edx = (1<<bit_count)-1
		shr	ebx,	3		; bit_pos normalize :
		add	esi,	ebx		; bit_pos normalize : bit_pool += (bit_pos>>3)
		and	ecx,	byte 7		; bit_pos normalize : bit_pos &= 7

		mov	ebx,	dword [esi]		; ebx = load dword bit_pool
		shr	ebx,	cl		; ebx >>= bit_pos
		and	ebx,	edx		; ebx &= edx
		lea	edi,	[edx+ebx+1]		; edi as running count

		add	ecx,	eax		; eax += bit_count

		mov	ebx,	ecx		; bit_pos normalize :
	ENDIF

%ifdef		USE_EMMX
		prefetcht0	[esi+32]
%endif
	shr	ebx,	3
	add	esi,	ebx		; bit_pool += (bit_pos>>3)
	and	ecx,	byte 7		; bit_pos &= 7
%endmacro

;--------------------------------------------------------------------
proc_start			FUNCTION_LABEL
	proc_arg	4,	pixelbuf
	proc_arg	4,	pixel_count
	proc_arg	4,	bit_pool
	local_var	4,	a
	local_var	4,	n
	local_var	4,	limit
	local_var	4,	zero

	end_local_vars

	mov	eax,	[%$pixelbuf]
%ifdef	USE_MMX
	movd	mm0,	eax
%endif
	mov	ebx,	[%$pixel_count]
	lea	ebx,	[ebx*4+eax]
	mov	[%$limit],	ebx
	cld
	mov	[%$n],	dword (TVP_TLG6_GOLOMB_N_COUNT - 1)
	xor	eax,	eax
	mov	[%$a],	eax
	mov	esi,	[%$bit_pool]		; esi as bit_pool
	mov	ecx,	1		; cl as bit_pos

	test	byte [esi],	1		; (*bit_pool&1) ?
	jz	near	.zero_part
	jmp	near	.non_zero_part

	loop_align
	.golomb_loop:

		;----------------------------------------------------- zero part
	.zero_part:

	get_running_count

%ifndef	USE_MMX
	mov	eax,	[%$pixelbuf]
%else
	movd	eax,	mm0
%endif
	lea	ebx,	[eax + edi*4]
%ifdef	FOR_FIRST_COLOR_COMPONENT
	lea	edi,	[edi*4-4]

.zero_part_loop:
	mov	dword [eax+edi],	0
	sub	edi,	4
	jns	.zero_part_loop

	.zero_part_exit:
%endif
	cmp	[%$limit],	ebx		; limit > ebx ?
%ifndef	USE_MMX
	mov	[%$pixelbuf],	ebx
%else
	movd	mm0,	ebx
%endif
	je	near .exit		; if not, exit

		;----------------------------------------------------- non zero part
	.non_zero_part:
	get_running_count

		; decode non-zero values, coded via golomb codes.
%ifndef	USE_MMX
	mov	edx,	[%$pixelbuf]
%else
	movd	edx,	mm0
%endif

	.decode_non_zero_loop:
	or	eax,	byte -1
	push	edi

	shl	eax,	cl		; eax = (-1) << bit_pos
	add	edx,	byte 4		; pixelbuf ++
	and	eax,	dword [esi]		; mask
	bsf	eax,	eax		; bit scan forward
	IF	z

			; when eax is zero
			; insufficient bits
		add	esi,	byte 4		; bit_pool += 4
		movzx	edi,	byte [esi]		; the true bit_count comes here
		mov	ecx,	8
		jmp	short 	.got_value

	ENDIF

	mov	edi,	eax
	sub	edi,	ecx
	lea	ecx,	[eax+1]

	.got_value:
	mov	eax,	[%$a]
	push	edx
	mov	ebx,	[%$n]


		;-- now edi as bit_count

	movzx	eax,	byte [TVPTLG6GolombBitLengthTable + eax*TVP_TLG6_GOLOMB_N_COUNT + ebx]
	or	eax,	eax		; eax as k
	IF	nz

		mov	ebx,	ecx
		shr	ebx,	3
		add	esi,	ebx		; bit_pool += (bit_pos>>3)
		and	ecx,	byte 7		; bit_pos &= 7

		mov	edx,	eax
		mov	ebx,	ecx

		mov	eax,	dword [esi]		; load bit_pool
		shr	eax,	cl		; eax >>= cl

		mov	ecx,	edx
		shl	edi,	cl		; bit_count <<= k
		and	eax,	[bit_mask_tbl+ecx*4]		; eax = ((1<<k)-1) & eax

		add	ecx,	ebx		; bit_pos += k, ecx as bit_pos

	ENDIF

	add	eax,	edi		; eax as v

	mov	ebx,	ecx		; normalize bit_pos : 
	mov	edx,	eax
	shr	ebx,	3		; normalize bit_pos : 
	and	edx,	byte 1
	add	esi,	ebx		; normalize bit_pos : bit_pool += (bit_pos>>3)
	dec	edx		; edx as sign

	shr	eax,	byte 1		; v >>= 1
	add	[%$a],	eax		; a += v
	and	ecx,	byte 7		; normalize bit_pos : bit_pos &= 7
	xor	eax,	edx
	lea	eax,	[eax+edx+1]		; now al is final value


	dec	dword [%$n]
	pop	edx
	pop	edi

	IF	s		; if n < 0
		shr	dword [%$$a],	1
		mov	dword [%$$n],	(TVP_TLG6_GOLOMB_N_COUNT - 1)
	ENDIF

%ifdef	FOR_FIRST_COLOR_COMPONENT
	and	eax,	0x000000ff
%endif

	dec	edi
%ifndef	FOR_FIRST_COLOR_COMPONENT
	mov	byte [edx-4],	al		; store pixelbuf
%else
	mov	dword [edx-4],	eax		; store pixelbuf
%endif
	jnz	near .decode_non_zero_loop

	cmp	[%$limit],	edx		; limit > edx ?
%ifndef	USE_MMX
	mov	[%$pixelbuf],	edx
%else
	movd	mm0,	edx
%endif

	jne	near .golomb_loop		; then loop

	jmp	short .exit

.exit:

%ifdef	USE_MMX
	emms
%endif
proc_end

;--------------------------------------------------------------------
%endif
