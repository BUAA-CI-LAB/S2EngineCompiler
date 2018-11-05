#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <cstdio>

#include "../include/Analyze.hpp"

using namespace std;

int main(){
    Analyze().AnalyzeAlexNet();
//    Analyze().AnalyzeVGG16();
//    Analyze().AnalyzeVGG19();
    return 0;
}
