project(3rd)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#if(CMAKE_COMPILER_IS_GNUCXX)
#    add_compile_options(-std=c++11)
#    message(STATUS "optional:-std=c++11")   
#endif(CMAKE_COMPILER_IS_GNUCXX)

#NCLUDE_DIRECTORIES(./include)
#INCLUDE_DIRECTORIES(/code/git/imsdk/TNIMSDK/TMTP/openssl/include)
#INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/../dest/include)
#INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/../dest/include/rapidjson)

#AUX_SOURCE_DIRECTORY(. DIR_SRCS)

#ADD_LIBRARY(http STATIC ${DIR_SRCS})
#target_link_libraries(http ../openssl/lib/libssl.a ../openssl/lib/libcrypto.a)

install(DIRECTORY rapidjson/ DESTINATION include/rapidjson)
install(DIRECTORY OpenSSL/include/openssl/ DESTINATION include/openssl)
