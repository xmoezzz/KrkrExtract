from idautils import *
from idaapi import *
from idc import *
import enum

class FindStatus(enum.IntEnum):
    FOUND_FAKE = 0,
    FOUND_CMP_FLAG0 = 1,
    FOUND_JNZ_ADDR  = 2,
    FOUND_MOV_FLAG1 = 3,
    FOUND_RET = 4

def find_func():
    for segea in Segments():
        for funcea in Functions(segea, SegEnd(segea)):
            functionName = GetFunctionName(funcea)
            function = idaapi.get_func(funcea)
            flowchart = idaapi.FlowChart(function)

            cnt = 0
            for bb in flowchart:
                cnt += 1
            
            if cnt > 5:
                continue

            if print_insn_mnem(funcea) != 'push':
                continue

            if ItemSize(funcea) != 1:
                continue

            if print_insn_mnem(funcea + 1) != 'cmp':
                continue

            buf = GetManyBytes(funcea + 1, 2)
            if buf[0] != '\x80' or buf[1] != '\x3D':
                continue

            flag_addr    = get_operand_value(funcea + 1, 0)
            flag_cmp_int = get_operand_value(funcea + 1, 1)

            if flag_cmp_int != 0:
                continue

            print('func : ', hex(funcea))
            endea = FindFuncEnd(funcea)
            offset_in_func = ItemSize(funcea + 1)
            status = FindStatus.FOUND_CMP_FLAG0
            while offset_in_func + funcea < endea:
                if print_insn_mnem(funcea + offset_in_func) == 'jnz':
                    flag_offset = get_operand_value(funcea + offset_in_func, 0)
                    print(hex(funcea + offset_in_func), GetDisasm(funcea + offset_in_func), hex(flag_offset))
                    if flag_offset < funcea or flag_offset >= endea:
                        print('invalid addr')
                        break

                    status = FindStatus.FOUND_JNZ_ADDR
                    offset_in_func += ItemSize(funcea + offset_in_func)
                    if offset_in_func + funcea >= endea:
                        print('size is too long')
                        break

                    mnem = print_insn_mnem(funcea + offset_in_func)
                    if mnem != 'mov':
                        print('no mov')
                        break

                    buf = GetManyBytes(funcea + offset_in_func, 2)
                    if buf[0] != '\xC6' or buf[1] != '\x05':
                        print('invalid mov')
                        break

                    addr2 = get_operand_value(funcea + offset_in_func, 0)
                    flag2 = get_operand_value(funcea + offset_in_func, 1)
                    if addr2 != flag_addr or flag2 != 1:
                        print('invalid addr or flag')
                        print(hex(funcea + offset_in_func), GetDisasm(funcea + offset_in_func))
                        print(flag_addr == addr2, flag2 == 1)
                        break

                    status = FindStatus.FOUND_MOV_FLAG1
                    if print_insn_mnem(flag_offset) != 'mov':
                        print('no mov2')
                        print(hex(flag_offset), GetDisasm(flag_offset))
                        break

                    offset_in_func += ItemSize(funcea + offset_in_func)
                    if offset_in_func + funcea >= endea:
                        print('size is too long')
                        break

                    mnem = print_insn_mnem(funcea + offset_in_func)
                    if mnem != 'call':
                        print('no call')
                        break

                    status = FindStatus.FOUND_RET
                    return funcea
                    
                offset_in_func += ItemSize(funcea + offset_in_func)

    return None


ea = find_func()
if ea:
    print(hex(ea))