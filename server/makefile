
#include /usr/local/tars/cpp/makefile/makefile.tars

DIRS = Common BrokerServer GroupChatServer HistoryMsgServer AuthServer UserFriendServer HttpServer SingleChatServer MsgOperatorServer OlapServer BizMsgServer
GEN_TARS_DIRS = CommonTars BrokerServer AuthServer SingleChatServer OlapServer GroupChatServer

all:
	cd CommonTars;  make;
	@for dir in $(DIRS); do  make -j16 -C $$dir || exit $$?; echo; done
tar:
	@for dir in $(DIRS); do make -C $$dir tar; echo; done

cleanall:
	@for dir in $(DIRS); do make -C $$dir cleanall; echo; done
clean:
	rm -f lib/*
	rm bin -rf
	@for dir in $(DIRS); do make -C $$dir clean; rm -f bin/$$dir; echo; done
	find ./ -name "*.d" | xargs rm -f
	find ./ -name "*.gcda" -o -name "*.gcno" | xargs rm -f

cleanbin:
	@for dir in $(DIRS); do rm -f bin/$$dir; echo; done

sz:
	@for dir in $(DIRS); do  sz $$dir/*.tgz; echo; done

cp:
	mkdir bin
	@for dir in $(DIRS); do  cp $$dir/*.tgz bin/; echo; done

release:
	mkdir -p $(HOME)/tarsproto/otim
	@for dir in $(GEN_TARS_DIRS); do mkdir -p $(HOME)/tarsproto/otim/$$dir; echo; done
	@for dir in $(GEN_TARS_DIRS); do make release -C $$dir || exit $$?; echo; done

