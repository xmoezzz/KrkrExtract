import os
import sys
import abc
import enum
import argparse
import pprint
import json
import typing
import logging
from ctypes import c_uint, c_uint16, c_uint32, LittleEndianStructure, sizeof


logger = logging.getLogger(__file__)

"""
// offset : 0, size : 0x28
struct amv_hdr {
    uint32_t magic; //0x4D504A41
    uint32_t size_of_file; //can be zero, ignore
    uint32_t revision; //must be zero
    uint32_t quantaization_table_size_plus_hdr_size; // quantaization table size, including hdr size(0x28), must be 128 or 192(jpeg)
    uint32_t unk;
    uint32_t frame_cnt;
    uint32_t unk2;
    uint32_t frame_rate; // frames per sec
    uint16_t width;
    uint16_t height;
    uint32_t alpha_decode_attr; //must be 1(jpeg, sub_10017570) or 2(zlib, sub_10016D20)
};
"""

class AmvHeader(LittleEndianStructure):
    _fields_ = [
        ('magic',        c_uint32),
        ("size_of_file", c_uint32),
        ("revision",     c_uint32),
        ("quantaization_table_size_plus_hdr_size", c_uint32),
        ("unk",       c_uint32),
        ("frame_cnt", c_uint32),
        ("unk2",      c_uint32),
        ("frame_rate",c_uint32),
        ("width",     c_uint16),
        ("height",    c_uint16),
        ("alpha_decode_attr", c_uint32)
    ]
    
    
"""
// offset : sizeof(amv_hdr), size : quantaization_table_size_plus_hdr_size - sizeof(amv_hdr)
uint8_t quantaization_table[quantaization_table_size_plus_hdr_size - sizeof(amv_hdr)]

// offset : quantaization_table_size_plus_hdr_size, ...
// count : frame_cnt
// for each frame: (depends on alpha_decode_attr)
struct frame_hdr {
    uint32_t magic; //0x4D415246
    uint32_t size_of_frame; // excluding this header
    uint32_t index_of_cur_frame; // increasing from zero
}
"""
    
class FrameHeader(LittleEndianStructure):
    _fields_ = [
        ("magic",         c_uint32),
        ("size_of_frame", c_uint32),
        ("index_of_cur_frame", c_uint32)
    ]
    
"""
struct jpeg_frame_hdr : frame_hdr {
    uint16_t frame_width;
    uint16_t frame_height;
    uint16_t alpha_channel_width;
    uint16_t alpha_channel_height;
}

uint8_t frame[size_of_frame - (sizeof(jpeg_frame_hdr) - sizeof(frame_hdr))]
"""
    
class JpegFrameHeader(LittleEndianStructure):
    _fields_ = [
        ("magic",         c_uint32),
        ("size_of_frame", c_uint32),
        ("index_of_cur_frame", c_uint32),
        ("frame_width",  c_uint16),
        ("frame_height", c_uint16),
        ("alpha_channel_width",  c_uint16),
        ("alpha_channel_height", c_uint16)
    ]
    
"""
// OR

// in zlib case:
// alpha_channel_size = size_of_frame - rgb_buffer_size - 16

struct zlib_frame_hdr {
    uint16_t frame_width;
    uint16_t frame_height;
    uint16_t alpha_channel_width;
    uint16_t alpha_channel_height;
    uint32_t rgb_buffer_size;
}

uint8_t frame[size_of_frame - (sizeof(zlib_frame_hdr) - sizeof(frame_hdr))]
"""

class ZlibFrameHeader(LittleEndianStructure):
    _fields_ = [
        ("magic",         c_uint32),
        ("size_of_frame", c_uint32),
        ("index_of_cur_frame", c_uint32),
        ("frame_width",  c_uint16),
        ("frame_height", c_uint16),
        ("alpha_channel_width",  c_uint16),
        ("alpha_channel_height", c_uint16),
        ("agb_buffer_size",      c_uint32)
    ]
    
    
class AmvAlphaChannelAttribute(enum.IntEnum):
    ALPHA_JPEG = 1
    ALPHA_ZLIB = 2
    
class Frame(abc.ABC):
    @abc.abstractmethod
    def decode(self, *args, **kwargs)->bytes:
        raise NotImplementedError()
    
    @abc.abstractmethod
    def info(self, *args, **kwargs)->dict:
        raise NotImplementedError()
    
    @abc.abstractmethod
    def width(self)->int:
        raise NotImplementedError()
    
    @abc.abstractmethod
    def height(self)->int:
        raise NotImplementedError()
    

class JpegFrame(Frame):
    def __init__(self, hdr : JpegFrameHeader, blob : bytes):
        self.hdr  = hdr
        self.data = blob
        
    def decode(self, *args, **kwargs)->bytes:
        pass
    
    def info(self)->dict:
        return dict((field, getattr(self.hdr, field)) for field, _ in self.hdr._fields_)
    
    def width(self)->int:
        return self.hdr.frame_width
    
    def height(self)->int:
        return self.hdr.frame_height
    
