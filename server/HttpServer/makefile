#-----------------------------------------------------------------------

APP       := otim
TARGET    := HttpServer
CONFIG    :=
STRIP_FLAG:= N
TARS2CPP_FLAG:=

INCLUDE   += -I../Common -I../CommonTars -I../third/include
LIB       += -ldl -luuid ../Common/libCommon.a ../third/lib/libhiredis.a ../third/lib/libhiredis_ssl.a ../third/lib/libssl.a ../third/lib/libcrypto.a
CFLAGS    += -DRAPIDJSON_HAS_STDSTRING
CFLAGS += -std=c++11 -ggdb3

#-----------------------------------------------------------------------

include /usr/local/tars/cpp/makefile/makefile.tars

ifneq ("$(MAKECMDGOALS)","release")
	include $(HOME)/tarsproto/otim/BrokerServer/BrokerServer.mk
	include $(HOME)/tarsproto/otim/GroupChatServer/GroupChatServer.mk
	include $(HOME)/tarsproto/otim/UserFriendServer/UserFriendServer.mk
endif

#-----------------------------------------------------------------------
