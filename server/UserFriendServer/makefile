#-----------------------------------------------------------------------

APP       := otim
TARGET    := UserFriendServer
CONFIG    :=
STRIP_FLAG:= N
TARS2CPP_FLAG:=

INCLUDE   += -I../Common -I../CommonTars -I../third/include
LIB       += -ldl -luuid ../Common/libCommon.a ../third/lib/libhiredis.a ../third/lib/libhiredis_ssl.a ../third/lib/libssl.a ../third/lib/libcrypto.a

#-----------------------------------------------------------------------

include /usr/local/tars/cpp/makefile/makefile.tars
ifneq ("$(MAKECMDGOALS)","release")
	include $(HOME)/tarsproto/otim/BrokerServer/BrokerServer.mk
	include $(HOME)/tarsproto/otim/OlapServer/OlapServer.mk
endif
#-----------------------------------------------------------------------
