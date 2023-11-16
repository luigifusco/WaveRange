include ../../config.mk

OBJECTC = rangecod.o 
CSOURCES = rangecod.cpp

ifeq ($(CXX),g++)
  CPICFLAG = -fPIC
else
  CPICFLAG = 
endif

all: rangecoder

rangecoder: 
	$(CXX) $(CSOURCES) $(CDNLFLAG) $(CPICFLAG) $(CFLAGS) 
.PHONY: clean
clean:
	$(RM) ./*.gc??
	$(RM) ./*.o
