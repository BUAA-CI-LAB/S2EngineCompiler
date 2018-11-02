#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <cstdio>

#include "../include/Analyze.hpp"

using namespace std;

int main(){
//    std::ios::sync_with_stdio(false);
//    LayerDimension layer1(224,224,64);
//    LayerDimension layer2(224,224,64);
//
//    LayerDimension layer1(112,112,128);
//    LayerDimension layer2(112,112,128);

//    LayerDimension layer1(56,56,256);
//    LayerDimension layer2(56,56,256);

//    LayerDimension layer1(56,56,128);
//    LayerDimension layer2(56,56,128);

//    LayerDimension layer1(56,56,128);
//    LayerDimension layer2(28,28,128);

//    LayerDimension layer1(56,56,64);
//    LayerDimension layer2(56,56,64);

//    LayerDimension layer1(24,24,32);
////    LayerDimension layer2( 8, 8,16);
////    LayerDimension layer2(24,24,16);
//    LayerDimension layer2(8,8,32);

//    LayerDimension layer1(227,227,GROUP_SIZE);
//    LayerDimension layer2( 55, 55, 96);

//    cout<<"equivalent workload (Mop):"<<layer2.GetWorkLoad(layer1,5,5)<<endl;
//
//    Analyze().GenRUPEArrayTestData(layer1,layer2,3,3,1,1);

    Analyze().AnalyzeAlexNet();
    Analyze().AnalyzeVGG16();
//    Analyze().AnalyzeVGG19();

//    float   r1 = Systolic::BestMap(224,224,64,3),
//            r2 = Systolic::BestMap(112,112,128,3),
//            r3 = Systolic::BestMap(56,56,256,3),
//            r4 = Systolic::BestMap(28,28,512,3),
//            r5 = Systolic::BestMap(14,14,512,3);
//
//
//    cout<<"total utilization ratio:"<<
//    (   3.0*3*  3* 64*224*224 *r1
//    +  (3.0*3* 64* 64*224*224)*r1
//    +  (3.0*3* 64*128*112*112)*r2
//    +  (3.0*3*128*128*112*112)*r2
//    +  (3.0*3*128* 56* 56*256)*r3
//    +3*(3.0*3*256* 56* 56*256)*r3
//    +  (3.0*3*256* 28* 28*512)*r4
//    +3*(3.0*3*512* 28* 28*512)*r4
//    +4*(3.0*3*512* 14* 14*512)*r5)
//    /
//    (   3.0*3*  3* 64*224*224
//    +  (3.0*3* 64* 64*224*224)
//    +  (3.0*3* 64*128*112*112)
//    +  (3.0*3*128*128*112*112)
//    +  (3.0*3*128* 56* 56*256)
//    +3*(3.0*3*256* 56* 56*256)
//    +  (3.0*3*256* 28* 28*512)
//    +3*(3.0*3*512* 28* 28*512)
//    +4*(3.0*3*512* 14* 14*512))<<"%"<<endl;

//####
//####
//len:62 waste112
//len:30 waste168
//len:14 waste196
//len:6 waste210
//used percentage:94.8148%
//####
//####
//len:56 waste392
//used percentage:88.8889%
//####
//####
//len:28 waste98
//used percentage:88.8889%
//####
//####
//len:14 waste56
//used percentage:77.7778%
//####

    return 0;
}


////    GenKernel(128,3,3,128,85);
////    GenFeature(112,112,128);
////    Layer thisLayer(112,112,128,3,3,128,SAME_PAD);
////    Layer lastLayer(112,112,128,3,3,128,SAME_PAD);
////
//////    GenKernel(512,3,3,128,75);
//////    GenFeature(112,112,128);
//////    Layer thisLayer(112,112,512,3,3,128,SAME_PAD);
//////    Layer lastLayer(112,112,128,3,3,512,SAME_PAD);
////
//////    GenKernel(256,3,3,128,75);
//////    GenFeature(112,112,128);
//////    Layer thisLayer(112,112,256,3,3,128,SAME_PAD);
//////    Layer lastLayer(112,112,128,3,3,256,SAME_PAD);
////
//////    GenKernel(240,3,3,64,75);
//////    GenFeature(112,112,64);
//////    Layer thisLayer(112,112,240,3,3,64,SAME_PAD);
//////    Layer lastLayer(112,112,64,3,3,256,SAME_PAD);
////
//////    GenKernel(8,3,3,8,75);
//////    GenFeature(4,4,8);
//////    Layer thisLayer(4,4,8,3,3,8,SAME_PAD);
//////    Layer lastLayer(4,4,8,3,3,8,SAME_PAD);
////
//////    (int lH,int lW,int kN,int kH,int kW,int kD,enum PAD_TYPE padding)
