# Select C++ compiler
#CXX = sxc++
#CXX = g++
CXX = mpicxx # H5public.h may need mpi.h

# Select C compiler
#CC = sxcc
CC = gcc

# Select archiver
#AR = sxar
AR = ar

# C and C++ "compile but do not link" flag
CDNLFLAG = -c

# C++ compiler flags
# Debug C++ flags
#CXXFLAGS = -Wall -D__STDC_LIMIT_MACROS -fsanitize=bounds
#CXXFLAGS = -Wall -D__STDC_LIMIT_MACROS -mmpx -fcheck-pointer-bounds

# Production C++ flags
#CXXFLAGS = -Xp -Kexceptions
#CXXFLAGS = -Caopt -Xp -Kexceptions
CXXFLAGS = -Wall -O3 -D__STDC_LIMIT_MACROS -mavx2

# C compiler flags
#CFLAGS = -Xa
#CFLAGS = -Caopt -Xa
CFLAGS = -Wall -O3 -fomit-frame-pointer -funroll-loops -mavx2

# HDF path (optional)
HDF_INC = $(HDF_ROOT)/include/
HDF_LIB = $(HDF_ROOT)/lib/
