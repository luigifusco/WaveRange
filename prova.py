from lib import LIBWAVERANGE, NLAYMAX

nx = 100
ny = 721
nz = 1440

data = (np.arange(nz*ny*nx).reshape(nz, ny, nx) / (nz*ny*nx)).astype(np.float32)
output = np.zeros_like(data, dtype=np.float32)

ntot_enc_max = NLAYMAX * max(1024, np.prod(data.shape))

tolabs = ctypes.c_float()
midval = ctypes.c_float()
halfspanval = ctypes.c_float()
wlev = ctypes.c_uint8()
nlay = ctypes.c_uint8()
ntot_enc = ctypes.c_ulong()
cutoffval = ctypes.c_float(100000000)
deps_vec = np.zeros(NLAYMAX, dtype=np.float32)
minval_vec = np.zeros(NLAYMAX, dtype=np.float32)
len_enc_vec = np.zeros(NLAYMAX, dtype=np.uint64)
data_enc = np.zeros(ntot_enc_max, dtype=np.uint8)

original = data.copy()

LIBWAVERANGE.encoding_wrap_float(nx, ny, nz, data, 1, 1, 1, 1, ctypes.pointer(cutoffval), ctypes.byref(tolabs),
        ctypes.byref(midval), ctypes.byref(halfspanval), ctypes.byref(wlev), ctypes.byref(nlay), ctypes.byref(ntot_enc), deps_vec, minval_vec, len_enc_vec, data_enc)

data_enc = data_enc[:ntot_enc.value]

with open('compressed.wr', 'wb') as file:
    pickle.dump((midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc), file)

LIBWAVERANGE.decoding_wrap_float(nx, ny, nz, output, ctypes.byref(tolabs),
        ctypes.byref(midval), ctypes.byref(halfspanval), ctypes.byref(wlev), ctypes.byref(nlay), ctypes.byref(ntot_enc), deps_vec, minval_vec, len_enc_vec, data_enc)

print(f'compressed to {ntot_enc.value} (from {original.nbytes}, ratio of {original.nbytes / ntot_enc.value})')
print(np.sum(np.abs(output - original)) / np.sum(original))