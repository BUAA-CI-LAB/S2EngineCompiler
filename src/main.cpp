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

//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/16bits/n01601694.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n01601694.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n01688243.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n01735189.jpg/");

//    Analyze().AnalyzeRealAlexNet("./16bits/n01601694.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n01601694.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n01688243.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n01735189.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n02093754.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n02099712.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n02701002.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n02841315.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03014705.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03042490.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03249569.jpg/");
//
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03255030.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03481172.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03676483.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03769881.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n03814906.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n04026417.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n04404412.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n04532670.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n07715103.jpg/");
//    Analyze().AnalyzeRealAlexNet("./alexnet/pruned/8bits/n07930864.jpg/");

//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n01601694.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n01688243.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n01735189.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n02093754.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n02099712.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n02701002.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n02841315.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03014705.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03042490.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03249569.jpg/");
//
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03255030.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03481172.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03676483.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03769881.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n03814906.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n04026417.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n04404412.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n04532670.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n07715103.jpg/");
//    Analyze().AnalyzeRealVGG16("./alexnet/pruned/16bits/n07930864.jpg/");

//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n01601694.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n01688243.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n01735189.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n02093754.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n02099712.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n02701002.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n02841315.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03014705.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03042490.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03249569.jpg/");
//
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03255030.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03481172.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03676483.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03769881.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n03814906.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n04026417.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n04404412.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n04532670.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n07715103.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/8bits/n07930864.jpg/");

//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n01601694.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n01688243.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n01735189.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n02093754.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n02099712.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n02701002.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n02841315.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03014705.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03042490.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03249569.jpg/");
//
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03255030.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03481172.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03676483.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03769881.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n03814906.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n04026417.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n04404412.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n04532670.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n07715103.jpg/");
//    Analyze().AnalyzeRealVGG16("./vgg16/pruned/16bits/n07930864.jpg/");
    return 0;
}
