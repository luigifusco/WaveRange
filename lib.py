import ctypes
import numpy as np
from numpy.ctypeslib import ndpointer
import pickle

NLAYMAX = 8
LIBWAVERANGE = ctypes.CDLL('/users/lfusco/code/WaveRange/bin/lib/libwaverange.so')

# setting up declaration
LIBWAVERANGE.encoding_wrap_float.restype = None
LIBWAVERANGE.encoding_wrap_float.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ndpointer(ctypes.c_float, flags="C_CONTIGUOUS"),
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.POINTER(ctypes.c_ulong),
    ndpointer(ctypes.c_float, flags="C_CONTIGUOUS"),
    ndpointer(ctypes.c_float, flags="C_CONTIGUOUS"),
    ndpointer(ctypes.c_ulong, flags="C_CONTIGUOUS"),
    ndpointer(ctypes.c_uint8, flags="C_CONTIGUOUS")]

LIBWAVERANGE.decoding_wrap_float.restype = None
LIBWAVERANGE.decoding_wrap_float.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ndpointer(ctypes.c_float, flags="C_CONTIGUOUS"),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.POINTER(ctypes.c_ulong),
    ndpointer(ctypes.c_float, flags="C_CONTIGUOUS"),
    ndpointer(ctypes.c_float, flags="C_CONTIGUOUS"),
    ndpointer(ctypes.c_ulong, flags="C_CONTIGUOUS"),
    ndpointer(ctypes.c_uint8, flags="C_CONTIGUOUS")]