class ZlibFrame(Frame):
    def __init__(self, hdr : ZlibFrameHeader, blob : bytes) -> None:
        self.hdr  = hdr
        self.data = blob
        
    def decode(self, *args, **kwargs)->bytes:
        pass
    
    def info(self)->dict:
        return dict((field, getattr(self.hdr, field)) for field, _ in self.hdr._fields_)
    
    def width(self)->int:
        return self.hdr.frame_width
    
    def height(self)->int:
        return self.hdr.frame_height
    
    
class AlphaMovieDecoder(object):
    def __init__(self, location : str) -> None:
        self.frames = []
        self.hdr = None
        self.quantaization_table = None
        self.quantaization_table_size = None
        
        if not os.path.exists(location):
            raise RuntimeError("no such file : %s" % location)
        
        if not os.path.isfile(location):
            raise RuntimeError("not a file : %s" % location)
        
        self.__parse(location)
    
    def __check_hdr(self, hdr : AmvHeader)->bool:
        if hdr.magic != 0x4D504A41:
            raise RuntimeError("unsupported amv header magic")
        if hdr.alpha_decode_attr not in \
            (AmvAlphaChannelAttribute.ALPHA_JPEG, AmvAlphaChannelAttribute.ALPHA_ZLIB):
            raise RuntimeError("unsupported alphe decoding attribute : %d", hdr.alpha_decode_attr)
        quantaization_table_size = hdr.quantaization_table_size_plus_hdr_size \
            - sizeof(AmvHeader)
        if quantaization_table_size not in (128, 192):
            raise RuntimeError("unsupported quantaization table size : %d" % quantaization_table_size)
        self.quantaization_table_size = quantaization_table_size
        return True
    
    def __check_frame_hdr(self, hdr)->bool:
        if hdr.magic != 0x4D415246:
            print(hex(hdr.magic))
            raise RuntimeError("unsupported amv frame header magic")
        return True
    
    def __parse_jpeg_frame(self, fp)->JpegFrame:
        fhdr = JpegFrameHeader()
        buf = fp.read(sizeof(fhdr))
        if len(buf) != sizeof(fhdr):
            raise RuntimeError("encountered unexpected eof while parsing frame header (index : %d)" % i)
        fhdr = JpegFrameHeader.from_buffer_copy(buf)
        self.__check_frame_hdr(fhdr)
        
        """ forgive me hardcoding here """
        total_size = fhdr.size_of_frame - 0xC
        blob = fp.read(total_size)
        if len(blob) != total_size:
            raise RuntimeError("encountered unexpected eof while reading frame content (index : %d)" % i)
        return JpegFrame(fhdr, blob)

    def __parse_zlib_frame(self, fp)->ZlibFrame:
        fhdr = ZlibFrameHeader()
        buf = fp.read(sizeof(fhdr))
        if len(buf) != sizeof(fhdr):
            raise RuntimeError("encountered unexpected eof while parsing frame header (index : %d)" % i)
        fhdr = ZlibFrameHeader.from_buffer_copy(buf)
        self.__check_frame_hdr(fhdr)
        total_size = fhdr.size_of_frame - 0x10
        blob = fp.read(total_size)
        if len(blob) != total_size:
            raise RuntimeError("encountered unexpected eof while reading frame content (index : %d)" % i)
        return ZlibFrame(fhdr, blob)
            
    def __parse(self, location : str)->bool:
        with open(location, "rb") as fp:
            hdr = AmvHeader()
            buf = fp.read(sizeof(hdr))
            if len(buf) != sizeof(hdr):
                raise RuntimeError("encountered unexpected eof while parsing amv header")
            hdr = AmvHeader.from_buffer_copy(buf)
            self.__check_hdr(hdr)
            self.hdr = hdr
            self.quantaization_table = fp.read(self.quantaization_table_size)
            for i in range(self.hdr.frame_cnt):
                if self.hdr.alpha_decode_attr == AmvAlphaChannelAttribute.ALPHA_JPEG:
                    frame = self.__parse_jpeg_frame(fp)
                    self.frames.append(frame)
                else:
                    frame = self.__parse_zlib_frame(fp)
                    self.frames.append(frame)
        
        return True
    
    def info(self)->dict:
        if self.hdr is None:
            return None
        return {
            "hdr" : dict((field, getattr(self.hdr, field)) for field, _ in self.hdr._fields_),
            "frames" : [frame.info() for frame in self.frames]
        }
    
    def decode(self):
        OUTPUT_PATH = "./frame"
        if not os.path.exists(OUTPUT_PATH):
            os.mkdir("./frame")
            
        for i, frame in enumerate(self.frames):
            path = os.path.join(OUTPUT_PATH, "%d.bin" % i)
            with open(path, "wb") as fp:
                fp.write(frame.data)
            
        
        
if __name__ == "__main__": 
    p = AlphaMovieDecoder(sys.argv[1])
    #pprint.pprint(p.info())
    p.decode()
        