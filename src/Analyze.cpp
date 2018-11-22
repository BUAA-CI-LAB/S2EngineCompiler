#include "../include/Analyze.hpp"

;void Analyze::PrintProcess(const char str[]) const{
    #ifdef PRINT_PROCESS
    std::cout<<str<<std::endl;
    #endif // PRINT_PROCESS
    return;
}

void Analyze::PrintResult(std::string str) const{
    #ifdef PRINT_RESULT
    std::cout<<str<<std::endl;
    #endif // PRINT_RESULT
    return;
}

void Analyze::analyze(const LayerDimension& lastLayerInfo,
                      const LayerDimension& thisLayerInfo,
                      int kH,int kW,int sH,int sW,const string& prefix,
                      uint64_t& totalSAWInSize,
                      uint64_t& totalSAXInSize,
                      uint64_t& totalCEXInSize,
                      std::ofstream& layerInfo
                      #ifndef GENERATE_DATA
                      ,const string& weightFile
                      ,const string&   biasFile
                      ,const string&     ifFile
                      ,const string&     ofFile
                      #endif // GENERATE_DATA
                      ){

    uint64_t saWInSize,
             saXInSize,
             ceXInSize;
    uint32_t totalCompute;

    #ifdef ANALYZE
    static int lastLayerCycle = -1;
    #endif // ANALYZE

    std::ofstream ofs (prefix+"workload.txt",std::ofstream::out);
    ofs<<thisLayerInfo.GetWorkLoad(lastLayerInfo,kH,kW)<<std::endl;
    ofs.close();

    #ifdef GENERATE_DATA
    GenKernel (thisLayerInfo.GetActD(),
               kH,kW,
               lastLayerInfo.GetActD(),
               thisLayerInfo.GetKS(),
               KERNEL_16_BIT_RATE,prefix);
    GenFeature(lastLayerInfo.GetH(),
               lastLayerInfo.GetW(),
               lastLayerInfo.GetActD(),
               lastLayerInfo.GetFS(),
               FEATURE_16_BIT_PERCENT,prefix);
    #endif // GENERATE_DATA

    Layer thisLayer(thisLayerInfo.GetH(),
                    thisLayerInfo.GetW(),
                    thisLayerInfo.GetD(),
                    kH,kW,sH,sW,
                    lastLayerInfo.GetD(),
                    thisLayerInfo.GetPadding(),
                    thisLayerInfo.GetHPadOff(),
                    thisLayerInfo.GetWPadOff(),
                    thisLayerInfo.IsInput(),
                    thisLayerInfo.HasBias());
    Layer lastLayer(lastLayerInfo.GetH(),
                    lastLayerInfo.GetW(),
                    lastLayerInfo.GetD(),
                     3, 3, 1, 1,
                     GROUP_SIZE,
                    lastLayerInfo.GetPadding(),
                    lastLayerInfo.GetHPadOff(),
                    lastLayerInfo.GetWPadOff(),
                    lastLayerInfo.IsInput(),
                    lastLayerInfo.HasBias());///the remained three parameter does not matter

    PrintProcess("[this layer] loading kernel");
    #ifdef GENERATE_DATA
    thisLayer.LoadKernel(prefix);
    #else
    thisLayer.LoadKernel(prefix,lastLayerInfo.GetActD(),
                                lastLayerInfo.GetD());
    #endif // GENERATE_DATA

    #ifdef GENERATE_DATA
    PrintProcess("[this layer] loading kernel sparse pattern");
    thisLayer.LoadPattern(prefix);
    #else
    PrintProcess("[this layer] loading bias");
    thisLayer.LoadBias(prefix);
    #endif // GENERATE_DATA

    #ifndef NDEBUG
    PrintProcess("[this layer] checking if the pattern meets the kernels");
    assert(thisLayer.CheckPattern());
    #endif // NDEBUG
    #ifdef GENERATE_DATA
    PrintProcess("[last layer] loading feature");
    lastLayer.LoadFeature(prefix,FEATURE_FILE_PATH);
    #else
    PrintProcess("[last layer] loading input feature");
    lastLayer.LoadFeature(prefix,lastLayerInfo.GetActD(),
                                 lastLayerInfo.GetD(),IF_FILE_PATH);
    PrintProcess("[this layer] loading output feature");
    thisLayer.LoadFeature(prefix,thisLayerInfo.GetActD(),
                                 thisLayerInfo.GetD(),OF_FILE_PATH);
    #endif // GENERATE_DATA
    PrintProcess("[last layer] partition feature into groups");
    lastLayer.PartitionFeature();
    PrintProcess("[this layer] generating the reorder (group) sequence and nonzero location of the kernel");
    thisLayer.GenReorderSeq();
    PrintProcess("[this layer] reorder each kernel base on the generated sequence and location");
    thisLayer.Reorder();
    #ifdef TRANS_DATA
    #ifndef NDEBUG
    #ifdef GENERATE_DATA
    PrintProcess("[this layer] computing the feature of this layer");
    thisLayer.Compute(lastLayer,totalCompute);
    #else
    PrintProcess("[this layer] check if the output feature matches");
    assert(thisLayer.Compute(lastLayer,totalCompute));
    #endif // GENERATE_DATA
    #endif // NDEBUG
    #endif // TRANS_DATA

    #ifdef PRINT_INTERMEDIA_INFO
    thisLayer.PrintReorder();
    thisLayer.PrintReorderedLoc();
    thisLayer.PrintFeature();
    thisLayer.PartitionFeature();
    #endif // PRINT_INTERMEDIA_INFO

    RUArray rua;
    Systolic sys(rua,this->dfus);

    PrintProcess("[systolic array] mapping layer");
    sys.MapToSA  (thisLayer);
    PrintProcess("[systolic array] generating workload");
    sys.GenWorkLoad(thisLayer);
    PrintProcess("[systolic array] generating weight input");
    sys.GenWInput(thisLayer);
    PrintProcess("[systolic array] generating feature input");
    sys.GenXInput(thisLayer);
    PrintProcess("[systolic array] generating output");
    sys.GenOutput(thisLayer);
    #ifdef ANALYZE
    PrintProcess("[systolic array] analyzing...");
    sys.AnalyzeInputData(thisLayer);
    #endif // ANALYZE
    #ifdef TRANS_DATA
    PrintProcess("[systolic array] translating weight input(from kernel index to actual data)");
    sys.TransWIn (thisLayer);
    saWInSize = sys.GetTotalWDataSize();
    #ifdef REFORMED
//    PrintProcess("\t[compiler] cleaning WIn");
//    sys.ClearWIn();
    #endif // REFORMED
    #ifdef NDEBUG
//    PrintProcess("\t[compiler] cleaning WTrans");
//    sys.ClearWTrans();
    #endif // NDEBUG
    PrintProcess("[systolic array] translating feature input(from logical location to actual data)");
    sys.TransXIn (thisLayer,lastLayer);
    saXInSize = sys.GetTotalXDataSize();

    #ifdef PRINT_TO_FILE
    PrintProcess("[systolic array] printing weight input");
    sys.PrintTransedW(prefix);
    #endif // PRINT_TO_FILE

    #ifdef PRINT_TO_FILE
    PrintProcess("[systolic array] printing feature input");
    sys.PrintTransedX(prefix);
    PrintProcess("[systolic array] printing output");
    sys.PrintOutput(thisLayer,prefix);
    #endif // PRINT_TO_FILE

    #ifndef NDEBUG
    PrintProcess("[systolic array] checking if translated weight and feature match");
    assert(sys.CheckXW(thisLayer));
    #endif // NDEBUG
    #endif // TRANS_DATA

    #ifdef ANALYZE
    PrintProcess("[DFU] analyzing...");
    sys.CalcDFUDRAMAccess(thisLayer);
    #endif // ANALYZE

    #ifdef REFORMED
    #ifdef NDEBUG
//    PrintProcess("\t[compiler] cleaning XTrans");
//    sys.ClearXTrans();
    #endif // NDEBUG
    #endif // REFORMED

//    PrintProcess("\t[compiler] cleaning Output");
//    sys.ClearOutput();

    #ifndef REFORMED
    PrintProcess("[RU array] generating weight location input");
    sys.GenRULIn();
//    PrintProcess("\t[compiler] cleaning WIn");
//    sys.ClearWIn();
    #endif // REFORMED
    PrintProcess("[RU array] generating feature input");
    sys.GenRUXIn(lastLayer,thisLayer);
//    PrintProcess("\t[compiler] cleaning XIn");
//    sys.ClearXIn();
    #ifndef REFORMED
    #ifndef NDEBUG
    PrintProcess("[RU array] checking the homogeneity of generated feature and location");
    assert(rua.CheckXLInHomo(thisLayer));
    #endif // NDEBUG
    #ifdef TRANS_DATA
    PrintProcess("[RU array] translating weight location input(from the index of kernel group to actual data)");
    sys.TransRULIn(thisLayer);
    #ifdef PRINT_TO_FILE
    PrintProcess("[RU array] printing translated weight location");
    rua.PrintLData(prefix);
    #endif // PRINT_TO_FILE
    #endif // TRANS_DATA
//    PrintProcess("\t[compiler] cleaning LIn");
//    rua.ClearLIn();
    #endif // REFORMED
//    PrintProcess("\t[compiler] cleaning this layer");
//    thisLayer.Clean();
    #ifdef TRANS_DATA
    PrintProcess("[RU array] translating feature input(from the serial of group to actual data)");
    sys.TransRUXIn(lastLayer);
    ceXInSize = rua.GetTotalXInSize();
    #ifdef PRINT_TO_FILE
    PrintProcess("[RU array] printing translated feature");
    rua.PrintXData(prefix);
    #endif // PRINT_TO_FILE
//    PrintProcess("\t[compiler] cleaning XIn");
//    rua.ClearXIn();
//    PrintProcess("\t[compiler] cleaning last layer");
//    lastLayer.Clean();
    #ifndef NDEBUG
    #ifndef REFORMED
    PrintProcess("[RU array] checking the homogeneity of translated feature and location");
    assert(rua.CheckXLDataHomo());
    #endif // REFORMED
    PrintProcess("[RU array] checking if the translated feature and location match");
    assert(sys.CheckRUXL());
    #endif // NDEBUG
//    PrintProcess("\t[compiler] cleaning XTrans");
//    sys.ClearXTrans();
    #endif // TRANS_DATA

    PrintProcess("[Systolic array] analyzing...");
    sys.AnalyzeWIn(thisLayer);
    PrintProcess("[RU array] analyzing...");
    rua.AnalyzeXIn(lastLayer);

    #ifdef ANALYZE
    const int SACyclseUse =
        this->PrintSysAnalReport(thisLayer,sys,
                                 thisLayerInfo,
                                 lastLayerInfo,
                                 lastLayerCycle,
                                 kH,kW);
    this->PrintDFUAnalReport(this->dfus,SACyclseUse);
    this->PrintRUAAnalReport(rua ,SACyclseUse);

    lastLayerCycle  = SACyclseUse;
    this->totalCyc += SACyclseUse;
    #endif // ANALYZE

    totalSAWInSize += saWInSize;
    totalSAXInSize += saXInSize;
    totalCEXInSize += ceXInSize;
    layerInfo<<saWInSize
             <<" "<<saXInSize
             <<" "<<ceXInSize
             <<" "<<totalCompute
             <<" "<<rua.GetGlobSparseDataSize()
             <<" "<<(lastLayerInfo.GetActD()<GROUP_SIZE ? rua.GetGlobDenseDataSize() / GROUP_SIZE * lastLayerInfo.GetActD()
                                                        : rua.GetGlobDenseDataSize())
             <<" "<<rua.GetInRowSparseDataSize()
             <<" "<<(lastLayerInfo.GetActD()<GROUP_SIZE ? rua.GetInRowDenseDataSize() / GROUP_SIZE * lastLayerInfo.GetActD()
                                                        : rua.GetInRowDenseDataSize())
             <<" "<<sys.GetUpSparseCacheSize()
             <<" "<<sys.GetUpDenseCacheSize()<<std::endl;
    return;
}

