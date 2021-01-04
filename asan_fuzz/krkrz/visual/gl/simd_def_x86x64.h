
#ifndef __SIMD_DEF_X86_X64_H__
#define __SIMD_DEF_X86_X64_H__


#include <intrin.h>

#ifdef _MSC_VER
#ifndef _mm_srli_pi64
#define _mm_srli_pi64 _mm_srli_si64
#endif
#ifndef _mm_slli_pi64
#define _mm_slli_pi64 _mm_slli_si64
#endif
#pragma warning(push)
#pragma warning(disable : 4799)	// ignore _mm_empty request.
#ifndef _mm_cvtsi64_m64
__inline __m64 _mm_cvtsi64_m64( __int64 v ) { __m64 ret; ret.m64_i64 = v; return ret; }
#endif
#ifndef _mm_cvtm64_si64
__inline __int64 _mm_cvtm64_si64( __m64 v ) { return v.m64_i64; }
#endif
#pragma warning(pop)
#endif

#ifdef _MSC_VER // visual c++
# define ALIGN16_BEG __declspec(align(16))
# define ALIGN16_END 
# define ALIGN32_BEG __declspec(align(32))
# define ALIGN32_END 
#else // gcc or icc
# define ALIGN16_BEG
# define ALIGN16_END __attribute__((aligned(16)))
# define ALIGN32_BEG
# define ALIGN32_END __attribute__((aligned(32)))
#endif


#define _PS_CONST128(Name, Val)    \
const ALIGN16_BEG float WeightValuesSSE::##Name[4] ALIGN16_END = { Val, Val, Val, Val }

#define _PI32_CONST128(Name, Val)  \
const ALIGN16_BEG tjs_uint32 WeightValuesSSE::##Name[4] ALIGN16_END = { Val, Val, Val, Val }


#define _PS_CONST256(Name, Val)    \
const ALIGN32_BEG float WeightValuesAVX::##Name[8] ALIGN32_END = { Val, Val, Val, Val, Val, Val, Val, Val }

#define _PI32_CONST256(Name, Val)  \
const ALIGN32_BEG tjs_uint32 WeightValuesAVX::##Name[8] ALIGN32_END = { Val, Val, Val, Val, Val, Val, Val, Val }

#define _PS_CONST_TYPE256(Name, Type, Val)                                 \
static const ALIGN32_BEG Type m256_ps_##Name[8] ALIGN32_END = { Val, Val, Val, Val, Val, Val, Val, Val }

#endif // __SIMD_DEF_X86_X64_H__
