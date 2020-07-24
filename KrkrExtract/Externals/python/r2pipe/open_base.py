# /usr/bin/env python
# -*- coding: utf-8 -*-

"""open_base.py
base class for new open objects from open_sync and open_async. Code derived from __init__.py

"""

import os
import sys
import json
import shutil
import platform

from subprocess import Popen, PIPE

try:
	import r2lang
except ImportError:
	r2lang = None
try:
        from .native import RCore
        has_native = True
except ImportError:
        has_native = False

if os.name == "nt":
	from ctypes import byref, c_ulong, create_string_buffer, windll
	import msvcrt
	GENERIC_READ = 0x80000000
	GENERIC_WRITE = 0x40000000
	OPEN_EXISTING = 0x3
	INVALID_HANDLE_VALUE = -1
	PIPE_READMODE_MESSAGE = 0x2
	ERROR_PIPE_BUSY = 231
	ERROR_MORE_DATA = 234
	BUFSIZE = 4096
	szPipename = "\\\\.\\pipe\\"
	chBuf = create_string_buffer(BUFSIZE)
	cbRead = c_ulong(0)
	cbWritten = c_ulong(0)


def in_rlang():
	return r2lang is not None and r2lang.cmd is not None

def jo2po(jo):
	from collections import namedtuple
	def _json_object_hook(d): return namedtuple('X', d.keys(), rename=True)(*d.values())
	def json2obj(data): return json.loads(data, object_hook=_json_object_hook)
	return json2obj(jo)

def get_radare_path():
	try:
		which = shutil.which
	except AttributeError:
		# In python 2 doesn't exist which function
		from distutils.spawn import find_executable

		which = find_executable

	bin_file = which("radare2")

	if bin_file and os.path.isfile(bin_file):
		return bin_file
	else:
		_platform = platform.system().lower()

		if _platform.startswith("darwin"):
			bin_file = "/usr/local/bin/radare2"
		else:
			bin_file = "/usr/bin/radare2"

		if os.path.isfile(bin_file):
			return bin_file
		else:
			raise IOError("radare2 can't be found in your system")