void Analyze::AnalyzeAlexNet(std::string path){
    this->totalCyc = 0;

    #ifdef STRAIGHT_OUT
    std::ofstream ofs (path+"layerInfo_naive_"+std::to_string(SYS_ROW)
                                          +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #else
    std::ofstream ofs (path+"layerInfo_"+std::to_string(SYS_ROW)
                                    +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #endif // STRAIGHT_OUT

    uint64_t totalSAWInSize = 0,
             totalSAXInSize = 0,
             totalCEXInSize = 0;

    LayerDimension layer0i(224,224,  3),
                   layer0o( 55, 55, 64,false,ZERO_PAD,2,2),
                   layer1i( 27, 27, 64),
                   layer1o( 27, 27,192,false,ZERO_PAD,2,2),
                   layer2i( 13, 13,192),
                   layer2o( 13, 13,384,false,ZERO_PAD,1,1),
                   layer3i( 13, 13,384),
                   layer3o( 13, 13,256,false,ZERO_PAD,1,1),
                   layer4i( 13, 13,256),
                   layer4o( 13, 13,256,false,ZERO_PAD,1,1);

    double WorkLoad=0;

    #ifdef GENERATE_DATA
    this->MKDir(path);
    #endif // GENERATE_DATA
    this->MKLayerDir(path,"conv_0");
    this->MKLayerDir(path,"conv_1");
    this->MKLayerDir(path,"conv_2");
    this->MKLayerDir(path,"conv_3");
    this->MKLayerDir(path,"conv_4");

    std::cout<<"#############"<<std::endl
             <<"##  conv1  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer0i.toString()<<"->"<<layer0o.toString()<<std::endl;
    analyze(layer0i,layer0o,11,11,4,4,path+"./conv_0/",totalSAWInSize,
                                                       totalSAXInSize,
                                                       totalCEXInSize,ofs);
    WorkLoad += layer0o.GetWorkLoad(layer0i,11,11);

    std::cout<<"#############"<<std::endl
             <<"##  conv2  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer1i.toString()<<"->"<<layer1o.toString()<<"\n"<<std::endl;
    analyze(layer1i,layer1o, 5, 5,1,1,path+"./conv_1/",totalSAWInSize,
                                                       totalSAXInSize,
                                                       totalCEXInSize,ofs);
    WorkLoad += layer1o.GetWorkLoad(layer1i,5,5);

    std::cout<<"#############"<<std::endl
             <<"##  conv3  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer2i.toString()<<"->"<<layer2o.toString()<<"\n"<<std::endl;
    analyze(layer2i,layer2o, 3, 3,1,1,path+"./conv_2/",totalSAWInSize,
                                                       totalSAXInSize,
                                                       totalCEXInSize,ofs);
    WorkLoad += layer2o.GetWorkLoad(layer2i,3,3);

    std::cout<<"#############"<<std::endl
             <<"##  conv4  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer3i.toString()<<"->"<<layer3o.toString()<<"\n"<<std::endl;
    analyze(layer3i,layer3o, 3, 3,1,1,path+"./conv_3/",totalSAWInSize,
                                                       totalSAXInSize,
                                                       totalCEXInSize,ofs);
    WorkLoad += layer3o.GetWorkLoad(layer3i,3,3);

    std::cout<<"#############"<<std::endl
             <<"##  conv5  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer4i.toString()<<"->"<<layer4o.toString()<<"\n"<<std::endl;
    analyze(layer4i,layer4o, 3, 3,1,1,path+"./conv_4/",totalSAWInSize,
                                                       totalSAXInSize,
                                                       totalCEXInSize,ofs);
    WorkLoad += layer4o.GetWorkLoad(layer4i,3,3);

    std::cout<<"total workload: "<<WorkLoad<<std::endl;
    std::cout<<"totalSAWIn size: "<<totalSAWInSize<<std::endl;
    std::cout<<"totalSAXIn size: "<<totalSAXInSize<<std::endl;
    std::cout<<"totalCEXIn size: "<<totalCEXInSize<<std::endl;
    std::cout<<"feature access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/totalSAXInSize<<std::endl;
    std::cout<<" total  access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/(totalSAXInSize+totalSAWInSize)<<std::endl;
    return;
}

void Analyze::AnalyzeVGG16(std::string path){
    this->totalCyc = 0;

    #ifdef STRAIGHT_OUT
    std::ofstream ofs (path+"layerInfo_naive_"+std::to_string(SYS_ROW)
                                          +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #else
    std::ofstream ofs (path+"layerInfo_"+std::to_string(SYS_ROW)
                                    +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #endif // STRAIGHT_OUT

    uint64_t totalSAWInSize = 0,
             totalSAXInSize = 0,
             totalCEXInSize = 0;

    LayerDimension layer0i(224,224,  3),
                   layer0o(224,224, 64,false,ZERO_PAD,1,1),
                   layer1i(224,224, 64),
                   layer1o(224,224, 64,false,ZERO_PAD,1,1),
                   layer2i(112,112, 64),
                   layer2o(112,112,128,false,ZERO_PAD,1,1),
                   layer3i(112,112,128),
                   layer3o(112,112,128,false,ZERO_PAD,1,1),
                   layer4i( 56, 56,128),
                   layer4o( 56, 56,256,false,ZERO_PAD,1,1),
                   layer5i( 56, 56,256),
                   layer5o( 56, 56,256,false,ZERO_PAD,1,1),
                   layer6i( 56, 56,256),
                   layer6o( 56, 56,256,false,ZERO_PAD,1,1),
                   layer7i( 28, 28,256),
                   layer7o( 28, 28,512,false,ZERO_PAD,1,1),
                   layer8i( 28, 28,512),
                   layer8o( 28, 28,512,false,ZERO_PAD,1,1),
                   layer9i( 28, 28,512),
                   layer9o( 28, 28,512,false,ZERO_PAD,1,1),
                   layer10i(14, 14,512),
                   layer10o(14, 14,512,false,ZERO_PAD,1,1),
                   layer11i(14, 14,512),
                   layer11o(14, 14,512,false,ZERO_PAD,1,1),
                   layer12i(14, 14,512),
                   layer12o(14, 14,512,false,ZERO_PAD,1,1);

    double WorkLoad=0;

    #ifdef GENERATE_DATA
    this->MKDir(path);
    #endif // GENERATE_DATA
    this->MKLayerDir(path,"conv_0");
    this->MKLayerDir(path,"conv_1");
    this->MKLayerDir(path,"conv_2");
    this->MKLayerDir(path,"conv_3");
    this->MKLayerDir(path,"conv_4");
    this->MKLayerDir(path,"conv_5");
    this->MKLayerDir(path,"conv_6");
    this->MKLayerDir(path,"conv_7");
    this->MKLayerDir(path,"conv_8");
    this->MKLayerDir(path,"conv_9");
    this->MKLayerDir(path,"conv_10");
    this->MKLayerDir(path,"conv_11");
    this->MKLayerDir(path,"conv_12");

    std::cout<<"###############"<<std::endl
             <<"##  conv1_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer0i.toString()<<"->"<<layer0o.toString()<<std::endl;
    analyze(layer0i,layer0o,3,3,1,1,path+"./conv_0/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer0o.GetWorkLoad(layer0i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv1_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer1i.toString()<<"->"<<layer1o.toString()<<"\n"<<endl;
    analyze(layer1i,layer1o,3,3,1,1,path+"./conv_1/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer1o.GetWorkLoad(layer1i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv2_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer2i.toString()<<"->"<<layer2o.toString()<<"\n"<<endl;
    analyze(layer2i,layer2o,3,3,1,1,path+"./conv_2/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer2o.GetWorkLoad(layer2i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv2_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer3i.toString()<<"->"<<layer3o.toString()<<"\n"<<endl;
    analyze(layer3i,layer3o,3,3,1,1,path+"./conv_3/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer3o.GetWorkLoad(layer3i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer4i.toString()<<"->"<<layer4o.toString()<<"\n"<<endl;
    analyze(layer4i,layer4o,3,3,1,1,path+"./conv_4/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer4o.GetWorkLoad(layer4i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer5i.toString()<<"->"<<layer5o.toString()<<"\n"<<endl;
    analyze(layer5i,layer5o,3,3,1,1,path+"./conv_5/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer5o.GetWorkLoad(layer5i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer6i.toString()<<"->"<<layer6o.toString()<<"\n"<<endl;
    analyze(layer6i,layer6o,3,3,1,1,path+"./conv_6/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer6o.GetWorkLoad(layer6i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer7i.toString()<<"->"<<layer7o.toString()<<"\n"<<endl;
    analyze(layer7i,layer7o,3,3,1,1,path+"./conv_7/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer7o.GetWorkLoad(layer7i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8i.toString()<<"->"<<layer8o.toString()<<"\n"<<endl;
    analyze(layer8i,layer8o,3,3,1,1,path+"./conv_8/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer8o.GetWorkLoad(layer8i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer9i.toString()<<"->"<<layer9o.toString()<<"\n"<<endl;
    analyze(layer9i,layer9o,3,3,1,1,path+"./conv_9/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer9o.GetWorkLoad(layer9i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer10i.toString()<<"->"<<layer10o.toString()<<"\n"<<endl;
    analyze(layer10i,layer10o,3,3,1,1,path+"./conv_10/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer10o.GetWorkLoad(layer10i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer11i.toString()<<"->"<<layer11o.toString()<<"\n"<<endl;
    analyze(layer11i,layer11o,3,3,1,1,path+"./conv_11/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer11o.GetWorkLoad(layer11i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer12i.toString()<<"->"<<layer12o.toString()<<"\n"<<endl;
    analyze(layer12i,layer12o,3,3,1,1,path+"./conv_12/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer12o.GetWorkLoad(layer12i,3,3);

    std::cout<<"total workload: "<<WorkLoad<<std::endl;
    std::cout<<"totalSAWIn size: "<<totalSAWInSize<<std::endl;
    std::cout<<"totalSAXIn size: "<<totalSAXInSize<<std::endl;
    std::cout<<"totalCEXIn size: "<<totalCEXInSize<<std::endl;
    std::cout<<"feature access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/totalSAXInSize<<std::endl;
    std::cout<<" total  access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/(totalSAXInSize+totalSAWInSize)<<std::endl;
    return;
}

void Analyze::AnalyzeResNet18(std::string path){
    this->totalCyc = 0;

    #ifdef STRAIGHT_OUT
    std::ofstream ofs (path+"layerInfo_naive_"+std::to_string(SYS_ROW)
                                          +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #else
    std::ofstream ofs (path+"layerInfo_"+std::to_string(SYS_ROW)
                                    +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #endif // STRAIGHT_OUT

    uint64_t totalSAWInSize = 0,
             totalSAXInSize = 0,
             totalCEXInSize = 0;

    LayerDimension
        layer0i (224,224,  3),
        layer0o (112,112, 64,false,ZERO_PAD,3,3,false),
        layer1i ( 56, 56, 64),
        layer1o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        layer2i ( 56, 56, 64),
        layer2o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        layer3i ( 56, 56, 64),
        layer3o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        layer4i ( 56, 56, 64),
        layer4o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        layer5i ( 56, 56, 64),
        layer5o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        layer6i ( 28, 28,128),
        layer6o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        layer7i ( 56, 56, 64),
        layer7o ( 28, 28,128,false,ZERO_PAD,0,0,false),
        layer8i ( 28, 28,128),
        layer8o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        layer9i ( 28, 28,128),
        layer9o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        layer10i( 28, 28,128),
        layer10o( 14, 14,256,false,ZERO_PAD,1,1,false),
        layer11i( 14, 14,256),
        layer11o( 14, 14,256,false,ZERO_PAD,1,1,false),
        layer12i( 28, 28,128),
        layer12o( 14, 14,256,false,ZERO_PAD,0,0,false),
        layer13i( 14, 14,256),
        layer13o( 14, 14,256,false,ZERO_PAD,1,1,false),
        layer14i( 14, 14,256),
        layer14o( 14, 14,256,false,ZERO_PAD,1,1,false),
        layer15i( 14, 14,256),
        layer15o(  7,  7,512,false,ZERO_PAD,1,1,false),
        layer16i(  7,  7,512),
        layer16o(  7,  7,512,false,ZERO_PAD,1,1,false),
        layer17i( 14, 14,256),
        layer17o(  7,  7,512,false,ZERO_PAD,0,0,false),
        layer18i(  7,  7,512),
        layer18o(  7,  7,512,false,ZERO_PAD,1,1,false),
        layer19i(  7,  7,512),
        layer19o(  7,  7,512,false,ZERO_PAD,1,1,false);

    double WorkLoad=0;

    #ifdef GENERATE_DATA
    this->MKDir(path);
    #endif // GENERATE_DATA
    this->MKLayerDir(path,"conv_0");
    this->MKLayerDir(path,"conv_1");
    this->MKLayerDir(path,"conv_2");
    this->MKLayerDir(path,"conv_3");
    this->MKLayerDir(path,"conv_4");
    this->MKLayerDir(path,"conv_5");
    this->MKLayerDir(path,"conv_6");
    this->MKLayerDir(path,"conv_7");
    this->MKLayerDir(path,"conv_8");
    this->MKLayerDir(path,"conv_9");
    this->MKLayerDir(path,"conv_10");
    this->MKLayerDir(path,"conv_11");
    this->MKLayerDir(path,"conv_12");
    this->MKLayerDir(path,"conv_13");
    this->MKLayerDir(path,"conv_14");
    this->MKLayerDir(path,"conv_15");
    this->MKLayerDir(path,"conv_16");
    this->MKLayerDir(path,"conv_17");
    this->MKLayerDir(path,"conv_18");
    this->MKLayerDir(path,"conv_19");

    std::cout<<"##############"<<std::endl
             <<"##  conv_0  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer0i.toString()<<"->"<<layer0o.toString()<<std::endl;
    analyze(layer0i,layer0o,7,7,2,2,path+"./conv_0/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer0o.GetWorkLoad(layer0i,7,7);

    std::cout<<"##############"<<std::endl
             <<"##  conv_1  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer1i.toString()<<"->"<<layer1o.toString()<<"\n"<<endl;
    analyze(layer1i,layer1o,3,3,1,1,path+"./conv_1/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer1o.GetWorkLoad(layer1i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_2  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer2i.toString()<<"->"<<layer2o.toString()<<"\n"<<endl;
    analyze(layer2i,layer2o,3,3,1,1,path+"./conv_2/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer2o.GetWorkLoad(layer2i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_3  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer3i.toString()<<"->"<<layer3o.toString()<<"\n"<<endl;
    analyze(layer3i,layer3o,3,3,1,1,path+"./conv_3/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer3o.GetWorkLoad(layer3i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_4  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer4i.toString()<<"->"<<layer4o.toString()<<"\n"<<endl;
    analyze(layer4i,layer4o,3,3,1,1,path+"./conv_4/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer4o.GetWorkLoad(layer4i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_5  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer5i.toString()<<"->"<<layer5o.toString()<<"\n"<<endl;
    analyze(layer5i,layer5o,3,3,2,2,path+"./conv_5/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer5o.GetWorkLoad(layer5i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_6  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer6i.toString()<<"->"<<layer6o.toString()<<"\n"<<endl;
    analyze(layer6i,layer6o,3,3,1,1,path+"./conv_6/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer6o.GetWorkLoad(layer6i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_7  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer7i.toString()<<"->"<<layer7o.toString()<<"\n"<<endl;
    analyze(layer7i,layer7o,1,1,2,2,path+"./conv_7/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer7o.GetWorkLoad(layer7i,1,1);

    std::cout<<"##############"<<std::endl
             <<"##  conv_8  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer8i.toString()<<"->"<<layer8o.toString()<<"\n"<<endl;
    analyze(layer8i,layer8o,3,3,1,1,path+"./conv_8/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer8o.GetWorkLoad(layer8i,3,3);

    std::cout<<"##############"<<std::endl
             <<"##  conv_9  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer9i.toString()<<"->"<<layer9o.toString()<<"\n"<<endl;
    analyze(layer9i,layer9o,3,3,1,1,path+"./conv_9/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer9o.GetWorkLoad(layer9i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_10  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer10i.toString()<<"->"<<layer10o.toString()<<"\n"<<endl;
    analyze(layer10i,layer10o,3,3,2,2,path+"./conv_10/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer10o.GetWorkLoad(layer10i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_11  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer11i.toString()<<"->"<<layer11o.toString()<<"\n"<<endl;
    analyze(layer11i,layer11o,3,3,1,1,path+"./conv_11/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer11o.GetWorkLoad(layer11i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_12  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer12i.toString()<<"->"<<layer12o.toString()<<"\n"<<endl;
    analyze(layer12i,layer12o,1,1,2,2,path+"./conv_12/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer12o.GetWorkLoad(layer12i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_13  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer13i.toString()<<"->"<<layer13o.toString()<<"\n"<<endl;
    analyze(layer13i,layer13o,3,3,1,1,path+"./conv_13/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer13o.GetWorkLoad(layer13i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_14  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer14i.toString()<<"->"<<layer14o.toString()<<"\n"<<endl;
    analyze(layer14i,layer14o,3,3,1,1,path+"./conv_14/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer14o.GetWorkLoad(layer14i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_15  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer15i.toString()<<"->"<<layer15o.toString()<<"\n"<<endl;
    analyze(layer15i,layer15o,3,3,2,2,path+"./conv_15/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer15o.GetWorkLoad(layer15i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_16  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer16i.toString()<<"->"<<layer16o.toString()<<"\n"<<endl;
    analyze(layer16i,layer16o,3,3,1,1,path+"./conv_16/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer16o.GetWorkLoad(layer16i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_17  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer17i.toString()<<"->"<<layer17o.toString()<<"\n"<<endl;
    analyze(layer17i,layer17o,1,1,2,2,path+"./conv_17/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer17o.GetWorkLoad(layer17i,1,1);

    std::cout<<"###############"<<std::endl
             <<"##  conv_18  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer18i.toString()<<"->"<<layer18o.toString()<<"\n"<<endl;
    analyze(layer18i,layer18o,3,3,1,1,path+"./conv_18/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer18o.GetWorkLoad(layer18i,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv_19  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer18i.toString()<<"->"<<layer18o.toString()<<"\n"<<endl;
    analyze(layer18i,layer18o,3,3,1,1,path+"./conv_19/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer18o.GetWorkLoad(layer18i,3,3);

    std::cout<<"total workload: "<<WorkLoad<<std::endl;
    std::cout<<"totalSAWIn size: "<<totalSAWInSize<<std::endl;
    std::cout<<"totalSAXIn size: "<<totalSAXInSize<<std::endl;
    std::cout<<"totalCEXIn size: "<<totalCEXInSize<<std::endl;
    std::cout<<"feature access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/totalSAXInSize<<std::endl;
    std::cout<<" total  access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/(totalSAXInSize+totalSAWInSize)<<std::endl;
    return;
}

void Analyze::AnalyzeResNet50(std::string path){
    this->totalCyc = 0;

    #ifdef STRAIGHT_OUT
    std::ofstream ofs (path+"layerInfo_naive_"+std::to_string(SYS_ROW)
                                          +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #else
    std::ofstream ofs (path+"layerInfo_"+std::to_string(SYS_ROW)
                                    +"_"+std::to_string(SYS_COLUM)+".txt",std::ofstream::out);
    #endif // STRAIGHT_OUT

    uint64_t totalSAWInSize = 0,
             totalSAXInSize = 0,
             totalCEXInSize = 0;

    LayerDimension
        ///Conv2d(3, 64, kernel_size=(7, 7), stride=(2, 2), padding=(3, 3), bias=False)
        layer0i (224,224,  3),
        layer0o (112,112, 64,false,ZERO_PAD,3,3,false),
        ///Conv2d(64, 64, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer1i ( 56, 56, 64),
        layer1o ( 56, 56, 64,false,ZERO_PAD,0,0,false),
        ///Conv2d(64, 64, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer2i ( 56, 56, 64),
        layer2o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer3i ( 56, 56, 64),
        layer3o ( 56, 56,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer4i ( 56, 56, 64),
        layer4o ( 56, 56,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 64, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer5i ( 56, 56,256),
        layer5o ( 56, 56, 64,false,ZERO_PAD,0,0,false),
        ///Conv2d(64, 64, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer6i ( 56, 56, 64),
        layer6o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer7i ( 56, 56, 64),
        layer7o ( 56, 56,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 64, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer8i ( 56, 56,256),
        layer8o ( 56, 56, 64,false,ZERO_PAD,0,0,false),
        ///Conv2d(64, 64, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer9i ( 56, 56, 64),
        layer9o ( 56, 56, 64,false,ZERO_PAD,1,1,false),
        ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer10i ( 56, 56, 64),
        layer10o ( 56, 56,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer11i ( 56, 56,256),
        layer11o ( 56, 56,128,false,ZERO_PAD,0,0,false),
        ///Conv2d(128, 128, kernel_size=(3, 3), stride=(2, 2), padding=(1, 1), bias=False)
        layer12i ( 56, 56,128),
        layer12o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer13i ( 28, 28,128),
        layer13o ( 28, 28,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 512, kernel_size=(1, 1), stride=(2, 2), bias=False)
        layer14i ( 56, 56,256),
        layer14o ( 28, 28,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer15i ( 28, 28,512),
        layer15o ( 28, 28,128,false,ZERO_PAD,0,0,false),
        ///Conv2d(128, 128, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer16i ( 28, 28,128),
        layer16o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer17i ( 28, 28,128),
        layer17o ( 28, 28,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer18i ( 28, 28,512),
        layer18o ( 28, 28,128,false,ZERO_PAD,0,0,false),
        ///Conv2d(128, 128, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer19i ( 28, 28,128),
        layer19o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer20i ( 28, 28,128),
        layer20o ( 28, 28,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer21i ( 28, 28,512),
        layer21o ( 28, 28,128,false,ZERO_PAD,0,0,false),
        ///Conv2d(128, 128, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer22i ( 28, 28,128),
        layer22o ( 28, 28,128,false,ZERO_PAD,1,1,false),
        ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer23i ( 28, 28,128),
        layer23o ( 28, 28,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer24i ( 28, 28,512),
        layer24o ( 28, 28,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 256, kernel_size=(3, 3), stride=(2, 2), padding=(1, 1), bias=False)
        layer25i ( 28, 28,256),
        layer25o ( 14, 14,256,false,ZERO_PAD,1,1,false),
        ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer26i ( 14, 14,256),
        layer26o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 1024, kernel_size=(1, 1), stride=(2, 2), bias=False)
        layer27i ( 28, 28,512),
        layer27o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer28i ( 14, 14,1024),
        layer28o ( 14, 14,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer29i ( 14, 14,256),
        layer29o ( 14, 14,256,false,ZERO_PAD,1,1,false),
        ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer30i ( 14, 14,256),
        layer30o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer31i ( 14, 14,1024),
        layer31o ( 14, 14,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer32i ( 14, 14,256),
        layer32o ( 14, 14,256,false,ZERO_PAD,1,1,false),
        ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer33i ( 14, 14,256),
        layer33o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer34i ( 14, 14,1024),
        layer34o ( 14, 14,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer35i ( 14, 14,256),
        layer35o ( 14, 14,256,false,ZERO_PAD,1,1,false),
        ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer36i ( 14, 14,256),
        layer36o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer37i ( 14, 14,1024),
        layer37o ( 14, 14,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer38i ( 14, 14,256),
        layer38o ( 14, 14,256,false,ZERO_PAD,1,1,false),
        ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer39i ( 14, 14,256),
        layer39o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer40i ( 14, 14,1024),
        layer40o ( 14, 14,256,false,ZERO_PAD,0,0,false),
        ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer41i ( 14, 14,256),
        layer41o ( 14, 14,256,false,ZERO_PAD,1,1,false),
        ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer42i ( 14, 14,256),
        layer42o ( 14, 14,1024,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer43i ( 14, 14,1024),
        layer43o ( 14, 14,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 512, kernel_size=(3, 3), stride=(2, 2), padding=(1, 1), bias=False)
        layer44i ( 14, 14,512),
        layer44o (  7,  7,512,false,ZERO_PAD,1,1,false),
        ///Conv2d(512, 2048, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer45i (  7,  7,512),
        layer45o (  7,  7,2048,false,ZERO_PAD,0,0,false),
        ///Conv2d(1024, 2048, kernel_size=(1, 1), stride=(2, 2), bias=False)
        layer46i ( 14, 14,1024),
        layer46o (  7,  7,2048,false,ZERO_PAD,0,0,false),
        ///Conv2d(2048, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer47i (  7,  7,2048),
        layer47o (  7,  7,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 512, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer48i (  7,  7,512),
        layer48o (  7,  7,512,false,ZERO_PAD,1,1,false),
        ///Conv2d(512, 2048, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer49i (  7,  7,512),
        layer49o (  7,  7,2048,false,ZERO_PAD,0,0,false),
        ///Conv2d(2048, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer50i (  7,  7,2048),
        layer50o (  7,  7,512,false,ZERO_PAD,0,0,false),
        ///Conv2d(512, 512, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
        layer51i (  7,  7,512),
        layer51o (  7,  7,512,false,ZERO_PAD,1,1,false),
        ///Conv2d(512, 2048, kernel_size=(1, 1), stride=(1, 1), bias=False)
        layer52i (  7,  7,512),
        layer52o (  7,  7,2048,false,ZERO_PAD,0,0,false);

    double WorkLoad=0;

    #ifdef GENERATE_DATA
    this->MKDir(path);
    #endif // GENERATE_DATA
    this->MKLayerDir(path,"conv_0");
    this->MKLayerDir(path,"conv_1");
    this->MKLayerDir(path,"conv_2");
    this->MKLayerDir(path,"conv_3");
    this->MKLayerDir(path,"conv_4");
    this->MKLayerDir(path,"conv_5");
    this->MKLayerDir(path,"conv_6");
    this->MKLayerDir(path,"conv_7");
    this->MKLayerDir(path,"conv_8");
    this->MKLayerDir(path,"conv_9");
    this->MKLayerDir(path,"conv_10");
    this->MKLayerDir(path,"conv_11");
    this->MKLayerDir(path,"conv_12");
    this->MKLayerDir(path,"conv_13");
    this->MKLayerDir(path,"conv_14");
    this->MKLayerDir(path,"conv_15");
    this->MKLayerDir(path,"conv_16");
    this->MKLayerDir(path,"conv_17");
    this->MKLayerDir(path,"conv_18");
    this->MKLayerDir(path,"conv_19");
    this->MKLayerDir(path,"conv_20");
    this->MKLayerDir(path,"conv_21");
    this->MKLayerDir(path,"conv_22");
    this->MKLayerDir(path,"conv_23");
    this->MKLayerDir(path,"conv_24");
    this->MKLayerDir(path,"conv_25");
    this->MKLayerDir(path,"conv_26");
    this->MKLayerDir(path,"conv_27");
    this->MKLayerDir(path,"conv_28");
    this->MKLayerDir(path,"conv_29");
    this->MKLayerDir(path,"conv_30");
    this->MKLayerDir(path,"conv_31");
    this->MKLayerDir(path,"conv_32");
    this->MKLayerDir(path,"conv_33");
    this->MKLayerDir(path,"conv_34");
    this->MKLayerDir(path,"conv_35");
    this->MKLayerDir(path,"conv_36");
    this->MKLayerDir(path,"conv_37");
    this->MKLayerDir(path,"conv_38");
    this->MKLayerDir(path,"conv_39");
    this->MKLayerDir(path,"conv_40");
    this->MKLayerDir(path,"conv_41");
    this->MKLayerDir(path,"conv_42");
    this->MKLayerDir(path,"conv_43");
    this->MKLayerDir(path,"conv_44");
    this->MKLayerDir(path,"conv_45");
    this->MKLayerDir(path,"conv_46");
    this->MKLayerDir(path,"conv_47");
    this->MKLayerDir(path,"conv_48");
    this->MKLayerDir(path,"conv_49");
    this->MKLayerDir(path,"conv_50");
    this->MKLayerDir(path,"conv_51");
    this->MKLayerDir(path,"conv_52");

/*
    ///Conv2d(3, 64, kernel_size=(7, 7), stride=(2, 2), padding=(3, 3), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_0  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer0i.toString()<<"->"<<layer0o.toString()<<std::endl;
    analyze(layer0i,layer0o,7,7,2,2,path+"./conv_0/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer0o.GetWorkLoad(layer0i,7,7);

    ///Conv2d(64, 64, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_1  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer1i.toString()<<"->"<<layer1o.toString()<<"\n"<<endl;
    analyze(layer1i,layer1o,1,1,1,1,path+"./conv_1/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer1o.GetWorkLoad(layer1i,1,1);

    ///Conv2d(64, 64, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_2  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer2i.toString()<<"->"<<layer2o.toString()<<"\n"<<endl;
    analyze(layer2i,layer2o,3,3,1,1,path+"./conv_2/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer2o.GetWorkLoad(layer2i,3,3);

    ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_3  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer3i.toString()<<"->"<<layer3o.toString()<<"\n"<<endl;
    analyze(layer3i,layer3o,1,1,1,1,path+"./conv_3/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer3o.GetWorkLoad(layer3i,1,1);

    ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_4  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer4i.toString()<<"->"<<layer4o.toString()<<"\n"<<endl;
    analyze(layer4i,layer4o,1,1,1,1,path+"./conv_4/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer4o.GetWorkLoad(layer4i,1,1);

    ///Conv2d(256, 64, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_5  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer5i.toString()<<"->"<<layer5o.toString()<<"\n"<<endl;
    analyze(layer5i,layer5o,1,1,1,1,path+"./conv_5/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer5o.GetWorkLoad(layer5i,1,1);

    ///Conv2d(64, 64, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_6  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer6i.toString()<<"->"<<layer6o.toString()<<"\n"<<endl;
    analyze(layer6i,layer6o,3,3,1,1,path+"./conv_6/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer6o.GetWorkLoad(layer6i,3,3);

    ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<layer45<std::endl
             <<"##  conv_7  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer7i.toString()<<"->"<<layer7o.toString()<<"\n"<<endl;
    analyze(layer7i,layer7o,1,1,1,1,path+"./conv_7/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer7o.GetWorkLoad(layer7i,1,1);

    ///Conv2d(256, 64, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_8  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer8i.toString()<<"->"<<layer8o.toString()<<"\n"<<endl;
    analyze(layer8i,layer8o,1,1,1,1,path+"./conv_8/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer8o.GetWorkLoad(layer8i,1,1);

    ///Conv2d(64, 64, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_9  ##"<<std::endl
             <<"##############"<<std::endl
             <<layer9i.toString()<<"->"<<layer9o.toString()<<"\n"<<endl;
    analyze(layer9i,layer9o,3,3,1,1,path+"./conv_9/",totalSAWInSize,
                                                     totalSAXInSize,
                                                     totalCEXInSize,ofs);
    WorkLoad += layer9o.GetWorkLoad(layer9i,3,3);

    ///Conv2d(64, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_10 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer10i.toStringlayer45()<<"->"<<layer10o.toString()<<"\n"<<endl;
    analyze(layer10i,layer10o,1,1,1,1,path+"./conv_10/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer10o.GetWorkLoad(layer10i,1,1);

    ///Conv2d(256, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_11 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer11i.toString()<<"->"<<layer11o.toString()<<"\n"<<endl;
    analyze(layer11i,layer11o,1,1,1,1,path+"./conv_11/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer11o.GetWorkLoad(layer11i,1,1);

    ///Conv2d(128, 128, kernel_size=(3, 3), stride=(2, 2), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_12 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer12i.toString()<<"->"<<layer12o.toString()<<"\n"<<endl;
    analyze(layer12i,layer12o,3,3,2,2,path+"./conv_12/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer12o.GetWorkLoad(layer12i,3,3);

    ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_13 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer13i.toString()<<"->"<<layer13o.toString()<<"\n"<<endl;
    analyze(layer13i,layer13o,1,1,1,1,path+"./conv_13/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer13o.GetWorkLoad(layer13i,1,1);

    ///Conv2d(256, 512, kernel_size=(1, 1), stride=(2, 2), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_14 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer14i.toString()<<"->"<<layer14o.toString()<<"\n"<<endl;
    analyze(layer14i,layer14o,1,1,2,2,path+"./conv_14/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer14o.GetWorkLoad(layer14i,1,1);

    ///Conv2d(512, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_15 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer15i.toString()<<"->"<<layer15o.toString()<<"\n"<<endl;
    analyze(layer15i,layer15o,1,1,1,1,path+"./conv_15/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer15o.GetWorkLoad(layer15i,1,1);

    ///Conv2d(128, 128, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_16 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer16i.toString()<<"->"<<layer16o.toString()<<"\n"<<endl;
    analyze(layer16i,layer16o,3,3,1,1,path+"./conv_16/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer16o.GetWorkLoad(layer16i,3,3);

    ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_17 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer17i.toString()<<"->"<<layer17o.toString()<<"\n"<<endl;
    analyze(layer17i,layer17o,1,1,1,1,path+"./conv_17/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer17o.GetWorkLoad(layer17i,1,1);

    ///Conv2d(512, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_18 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer18i.toString()<<"->"<<layer18o.toString()<<"\n"<<endl;
    analyze(layer18i,layer18o,1,1,1,1,path+"./conv_18/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer18o.GetWorkLoad(layer18i,1,1);

    ///Conv2d(128, 128, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_19 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer19i.toString()<<"->"<<layer19o.toString()<<"\n"<<endl;
    analyze(layer19i,layer19o,3,3,1,1,path+"./conv_19/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer19o.GetWorkLoad(layer19i,3,3);

    ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_20 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer20i.toString()<<"->"<<layer20o.toString()<<"\n"<<endl;
    analyze(layer20i,layer20o,1,1,1,1,path+"./conv_20/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer20o.GetWorkLoad(layer20i,1,1);

    ///Conv2d(512, 128, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_21 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer21i.toString()<<"->"<<layer21o.toString()<<"\n"<<endl;
    analyze(layer21i,layer21o,1,1,1,1,path+"./conv_21/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer21o.GetWorkLoad(layer21i,1,1);

    ///Conv2d(128, 128, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_22 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer22i.toString()<<"->"<<layer22o.toString()<<"\n"<<endl;
    analyze(layer22i,layer22o,3,3,1,1,path+"./conv_22/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer22o.GetWorkLoad(layer22i,3,3);

    ///Conv2d(128, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_23 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer23i.toString()<<"->"<<layer23o.toString()<<"\n"<<endl;
    analyze(layer23i,layer23o,1,1,1,1,path+"./conv_23/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer23o.GetWorkLoad(layer23i,1,1);

    ///Conv2d(512, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_24 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer24i.toString()<<"->"<<layer24o.toString()<<"\n"<<endl;
    analyze(layer24i,layer24o,1,1,1,1,path+"./conv_24/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer24o.GetWorkLoad(layer24i,1,1);

    ///Conv2d(256, 256, kernel_size=(3, 3), stride=(2, 2), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_25 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer25i.toString()<<"->"<<layer25o.toString()<<"\n"<<endl;
    analyze(layer25i,layer25o,3,3,2,2,path+"./conv_25/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer25o.GetWorkLoad(layer25i,3,3);

    ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_26 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer26i.toString()<<"->"<<layer26o.toString()<<"\n"<<endl;
    analyze(layer26i,layer26o,1,1,1,1,path+"./conv_26/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer26o.GetWorkLoad(layer26i,1,1);

    ///Conv2d(512, 1024, kernel_size=(1, 1), stride=(2, 2), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_27 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer27i.toString()<<"->"<<layer27o.toString()<<"\n"<<endl;
    analyze(layer27i,layer27o,1,1,2,2,path+"./conv_27/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer27o.GetWorkLoad(layer27i,1,1);

    ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_28 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer28i.toString()<<"->"<<layer28o.toString()<<"\n"<<endl;
    analyze(layer28i,layer28o,1,1,1,1,path+"./conv_28/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer28o.GetWorkLoad(layer28i,1,1);

    ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_29 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer29i.toString()<<"->"<<layer29o.toString()<<"\n"<<endl;
    analyze(layer29i,layer29o,3,3,1,1,path+"./conv_29/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer29o.GetWorkLoad(layer29i,3,3);

    ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_30 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer30i.toString()<<"->"<<layer30o.toString()<<"\n"<<endl;
    analyze(layer30i,layer30o,1,1,1,1,path+"./conv_30/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer30o.GetWorkLoad(layer30i,1,1);

    ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_31 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer31i.toString()<<"->"<<layer31o.toString()<<"\n"<<endl;
    analyze(layer31i,layer31o,1,1,1,1,path+"./conv_31/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer31o.GetWorkLoad(layer31i,1,1);

    ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_32 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer32i.toString()<<"->"<<layer32o.toString()<<"\n"<<endl;
    analyze(layer32i,layer32o,3,3,1,1,path+"./conv_32/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer32o.GetWorkLoad(layer32i,3,3);
*/
    ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_33 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer33i.toString()<<"->"<<layer33o.toString()<<"\n"<<endl;
    analyze(layer33i,layer33o,1,1,1,1,path+"./conv_33/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer33o.GetWorkLoad(layer33i,1,1);
/*
    ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_34 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer34i.toString()<<"->"<<layer34o.toString()<<"\n"<<endl;
    analyze(layer34i,layer34o,1,1,1,1,path+"./conv_34/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer34o.GetWorkLoad(layer34i,1,1);

    ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_35 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer35i.toString()<<"->"<<layer35o.toString()<<"\n"<<endl;
    analyze(layer35i,layer35o,3,3,1,1,path+"./conv_35/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer35o.GetWorkLoad(layer35i,3,3);

    ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_36 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer36i.toString()<<"->"<<layer36o.toString()<<"\n"<<endl;
    analyze(layer36i,layer36o,1,1,1,1,path+"./conv_36/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer36o.GetWorkLoad(layer36i,1,1);

    ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_37 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer37i.toString()<<"->"<<layer37o.toString()<<"\n"<<endl;
    analyze(layer37i,layer37o,1,1,1,1,path+"./conv_37/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer37o.GetWorkLoad(layer37i,1,1);

    ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_38 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer38i.toString()<<"->"<<layer38o.toString()<<"\n"<<endl;
    analyze(layer38i,layer38o,3,3,1,1,path+"./conv_38/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer38o.GetWorkLoad(layer38i,3,3);

    ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_39 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer39i.toString()<<"->"<<layer39o.toString()<<"\n"<<endl;
    analyze(layer39i,layer39o,1,1,1,1,path+"./conv_39/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer39o.GetWorkLoad(layer39i,1,1);

    ///Conv2d(1024, 256, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_40 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer40i.toString()<<"->"<<layer40o.toString()<<"\n"<<endl;
    analyze(layer40i,layer40o,1,1,1,1,path+"./conv_40/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer40o.GetWorkLoad(layer40i,1,1);

    ///Conv2d(256, 256, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_41 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer41i.toString()<<"->"<<layer41o.toString()<<"\n"<<endl;
    analyze(layer41i,layer41o,3,3,1,1,path+"./conv_41/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer41o.GetWorkLoad(layer41i,3,3);

    ///Conv2d(256, 1024, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_42 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer42i.toString()<<"->"<<layer42o.toString()<<"\n"<<endl;
    analyze(layer42i,layer42o,1,1,1,1,path+"./conv_42/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer42o.GetWorkLoad(layer42i,1,1);

    ///Conv2d(1024, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_43 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer43i.toString()<<"->"<<layer43o.toString()<<"\n"<<endl;
    analyze(layer43i,layer43o,1,1,1,1,path+"./conv_43/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer43o.GetWorkLoad(layer43i,1,1);

    ///Conv2d(512, 512, kernel_size=(3, 3), stride=(2, 2), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_44 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer44i.toString()<<"->"<<layer44o.toString()<<"\n"<<endl;
    analyze(layer44i,layer44o,3,3,2,2,path+"./conv_44/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer44o.GetWorkLoad(layer44i,3,3);

    ///Conv2d(512, 2048, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_45 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer45i.toString()<<"->"<<layer45o.toString()<<"\n"<<endl;
    analyze(layer45i,layer45o,1,1,1,1,path+"./conv_45/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer45o.GetWorkLoad(layer45i,1,1);

    ///Conv2d(1024, 2048, kernel_size=(1, 1), stride=(2, 2), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_46 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer46i.toString()<<"->"<<layer40o.toString()<<"\n"<<endl;
    analyze(layer46i,layer46o,1,1,2,2,path+"./conv_46/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer46o.GetWorkLoad(layer46i,1,1);

    ///Conv2d(2048, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_47 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer47i.toString()<<"->"<<layer47o.toString()<<"\n"<<endl;
    analyze(layer47i,layer47o,1,1,1,1,path+"./conv_47/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer47o.GetWorkLoad(layer47i,1,1);

    ///Conv2d(512, 512, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_48 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer48i.toString()<<"->"<<layer48o.toString()<<"\n"<<endl;
    analyze(layer48i,layer48o,3,3,1,1,path+"./conv_48/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer48o.GetWorkLoad(layer48i,3,3);

    ///Conv2d(512, 2048, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_49 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer49i.toString()<<"->"<<layer49o.toString()<<"\n"<<endl;
    analyze(layer49i,layer49o,1,1,1,1,path+"./conv_49/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer49o.GetWorkLoad(layer49i,1,1);

    ///Conv2d(2048, 512, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_50 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer50i.toString()<<"->"<<layer50o.toString()<<"\n"<<endl;
    analyze(layer50i,layer50o,1,1,1,1,path+"./conv_50/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer50o.GetWorkLoad(layer50i,1,1);

    ///Conv2d(512, 512, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_51 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer51i.toString()<<"->"<<layer51o.toString()<<"\n"<<endl;
    analyze(layer51i,layer51o,3,3,1,1,path+"./conv_51/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer51o.GetWorkLoad(layer51i,3,3);

    ///Conv2d(512, 2048, kernel_size=(1, 1), stride=(1, 1), bias=False)
    std::cout<<"##############"<<std::endl
             <<"##  conv_52 ##"<<std::endl
             <<"##############"<<std::endl
             <<layer52i.toString()<<"->"<<layer52o.toString()<<"\n"<<endl;
    analyze(layer52i,layer52o,1,1,1,1,path+"./conv_52/",totalSAWInSize,
                                                        totalSAXInSize,
                                                        totalCEXInSize,ofs);
    WorkLoad += layer52o.GetWorkLoad(layer52i,1,1);
*/
    std::cout<<"total workload: "<<WorkLoad<<std::endl;
    std::cout<<"totalSAWIn size: "<<totalSAWInSize<<std::endl;
    std::cout<<"totalSAXIn size: "<<totalSAXInSize<<std::endl;
    std::cout<<"totalCEXIn size: "<<totalCEXInSize<<std::endl;
    std::cout<<"feature access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/totalSAXInSize<<std::endl;
    std::cout<<" total  access reduce: "<<(totalSAXInSize-totalCEXInSize)*1.0/(totalSAXInSize+totalSAWInSize)<<std::endl;
    return;
}

#ifdef GENERATE_DATA
void Analyze::GenKernel(int kN,int kH,int kW,int kD,int zeroRatio,int _16bitRatio,const string& prefix){
    /// assert(kN%KERNEL_GROUP_SIZE==0);  wenzhi: now to remove

    std::ofstream kOfs(prefix+ KERNEL_FILE_PATH,
                        std::ofstream::out),
                  pOfs(prefix+PATTERN_FILE_PATH,
                        std::ofstream::out);

    const int groupNum     =  (kN + KERNEL_GROUP_SIZE -1) / KERNEL_GROUP_SIZE,
              groupPerLine = ((kD + GROUP_SIZE - 1) / GROUP_SIZE),
              kernelSize   =   kH * kW * groupPerLine * GROUP_SIZE;

    vector<bool> pattern(kernelSize);

    for (int g=0;g<groupNum;g++){
        int idx = 0;
        for (int w=0;w<kW;w++)
            for (int _g=0;_g<groupPerLine;_g++)
                for (int h=0;h<kH;h++)
                    for (int i=0;i<GROUP_SIZE;i++){
                        if (_g * GROUP_SIZE + i >= kD)
                            pattern[idx] = false;
                        else if (( w == (kW - 1))
                              && (_g == (groupPerLine -1))
                              && ( h == (kH - 1))
                              && ( i == (GROUP_SIZE - 1)))
                            pattern[idx] = true;
                        else
                            pattern[idx] = ((rand()%1000)>=zeroRatio);
                        pOfs << pattern[idx];
                        idx++;
                    }
        pOfs << std::endl;
        for (int k=0;k<KERNEL_GROUP_SIZE;k++){
            for (const auto& it : pattern){
                int tempValue;
                int additionalValue;
                if (it && ((k + g * KERNEL_GROUP_SIZE) < kN)){
                    if ((rand()%1000)<_16bitRatio){///16-bit
                        #ifndef AVOID_OVERFLOW
                        tempValue = (1<<(2*WEIGHT_BIT_WIDTH-1));
                        #else
                        tempValue = (1<<(2*WEIGHT_BIT_WIDTH-5));
                        #endif //AVOID_OVERFLOW
                        additionalValue = (1<<  WEIGHT_BIT_WIDTH);
                    }
                    else{/// 8-bit
                        tempValue = (1<<(  WEIGHT_BIT_WIDTH-1));
                        additionalValue = 0;
                    }
                    if (rand()%2==0)
                        kOfs <<" "<< (( (rand()%(tempValue-1))+1) + additionalValue);
                    else
                        kOfs <<" "<< ((-(rand()% tempValue   )-1) - additionalValue);
                }
                else
                    kOfs << " 0";
            }
            kOfs << std::endl;
        }
    }
    kOfs.close();
    pOfs.close();
    return;
}

void Analyze::GenFeature(int h,int w,int t,int zeroRatio,int _16bitRatio,const string& prefix){
    std::ofstream ofs(prefix+FEATURE_FILE_PATH,
                      std::ofstream::out);
    const int groupPerLine = ((t + GROUP_SIZE - 1)/GROUP_SIZE),
                correctedT = groupPerLine * GROUP_SIZE;
    for (int i=0;i<correctedT;i++){
        for (int j=0;j<h;j++){
            for (int k=0;k<w;k++){
                int tempValue;/// is used to generate different precision of value
                int additionalValue;
                if (i<t && rand()%1000>=zeroRatio){
                    if ((rand()%1000)<_16bitRatio){///16-bit
                        #ifndef AVOID_OVERFLOW
                        tempValue = (1<<(2*WEIGHT_BIT_WIDTH-1));
                        #else
                        tempValue = (1<<(2*WEIGHT_BIT_WIDTH-5));
                        #endif //AVOID_OVERFLOW
                        additionalValue = (1<<  WEIGHT_BIT_WIDTH);
                    }
                    else{ /// 8-bit
                        tempValue = (1<<(  WEIGHT_BIT_WIDTH-1));
                        additionalValue = 0;
                    }

                    if (rand()%2==0)
                        ofs<<" "<<(( (rand()%(tempValue-1))+1) + additionalValue);
                    else
                        ofs<<" "<<((-(rand()% tempValue   )-1) - additionalValue);
                }
                else
                    ofs<<" 0";
            }
            ofs<<std::endl;
        }
        ofs<<std::endl;
    }
    ofs.close();
    return;
}
#endif // GENERATE_DATA

#ifdef ANALYZE
int Analyze::PrintSysAnalReport(const Layer& thisLayer, const Systolic& sys,
                                const LayerDimension& thisLayerInfo,
                                const LayerDimension& lastLayerInfo,
                                int lastLayerCycle,int kH,int kW){
    int   SAUpCacheSize = sys.GetUpCacheSize();
    int    SAUpDataSize = sys.GetUpDataSize();
    int  SALeftDataSize = sys.GetLeftDataSize();
    int  SADownDataSize,SADownReluSize;
    sys.GetDownDataSize(thisLayer,SADownDataSize,SADownReluSize);
    int SADataReuse = sys.GetUpDataReuse(thisLayer);
    int SACyclseUse = sys.GetInputCyc(thisLayer);
    int SA_DRAM_Bandwidth = 0;

    std::cout<<"[systolic array] analyze result:"<<std::endl
             <<"\t[up] cache size:\t"   <<SAUpCacheSize <<"(bits)" <<std::endl
             <<"\t[up]  data size:\t"   <<SAUpDataSize  <<"(bits)" <<std::endl
             <<"\t[up]  data reuse:\t"  <<SADataReuse   <<"(times)"<<std::endl
             <<"\t[left] data size:\t"  <<SALeftDataSize<<"(bits)" <<std::endl
             <<"\t[down] output size:\t"<<SADownDataSize<<"(bits)" <<std::endl
             <<"\t[down] relued size:\t"<<SADownReluSize<<"(bits)" <<std::endl
             <<"\t[to RU] bandwidth:\t" <<SALeftDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8<<"(MBps)"<<std::endl;

    if (lastLayerCycle!=-1){
        std::cout<<"\t[to DRAM] bandwidth:\t"<<SAUpDataSize/lastLayerCycle*SYSTOLIC_ARRAY_FREQUENCE/8<<"(MBps)"<<std::endl;
        SA_DRAM_Bandwidth = SAUpDataSize/lastLayerCycle*SYSTOLIC_ARRAY_FREQUENCE/8;
    }
    std::cout<<"\tcycle:\t\t"<<SACyclseUse<<"(cyc)"  <<std::endl
             <<"\tequivalent ops: "<<1.0*thisLayerInfo.GetWorkLoad(lastLayerInfo,kH,kW)*SYSTOLIC_ARRAY_FREQUENCE/SACyclseUse<<" Tops ##"<<std::endl;

    fprintf(this->fp_SAUpCacheSize," %d",SAUpCacheSize);
    fprintf(this->fp_SAUpDataSize," %d",SAUpDataSize);
    fprintf(this->fp_SAUpDataReuse," %d",SADataReuse);
    fprintf(this->fp_SAUpDRAM_Bandwidth," %d",SA_DRAM_Bandwidth);
    fprintf(this->fp_SADownDataSize," %d",SADownDataSize);
    fprintf(this->fp_SADownReluSize," %d",SADownReluSize);
    fprintf(this->fp_SACycle," %d",SACyclseUse);
    fprintf(this->fp_SAEqualSpeed," %f",1.0*thisLayerInfo.GetWorkLoad(lastLayerInfo,kH,kW)*SYSTOLIC_ARRAY_FREQUENCE/SACyclseUse);
    return SACyclseUse;
}


void Analyze::PrintDFUAnalReport(const DFU& dfus,int SACyclseUse){
    std::cout<<"[DFU] analyze result:"<<std::endl;
    std::cout<<"\t[DRAM] memory access volume of this Layer: \t"
             <<1.0 * dfus.GetCurReadMem()
                   * WEIGHT_SIZE/1024.0/1024.0
             <<"(Mb)"
             <<std::endl;

    std::cout<<"\t[DRAM] memory access bandwidth of this Layer: \t"
             <<1.0 * dfus.GetCurReadMem()
                   * WEIGHT_SIZE
                   * SYSTOLIC_ARRAY_FREQUENCE
                   / SACyclseUse / 1024.0
             <<"(Gbps)"
             <<std::endl;

    for (int g=0;g<DFU_CACHE_NUM;g++){
        fprintf(this->fp_DFU_DRAM_Volume," %f",
            1.0*dfus.GetCurReadMem(g)*WEIGHT_SIZE/1024.0);
    }
    fprintf(this->fp_DFU_DRAM_Volume,"\n");

    for (int g=0;g<DFU_CACHE_NUM;g++){
        fprintf(this->fp_DFU_DRAM_BandWidth," %f",
            1.0*dfus.GetCurReadMem(g)*WEIGHT_SIZE
                 *SYSTOLIC_ARRAY_FREQUENCE/SACyclseUse/1024.0);
    }
    fprintf(this->fp_DFU_DRAM_BandWidth,"\n");
    return;
}

void Analyze::PrintRUAAnalReport(const RUArray& rua,int SACyclseUse){
    int  RUGlobLeftDataSize = rua. GetGlobDataSize(),
        RUInRowLeftDataSize = rua.GetInRowDataSize(),
             RULoadDataSize = rua. GetLoadDataSize(),
           RUUpLoadDataSize = rua.GetUpLoadDataSize(),
           RUUpDataReuseTime= rua.GetUpDataReuseTime(),
                RUCyclseUse = rua.GetInputCyc();

    float inRowReuse = rua.GetInRowReuseTime(),
           globReuse = rua.GetGlobReuseTime();

    std::cout<<"[RU array] analyze result:"<<std::endl
             <<"\t[left] in row data size:\t"<<RUInRowLeftDataSize<<"(bits)"<<std::endl
             <<"\t[left] global data size:\t"<<RUGlobLeftDataSize<<"(bits)"<<std::endl
             <<"\t[left] loaded data size:\t"<<RULoadDataSize<<"(bits)"<<std::endl
             <<"\t[left] in row reuse:\t"<<inRowReuse<<"(times)"<<std::endl
             <<"\t[left] global reuse:\t"<<globReuse<<"(times)"<<std::endl
             <<"\t[up] load data size:\t"<<RUUpLoadDataSize<<"(bits)"<<std::endl
             <<"\t[up] data reuse:\t"<<RUUpDataReuseTime<<"(times)"<<std::endl
             <<"\t[to SMU] bandwidth:\t"<<RULoadDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8<<"(MBps)"<<std::endl
             <<"\t[to DRAM] bandwidth (in row):\t"<<RUInRowLeftDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8<<"(MBps)"<<std::endl
             <<"\t[to DRAM] bandwidth (glob):\t"<<RUGlobLeftDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8<<"(MBps)"<<std::endl
             <<"\tcycle:\t"<<RUCyclseUse<<"(cyc)"<<std::endl;

    fprintf(this->fp_RULeftDataSizeInRow," %d",RUInRowLeftDataSize);
    fprintf(this->fp_RULeftDataSizeGlob," %d",RUGlobLeftDataSize);
    fprintf(this->fp_RUInRowReuse," %f",inRowReuse);
    fprintf(this->fp_RUGlobReuse," %f",globReuse);
    fprintf(this->fp_RUUpLoadDataSize," %d",RUUpLoadDataSize);
    fprintf(this->fp_RU_SMU_BandWidth," %d",RULoadDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8);
    fprintf(this->fp_RU_DRAM_BandWidth_InRow," %d",RUInRowLeftDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8);
    fprintf(this->fp_RU_DRAM_BandWidth_Glob," %d",RUGlobLeftDataSize/SACyclseUse*SYSTOLIC_ARRAY_FREQUENCE/8);
    fprintf(this->fp_RUCycle," %d",RUCyclseUse);

    return;
}
#endif // ANALYZE
