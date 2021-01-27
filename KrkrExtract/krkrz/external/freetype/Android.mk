LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

FREETYPE_SRC_PATH := 

LOCAL_MODULE := freetype

LOCAL_CFLAGS := -DANDROID_NDK \
-DFT2_BUILD_LIBRARY=1

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include_all \
$(LOCAL_PATH)/include \
$(LOCAL_PATH)/src
#$(FREETYPE_SRC_PATH)include \
#$(FREETYPE_SRC_PATH)src

LOCAL_SRC_FILES := \
$(FREETYPE_SRC_PATH)src/autofit/autofit.c \
$(FREETYPE_SRC_PATH)src/base/basepic.c \
$(FREETYPE_SRC_PATH)src/base/ftapi.c \
$(FREETYPE_SRC_PATH)src/base/ftbase.c \
$(FREETYPE_SRC_PATH)src/base/ftbbox.c \
$(FREETYPE_SRC_PATH)src/base/ftbitmap.c \
$(FREETYPE_SRC_PATH)src/base/ftdbgmem.c \
$(FREETYPE_SRC_PATH)src/base/ftdebug.c \
$(FREETYPE_SRC_PATH)src/base/ftglyph.c \
$(FREETYPE_SRC_PATH)src/base/ftinit.c \
$(FREETYPE_SRC_PATH)src/base/ftpic.c \
$(FREETYPE_SRC_PATH)src/base/ftstroke.c \
$(FREETYPE_SRC_PATH)src/base/ftsynth.c \
$(FREETYPE_SRC_PATH)src/base/ftsystem.c \
$(FREETYPE_SRC_PATH)src/cff/cff.c \
$(FREETYPE_SRC_PATH)src/pshinter/pshinter.c \
$(FREETYPE_SRC_PATH)src/pshinter/pshglob.c \
$(FREETYPE_SRC_PATH)src/pshinter/pshpic.c \
$(FREETYPE_SRC_PATH)src/pshinter/pshrec.c \
$(FREETYPE_SRC_PATH)src/psnames/psnames.c \
$(FREETYPE_SRC_PATH)src/psnames/pspic.c \
$(FREETYPE_SRC_PATH)src/raster/raster.c \
$(FREETYPE_SRC_PATH)src/raster/rastpic.c \
$(FREETYPE_SRC_PATH)src/sfnt/pngshim.c \
$(FREETYPE_SRC_PATH)src/sfnt/sfntpic.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttbdf.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttkern.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttload.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttmtx.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttpost.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttsbit.c \
$(FREETYPE_SRC_PATH)src/sfnt/sfobjs.c \
$(FREETYPE_SRC_PATH)src/sfnt/ttcmap.c \
$(FREETYPE_SRC_PATH)src/sfnt/sfdriver.c \
$(FREETYPE_SRC_PATH)src/smooth/smooth.c \
$(FREETYPE_SRC_PATH)src/smooth/ftspic.c \
$(FREETYPE_SRC_PATH)src/truetype/truetype.c \
$(FREETYPE_SRC_PATH)src/type1/t1driver.c \
$(FREETYPE_SRC_PATH)src/cid/cidgload.c \
$(FREETYPE_SRC_PATH)src/cid/cidload.c \
$(FREETYPE_SRC_PATH)src/cid/cidobjs.c \
$(FREETYPE_SRC_PATH)src/cid/cidparse.c \
$(FREETYPE_SRC_PATH)src/cid/cidriver.c \
$(FREETYPE_SRC_PATH)src/pfr/pfr.c \
$(FREETYPE_SRC_PATH)src/pfr/pfrgload.c \
$(FREETYPE_SRC_PATH)src/pfr/pfrload.c \
$(FREETYPE_SRC_PATH)src/pfr/pfrobjs.c \
$(FREETYPE_SRC_PATH)src/pfr/pfrsbit.c \
$(FREETYPE_SRC_PATH)src/type42/t42objs.c \
$(FREETYPE_SRC_PATH)src/type42/t42parse.c \
$(FREETYPE_SRC_PATH)src/type42/type42.c \
$(FREETYPE_SRC_PATH)src/winfonts/winfnt.c \
$(FREETYPE_SRC_PATH)src/pcf/pcfread.c \
$(FREETYPE_SRC_PATH)src/pcf/pcfutil.c \
$(FREETYPE_SRC_PATH)src/pcf/pcfdrivr.c \
$(FREETYPE_SRC_PATH)src/psaux/afmparse.c \
$(FREETYPE_SRC_PATH)src/psaux/psaux.c \
$(FREETYPE_SRC_PATH)src/psaux/psconv.c \
$(FREETYPE_SRC_PATH)src/psaux/psobjs.c \
$(FREETYPE_SRC_PATH)src/psaux/t1decode.c \
$(FREETYPE_SRC_PATH)src/tools/apinames.c \
$(FREETYPE_SRC_PATH)src/type1/t1afm.c \
$(FREETYPE_SRC_PATH)src/type1/t1gload.c \
$(FREETYPE_SRC_PATH)src/type1/t1load.c \
$(FREETYPE_SRC_PATH)src/type1/t1objs.c \
$(FREETYPE_SRC_PATH)src/type1/t1parse.c\
$(FREETYPE_SRC_PATH)src/bdf/bdfdrivr.c\
$(FREETYPE_SRC_PATH)src/bdf/bdflib.c\
$(FREETYPE_SRC_PATH)src/gzip/ftgzip.c\
$(FREETYPE_SRC_PATH)src/lzw/ftlzw.c \

#LOCAL_LDLIBS := -ldl -llog

#include $(BUILD_SHARED_LIBRARY)

include $(BUILD_STATIC_LIBRARY)
