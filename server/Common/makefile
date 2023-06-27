#-----------------------------------------------------------------------
LIBCOMMON = libCommon.a


TARGET := ${LIBCOMMON}

#
#APP       := otimim
#TARGET    := tars
CONFIG    :=
STRIP_FLAG:= N
TARS2CPP_FLAG:=
CFLAGS    += -DRAPIDJSON_HAS_STDSTRING

#-------------------------------------------------------------------------------

#LIB     +=  -lhiredis -luuid  ../thrid/lib/libssl.a ../thrid/lib/libcrypto.a -L../../lib -lmsgstructpb -lprotobuf -lcommonjce -L/usr/lib64/mysql -lmysqlclient
#
INCLUDE    += -I/usr/include -I../CommonTars -I../third/include -I../third/include/mysql
#-------------------------------------------------------------------------------

#-----------------------------------------------------------------------

include /usr/local/tars/cpp/makefile/makefile.tars
ifneq ("$(MAKECMDGOALS)","release")
	include $(HOME)/tarsproto/otim/BrokerServer/BrokerServer.mk
endif

#-----------------------------------------------------------------------
