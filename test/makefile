
THIRDPATH  := ../server/third
TARSPATH := /usr/local/tars/cpp
#----------------------------------------------------------------------

APP       := otim
TARGET    := test
CONFIG    :=
STRIP_FLAG:= N
TARS2CPP_FLAG:=
CFLAGS    += -std=c++11 -ggdb3 -Wsign-compare



INCLUDE    += -I$(THIRDPATH)
INCLUDE    += -I./gtest/include -I../server/CommonTars
INCLUDE    += -I../server/Common -I$(THIRDPATH)/include 

LIB    += -L../server/Common -lCommon
LIB    += -L$(TARSPATH)/lib -ltarsutil -ltarsparse -ltarsservant
LIB    += $(THIRDPATH)/lib/libhiredis.a  $(THIRDPATH)/lib/libssl.a $(THIRDPATH)/lib/libcrypto.a  ${THIRDPATH}/lib/libmysqlclient.a  
LIB    += -L${THIRDPATH}/lib  -luuid -lrt -ldl -lz
LIB    += -L./lib -lgtest -lpthread

#-----------------------------------------------------------------------

include /usr/local/tars/cpp/makefile/makefile.tars
#-----------------------------------------------------------------------
