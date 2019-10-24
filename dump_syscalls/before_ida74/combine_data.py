import json
import sys
import os

datas = []

def dump(num, pro, name):
    datas.append([int(num), pro, name])

def main():
    with open('syscalls_1p.json') as p1:
        with open('syscall_prototype_p2.json') as p2:
            syscall_num_data = json.load(p1)
            syscall_pro_data = json.load(p2)
            for k, v in syscall_num_data.items():
                if v.startswith('Zw'):
                    continue
                if v in syscall_pro_data:
                    dump(k, syscall_pro_data[v], v)
                else:
                    print('error : %s' % v)
    global datas
    datas = sorted(datas, key=lambda x : x[0])
    with open('entry.h', 'w') as w:
        for data in datas:
            item = '{0x%x, %d, Hook%s, "%s"},\n' % (data[0], data[1], data[2], data[2])
            w.write(item)

if __name__ == "__main__":
    main()