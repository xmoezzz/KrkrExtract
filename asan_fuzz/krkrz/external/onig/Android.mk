LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libonig

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/unicode-7.0

LOCAL_CFLAGS += -DONIG_EXTERN=extern

LOCAL_SRC_FILES := \
src/ascii.c \
src/big5.c \
src/cp1251.c \
src/euc_jp.c \
src/euc_jp_prop.c \
src/euc_kr.c \
src/euc_tw.c \
src/gb18030.c \
src/iso8859_1.c \
src/iso8859_2.c \
src/iso8859_3.c \
src/iso8859_4.c \
src/iso8859_5.c \
src/iso8859_6.c \
src/iso8859_7.c \
src/iso8859_8.c \
src/iso8859_9.c \
src/iso8859_10.c \
src/iso8859_11.c \
src/iso8859_13.c \
src/iso8859_14.c \
src/iso8859_15.c \
src/iso8859_16.c \
src/koi8_r.c \
src/onig_init.c \
src/regcomp.c \
src/regenc.c \
src/regerror.c \
src/regexec.c \
src/regext.c \
src/reggnu.c \
src/regparse.c \
src/regposerr.c \
src/regposix.c \
src/regsyntax.c \
src/regtrav.c \
src/regversion.c \
src/sjis.c \
src/sjis_prop.c \
src/st.c \
src/unicode.c \
src/utf8.c \
src/utf16_be.c \
src/utf16_le.c \
src/utf32_be.c \
src/utf32_le.c \
src/unicode-7.0/unicode_fold1_key.c \
src/unicode-7.0/unicode_fold2_key.c \
src/unicode-7.0/unicode_fold3_key.c \
src/unicode-7.0/unicode_unfold_key.c

include $(BUILD_STATIC_LIBRARY)
