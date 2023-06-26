#set(CMAKE_SYSTEM_NAME Android)
#set(CMAKE_SYSTEM_VERSION 21) # API level
#set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
#set(CMAKE_ANDROID_NDK /Users/lanhy/Library/Android/sdk/ndk-bundle/)
#set(CMAKE_ANDROID_STL_TYPE gnustl_static)

project(OMTP)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(ANDROID)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROID")
 	message("-- CMAKE_CXX_FLAGS -DANDROID")
endif()



INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/../dest/include)

AUX_SOURCE_DIRECTORY(./src DIR_SRCS)

IF (WIN32)  
    AUX_SOURCE_DIRECTORY(./src/win32 DIR_SRCS)
ELSE()  
    AUX_SOURCE_DIRECTORY(./src/unix DIR_SRCS)
ENDIF ()  

ADD_LIBRARY(OMTP STATIC ${DIR_SRCS})
target_link_libraries(OMTP ../openssl/lib/libssl.a ../openssl/lib/libcrypto.a)

#install(DIRECTORY ${PROJECT_BINARY_DIR}/../../3rd/rapidjson/ DESTINATION include/rapidjson)
install(DIRECTORY include/ DESTINATION include)
install(FILES include/CLog.h DESTINATION api)
install(FILES include/CSafeQueue.h DESTINATION api)
install(FILES include/CThreadEx.h DESTINATION api)
install(TARGETS OMTP DESTINATION lib)