class OpenBase(object):
        """Class representing an r2pipe connection with a running radare2 instance
        Class body derived from __init__.py "open" class.
    
    
        """
        def __init__(self, filename='', flags=[]):
                """Open a new r2 pipe
                The 'filename' can be one of the following:

                * absolute or relative path to file
                * http://<host>:<port> to connect to an r2 webserver
                * tcp://<host>:<port> to connect to an r2 tcp server
                * #!pipe when launching it from r2 via RLang.pipe

                Args:
                    filename (str): path to filename or uri
                    flags (list of str): arguments, either in comapct form
                        ("-wdn") or sepparated by commas ("-w","-d","-n")
                Returns:
                    Returns an object with methods to interact with r2 via commands
                """
                self.asyn = False
                if not filename and in_rlang():
                        self._cmd = self._cmd_rlang
                        return
                try:
                        if os.name == "nt":
                                mypipename = os.environ['r2pipe_path']
                                while 1:
                                        hPipe = windll.kernel32.CreateFileA(szPipename + mypipename, GENERIC_READ | GENERIC_WRITE, 0, None, OPEN_EXISTING, 0, None)
                                        if (hPipe != INVALID_HANDLE_VALUE):
                                                break
                                        else:
                                                print("Invalid Handle Value")
                                        if (windll.kernel32.GetLastError() != ERROR_PIPE_BUSY):
                                                print("Could not open pipe")
                                                return
                                        elif ((windll.kernel32.WaitNamedPipeA(szPipename, 20000)) == 0):
                                                print("Could not open pipe\n")
                                                return
                                windll.kernel32.WriteFile(hPipe, "e scr.color=false\n", 18, byref(cbWritten), None)
                                windll.kernel32.ReadFile(hPipe, chBuf, BUFSIZE, byref(cbRead), None)
                                self.pipe = [hPipe, hPipe]
                                self._cmd = self._cmd_pipe
                        else:
                                self.pipe = [int(os.environ['R2PIPE_IN']), int(os.environ['R2PIPE_OUT'])]
                                self._cmd = self._cmd_pipe
                        self.url = "#!pipe"
                        return
                except:
                        pass
                if filename.startswith("#!pipe"):
                        raise Exception("ERROR: Cannot use #!pipe without R2PIPE_{IN|OUT} env")


        def _cmd_pipe(self, cmd):
                out = b''
                cmd = cmd.strip().replace("\n", ";")
                if os.name == "nt":
                        windll.kernel32.WriteFile(self.pipe[1], cmd, len(cmd), byref(cbWritten), None)
                        while True:
                                windll.kernel32.ReadFile(self.pipe[1], chBuf, BUFSIZE, byref(cbRead), None)
                                out += chBuf.value
                                if ord(chBuf[cbRead.value - 1]) == 0:
                                        out = out[0:-1]
                                        break
                else:
                        os.write(self.pipe[1], cmd.encode())
                        while True:
                                res = os.read(self.pipe[0], 4096)
                                if (len(res) < 1):
                                        break
                                if res[-1] == b'\x00'[0]:
                                        out += res[0:-1]
                                else:
                                        out += res
                                if (len(res) < 4096):
                                        break
                return out.decode('utf-8')

        def _cmd_native(self, cmd):
                cmd = cmd.strip().replace("\n", ";")
                if not has_native:
                        raise Exception('No native ctypes connector available')
                if not hasattr(self, 'native'):
                        self.native = RCore()
                        self.native.cmd_str("o " + self.uri)
                return self.native.cmd_str(cmd)

        def _cmd_rlang(self, cmd):
                return r2lang.cmd(cmd)
     
        def quit(self):
                """Quit current r2pipe session and kill
                """
                self.cmd("q")
                if hasattr(self, 'process'):
                        import subprocess
                        is_async = not isinstance(self.process, subprocess.Popen)
                        if not is_async:
                                for f in [self.process.stdin, self.process.stdout]:
                                        if f is not None:
                                                f.close()
                        self.process.terminate()
                        self.process.wait()
                        delattr(self, 'process')

                        if is_async:
                                import asyncio
                                asyncio.get_event_loop().run_until_complete(asyncio.sleep(0.1))

        # r2 commands
        def cmd(self, cmd, **kwargs):
                """Run an r2 command return string with result
                Args:
                    cmd (str): r2 command
                Returns:
                    Returns an string with the results of the command
                
                res = self._cmd(cmd)
                if res is not None:
                    return res.strip()
                return None
                """
                
                res = self._cmd(cmd, **kwargs)
                if res is not None:
                        return res
                return None
            
        def cmdj(self, cmd, **kwargs):
                """Same as cmd() but evaluates JSONs and returns an object
                Args:
                    cmdj (str): r2 command
                Returns:
                    Returns a JSON object respresenting the parsed JSON
                """
                result = self.cmd(cmd, **kwargs)

                try:
                        data = json.loads(result)
                except (ValueError, KeyError, TypeError) as e:
                        sys.stderr.write("r2pipe.cmdj.Error: %s\n" % (e))
                        data = None
                return data

        def cmdJ(self, cmd, **kwargs):
                """Same as cmdj() but evaluates into a native Python Object
                Args:
                    cmdJ (str): r2 command
                Returns:
                    Returns a Python object respresenting the parsed JSON
                """
                result = self.cmd(cmd, **kwargs)
                try:
                        return jo2po(result)
                except (ValueError, KeyError, TypeError) as e:
                        sys.stderr.write("r2pipe.cmdj.Error: %s\n" % (e))
                return None

        def syscmd(self, cmd):
                """Executes a program and returns the output (stdout only)
                Args:
                    cmd (str): commandline shell command
                Returns:
                    Returns a string with the output
                """
                p = Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE)
                out, err = p.communicate()
                return out

        def syscmdj(self, cmd):
                """Executes a program and returns an object representing the parsed JSON of the output
                Args:
                    cmd (str): commandline shell command
                Returns:
                    Returns an object constructed by parsing the JSON returned by the command
                """
                try:
                        data = json.loads(self.syscmd(cmd))
                except (ValueError, KeyError, TypeError) as e:
                        sys.stderr.write("r2pipe.syscmdj.Error %s\n" % (e))
                        data = None
                return data

