#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <cstdio>

#include "../include/Analyze.hpp"

using namespace std;

int main(){
//    Analyze().AnalyzeAlexNet();
//    Analyze().AnalyzeVGG16();
//    Analyze().AnalyzeVGG19();

//    Analyze().AnalyzeRealAlexNet("./n01601694.jpg/");
//    Analyze().AnalyzeRealAlexNet("./n01688243.jpg/");
//    Analyze().AnalyzeRealAlexNet("./n01735189.jpg/");

    Analyze().AnalyzeRealVGG16("./vgg16/8bits/n01601694.jpg/");
    Analyze().AnalyzeRealVGG16("./vgg16/8bits/n01688243.jpg/");
    return 0;
}
