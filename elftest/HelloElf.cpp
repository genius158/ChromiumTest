//
// Created by dimina on 2021/12/22.
//

#include "HelloElf.h"
#include "stdio.h"


void HelloElf::hello(){
    printf("sadfasdfa");
}

int main(int argc, char** argv){
    HelloElf* hello = new HelloElf();

    hello->hello();
}