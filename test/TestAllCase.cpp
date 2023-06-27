#include <gtest/gtest.h>
#include "GlobalEnv.h"
#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc, char** argv) 
{ 
    std::string setting = "qa";
    if(argc>1){
        setting = argv[1];
    }
    GlobalEnv *env = GlobalEnv::instance();
    testing::AddGlobalTestEnvironment(env);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
