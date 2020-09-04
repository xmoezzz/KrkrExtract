#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from ctypes import Structure, addressof, c_char_p, c_void_p
from ctypes.util import find_library

try:
	from ctypes import CDLL
except:
	pass

try:
	from ctypes import WinDLL
except:
	pass

lib_name = find_library('r_core')
if lib_name == None:
	raise ImportError("No native r_core library")

if sys.platform.startswith('win'):
	lib = WinDLL(lib_name)
else:
	lib = CDLL(lib_name)


class AddressHolder(object):
	def __get__(self, obj, type_):
		if getattr(obj, '_address', None) is None:
			obj._address = addressof(obj)
		return obj._address

	def __set__(self, obj, value):
		obj._address = value


class WrappedRMethod(object):
	def __init__(self, cname, args, ret):
		self.cname = cname
		self.args = args
		self.ret = ret
		self.args_set = False
		self.method = getattr(lib, cname)

	def __call__(self, *a):
		if not self.args_set:
			if self.args:
				self.method.argtypes = [eval(x.strip()) for x in self.args.split(',')]
			self.method.restype = eval(self.ret) if self.ret else None
			self.args_set = True
		a = list(a)
		for i, argt in enumerate(self.method.argtypes):
			if argt is c_char_p:
				a[i] = a[i].encode()
		if self.method.restype is c_char_p:
			return self.method(*a).decode()
		return self.method(*a)


class WrappedApiMethod(object):
	def __init__(self, method, ret2, last):
		self.method = method
		self._o = None
		self.ret2 = ret2
		self.last = last

	def __call__(self, *a):
		result = self.method(self._o, *a)
		if self.ret2:
			if self.ret2 == 'c_char_p':
				return result
			else:
				result = eval(self.ret2)(result)
		if self.last:
			return getattr(result, self.last)
		return result

	def __get__(self, obj, type_):
		self._o = obj._o
		return self


def register(cname, args, ret):
	ret2 = last = None
	if ret:
		if ret[0] >= 'A' and ret[0] <= 'Z':
			x = ret.find('<')
			if x != -1:
				ret = ret[0:x]
			last = 'contents'
			ret = 'POINTER(' + ret + ')'
		else:
			last = 'value'
			ret2 = ret

	method = WrappedRMethod(cname, args, ret)
	wrapped_method = WrappedApiMethod(method, ret2, last)
	return wrapped_method, method


class RCore(Structure):  # 1
	def __init__(self):
		Structure.__init__(self)
		r_core_new = lib.r_core_new
		r_core_new.restype = c_void_p
		self._o = r_core_new()

	_o = AddressHolder()

	cmd_str, r_core_cmd_str = register('r_core_cmd_str', 'c_void_p, c_char_p', 'c_char_p')
	free, r_core_free = register('r_core_free', 'c_void_p', 'c_void_p')

#  c = RCore()
#  c.cmd_str("o /bin/ls")
#  print(c)
#  print(c.cmd_str("s entry0;pd 20"))
#  c.free();
