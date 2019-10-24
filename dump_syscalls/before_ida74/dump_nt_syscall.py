import idautils
import idc
import json
import os
from idaapi import *

ea = idc.MinEA()
all = {}
while True:
    ea = idc.FindText(ea, idc.SEARCH_DOWN, 0, 0, "syscall")
    if ea == idaapi.BADADDR:
        break
    func_name = idc.GetFunctionName(ea)
    ins = idc.FindText(ea, idc.SEARCH_UP, 0, 0, "mov")

    syscall_num = idc.GetOpnd(ins, 1)
    if syscall_num.endswith('h'):
        syscall_num = syscall_num[:-1]
    syscall_num = int(syscall_num, 16)
    if syscall_num in all:
        print "XXXXXX prev : %s, curr : %s" % (all[syscall_num], func_name)
    all[syscall_num] = func_name

    ea += len("syscall")

out_path = os.path.join(os.path.expanduser('~'), 'Desktop')
in_file = idc.GetInputFile().replace('.','_')
out_path = 'syscalls_1p.json'
with open(out_path, 'w+') as fp:
	json.dump(all, fp, indent=4)
	
print "Saved %d system calls to %s" % (len(all), out_path)