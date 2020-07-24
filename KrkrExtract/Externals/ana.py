import sys
import os
import subprocess
import platform

path = os.path.dirname(os.path.abspath(__file__))
script_path =  os.path.join(os.path.join(os.path.join(path, 'scripts'), 'r2'), 'find_private_proc.py')
(bits, linker) = platform.architecture()

r2_home = os.path.join(os.path.join(path, 'radare2'), 'bin')
py_path = os.path.join(os.path.join(path, 'python'), 'python.exe')
kr_path = os.path.join(path, 'krkr.exe')
out_path = os.path.join(path, 'out.txt')
if bits.startswith('64'):
    r2_home = os.path.join(os.path.join(path, 'radare2_64'), 'bin')

if not os.path.isfile(script_path):
    print(script_path)
    sys.exit(-1)

if not os.path.isfile(py_path):
    print(py_path)
    sys.exit(-1)

if not os.path.isdir(r2_home):
    print(r2_home)
    sys.exit(-1)

p = subprocess.Popen([py_path, script_path, kr_path, r2_home, out_path])
p.wait()
