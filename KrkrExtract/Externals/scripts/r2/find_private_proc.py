import os
import sys
import pprint
import json
import enum
import r2pipe


class FindStatus(enum.IntEnum):
    FOUND_FAKE = 0,
    FOUND_CMP_FLAG0 = 1,
    FOUND_JNZ_ADDR  = 2,
    FOUND_MOV_FLAG1 = 3,
    FOUND_RET = 4

def get_private_addr(r):
    r.cmd('aaa')
    functions = r.cmdj('aflj')
    for function in functions:
        blocks = r.cmdj("afbj %d" % function['offset'])
        if len(blocks) > 5:
            continue

        size = function['size']
        if size <= 0:
            continue

        #print('addr : ', hex(function['offset']))
        addr = function['offset']
        offset_in_func = 0
        opcodes = []
        opcode_map = {}
        while offset_in_func < size:
            r.cmd('s %d' % (offset_in_func + addr))
            fetched_opcodes = r.cmdj('aoj 1')
            if fetched_opcodes is None or len(fetched_opcodes) == 0:
                break
            
            opcode_map[offset_in_func + addr] = fetched_opcodes[0]
            opcode = fetched_opcodes[0]
            opcodes.append(opcode)
            instr_size = opcode['size']
            offset_in_func += instr_size
        
        if len(opcodes) < 3:
            print('opcodes size')
            continue
        
        opcode_0 = opcodes[0]
        if opcode_0['mnemonic'] != 'push':
            print('no push')
            continue

        opcode_1 = opcodes[1]
        if opcode_1['mnemonic'] != 'cmp':
            print('no cmp')
            continue

        if opcode_1['bytes'].startswith('803d') ==  False:
            print('cmp : invalid mode')
            continue

        if opcode_1['val'] != 0:
            print('cmp : invalid imm')
            continue

        flag_addr = opcode_1['ptr']
        status = FindStatus.FOUND_CMP_FLAG0
        func_end_addr = addr + size
        i = 2
        while i < len(opcodes):
            goto_addr = -1
            opcode = opcodes[i]
            if opcode['mnemonic'] in ('jnz', 'jne'):
                goto_addr = opcode['jump']
                if goto_addr < addr or goto_addr >= func_end_addr:
                    print('invalid addr')
                    break

                status = FindStatus.FOUND_JNZ_ADDR
                if i + 1 >= len(opcodes):
                    print('size is too long')
                    break
                i += 1
                
                opcode = opcodes[i]
                if opcode['mnemonic'] != 'mov':
                    print('no mov')
                    break

                if opcode['bytes'].startswith('c605') ==  False:
                    print('invalid mov')
                    break

                if opcode['ptr'] != flag_addr or opcode['val'] != 1:
                    print('invalid addr or flag')
                    print(flag_addr == opcode['ptr'], opcode['val'] == 1)
                    break

                status = FindStatus.FOUND_MOV_FLAG1
                if goto_addr not in opcode_map:
                    print('no mov2')
                    break

                if i + 1 >= len(opcodes):
                    print('size is too long')
                    break
                i += 1

                opcode = opcodes[i]
                if opcode['mnemonic'] != 'call':
                    print('no call')
                    break

                status = FindStatus.FOUND_RET
                return addr
                    
            i += 1

    return None

if __name__ == '__main__':
    if len(sys.argv) < 4:
        sys.exit(-1)
    r = r2pipe.open(sys.argv[1], radare2home=sys.argv[2])
    func_addr = get_private_addr(r)
    with open(sys.argv[3], 'w') as fd:
        if func_addr is None:
            fd.write('-1')
        else:
            fd.write(str(func_addr))
    r.quit()
    sys.exit(0)
