import idautils
import idc
import json
import os
from idautils import *
from idaapi import *
from idc import *

def get_prototype(ea):
    tif = tinfo_t()
    if not get_tinfo2(ea, tif):
        return None
    funcdata = func_type_data_t()
    if not tif.get_func_details(funcdata):
        return None
    return funcdata.size()

datas = {}
for segea in Segments():
    for funcea in Functions(segea, SegEnd(segea)):
        functionName = GetFunctionName(funcea)
        if not functionName.startswith('Nt'):
            continue
        p = get_prototype(funcea)
        if p is None:
            continue
        datas[functionName] = p

with open('syscall_prototype_p2.json', 'w') as fd:
    json.dump(datas, fd, indent=4)
