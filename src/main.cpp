#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <cstdio>

#include "../include/Analyze.hpp"

using namespace std;

#define TEST

#ifdef TEST
int main(){
//    Analyze().AnalyzeResNet50("./ResNet50/n02092339_2913.JPEG/");
//    Analyze().AnalyzeAlexNet("./AlexNet/n03481172.jpg/");
//    Analyze().AnalyzeAlexNet("./AlexNet/n02783161_4946.JPEG/");
    Analyze().AnalyzeVGG16  ("./VGG16/n02841315_7661.JPEG/");
//    Analyze().AnalyzeResNet18("./ResNet18/n01751748_11712.JPEG/");
    return 0;
}
#else
int main(int argc, char * argv []){
    std::string str1;
    std::string str2;
    switch(argc){
    case 2:
        str1 = argv[1];
        if (str1.compare("AlexNet") == 0)
            Analyze().AnalyzeAlexNet("./");
        else if (str1.compare("VGG16") == 0)
            Analyze().AnalyzeVGG16("./");
        else if (str1.compare("ResNet18") == 0)
            Analyze().AnalyzeResNet18("./");
        else if (str1.compare("ResNet50") == 0)
            Analyze().AnalyzeResNet50("./");
        else
            std::cout<<"unsupported model"<<std::endl;
        break;
    case 3:
        str1 = argv[1];
        str2 = argv[2];
        if (str1.compare("AlexNet") == 0)
            Analyze().AnalyzeAlexNet(str2);
        else if (str1.compare("VGG16") == 0)
            Analyze().AnalyzeVGG16(str2);
        else if (str1.compare("ResNet18") == 0)
            Analyze().AnalyzeResNet18(str2);
        else if (str1.compare("ResNet50") == 0)
            Analyze().AnalyzeResNet50(str2);
        else
            std::cout<<"unsupported model"<<std::endl;
        break;
    default:
        std::cout<<"illegal parameter number"<<std::endl;
    }
    return 0;
}
#endif //TEST
