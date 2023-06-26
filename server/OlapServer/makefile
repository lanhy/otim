#-----------------------------------------------------------------------

APP       := otim
TARGET    := OlapServer
CONFIG    :=
STRIP_FLAG:= N
TARS2CPP_FLAG:=

INCLUDE   += -I../Common -I../CommonTars -I../third/include
LIB       += -ldl ../Common/libCommon.a ../third/lib/libhiredis.a ../third/lib/libhiredis_ssl.a ../third/lib/libssl.a ../third/lib/libcrypto.a

#-----------------------------------------------------------------------

include /usr/local/tars/cpp/makefile/makefile.tars

#-----------------------------------------------------------------------
