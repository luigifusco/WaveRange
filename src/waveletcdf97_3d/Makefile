include ../../config.mk

OBJECTC = waveletcdf97_3d.o
CSOURCES = waveletcdf97_3d.cpp

ifeq ($(CXX),g++)
  CPICFLAG = -fPIC
else
  CPICFLAG =
endif

all: waveletcdf

waveletcdf: 
	$(CXX) $(CSOURCES) $(CDNLFLAG) $(CPICFLAG) $(CFLAGS) 
.PHONY: clean
clean:
	$(RM) ./*.gc??
	$(RM) ./*.o
