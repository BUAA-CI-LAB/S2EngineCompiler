#include "../include/Analyze.hpp"

void Analyze::PrintProcess(const char str[]){
    #ifdef PRINT_PROCESS
    cout<<str<<endl;
    #endif // PRINT_PROCESS
}

void Analyze::analyze(const LayerDimension& lastLayerInfo,
                      const LayerDimension& thisLayerInfo,
                      int kH,int kW,int sH,int sW,const string& prefix
                      #ifndef GENERATE_DATA
                      ,const string& weightFile
                      ,const string&   biasFile
                      ,const string&     ifFile
                      ,const string&     ofFile
                      #endif // GENERATE_DATA
                      ){
    #ifdef ANALYZE
    static int lastLayerCycle = -1;
    #endif // ANALYZE

    std::ofstream ofs (prefix+"workload.txt",std::ofstream::out);
    ofs<<thisLayerInfo.GetWorkLoad(lastLayerInfo,kH,kW)<<std::endl;
    ofs.close();

    #ifdef GENERATE_DATA
    GenKernel (thisLayerInfo.GetD(),
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
                    thisLayerInfo.GetPadding());
    Layer lastLayer(lastLayerInfo.GetH(),
                    lastLayerInfo.GetW(),
                    lastLayerInfo.GetD(),
                     3, 3, 1, 1,
                     GROUP_SIZE,
                    lastLayerInfo.GetPadding());///the remained three parameter does not matter

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
    thisLayer.Compute(lastLayer);
    #else
    PrintProcess("[this layer] check if the output feature matches");
    assert(thisLayer.Compute(lastLayer));
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
    #ifdef REFORMED
    PrintProcess("\t[compiler] cleaning WIn");
    sys.ClearWIn();
    #endif // REFORMED
    #ifdef NDEBUG
    PrintProcess("\t[compiler] cleaning WTrans");
    sys.ClearWTrans();
    #endif // NDEBUG
    PrintProcess("[systolic array] translating feature input(from logical location to actual data)");
    sys.TransXIn (thisLayer,lastLayer);
    #ifndef NDEBUG
    PrintProcess("[systolic array] checking if translated weight and feature match");
    assert(sys.CheckXW(thisLayer));
    #endif // NDEBUG
    #endif // TRANS_DATA

    #ifdef ANALYZE
    PrintProcess("[DFU] analyzing...");
    sys.CalcDFUDRAMAccess(thisLayer);
    #endif // ANALYZE

    #ifndef NDEBUG
    #ifdef PRINT_TO_FILE
    PrintProcess("[systolic array] printing weight input");
    sys.PrintTransedW(prefix);
    #endif // PRINT_TO_FILE
    PrintProcess("\t[compiler] cleaning WTrans");
    sys.ClearWTrans();
    #endif // NDEBUG

    #ifdef PRINT_TO_FILE
    PrintProcess("[systolic array] printing feature input");
    sys.PrintTransedX(prefix);
    PrintProcess("[systolic array] printing output");
    sys.PrintOutput(thisLayer,prefix);
    #endif // PRINT_TO_FILE

    #ifdef REFORMED
    PrintProcess("\t[compiler] cleaning XTrans");
    sys.ClearXTrans();
    #endif // REFORMED

    PrintProcess("\t[compiler] cleaning Output");
    sys.ClearOutput();

    #ifndef REFORMED
    PrintProcess("[RU array] generating weight location input");
    sys.GenRULIn();
    PrintProcess("\t[compiler] cleaning WIn");
    sys.ClearWIn();
    #endif // REFORMED
    PrintProcess("[RU array] generating feature input");
    sys.GenRUXIn(lastLayer,thisLayer);
    #ifdef ANALYZE
    PrintProcess("[RU array] analyzing...");
    rua.AnalyzeInputData(thisLayer,lastLayer);
    #endif // ANALYZE
    PrintProcess("\t[compiler] cleaning XIn");
    sys.ClearXIn();
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
    PrintProcess("\t[compiler] cleaning LIn");
    rua.ClearLIn();
    #endif // REFORMED
    PrintProcess("\t[compiler] cleaning this layer");
    thisLayer.Clean();
    #ifdef TRANS_DATA
    PrintProcess("[RU array] translating feature input(from the serial of group to actual data)");
    sys.TransRUXIn(lastLayer);
    #ifdef PRINT_TO_FILE
    PrintProcess("[RU array] printing translated feature");
    rua.PrintXData(prefix);
    #endif // PRINT_TO_FILE
    PrintProcess("\t[compiler] cleaning XIn");
    rua.ClearXIn();
    PrintProcess("\t[compiler] cleaning last layer");
    lastLayer.Clean();
    #ifndef REFORMED
    #ifndef NDEBUG
    PrintProcess("[RU array] checking the homogeneity of translated feature and location");
    assert(rua.CheckXLDataHomo());
    PrintProcess("[RU array] checking if the translated feature and location match");
    assert(sys.CheckRUXL());
    #endif // NDEBUG
    PrintProcess("\t[compiler] cleaning XTrans");
    sys.ClearXTrans();
    #endif // REFORMED
    #endif // TRANS_DATA

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
    return;
}

void Analyze::AnalyzeAlexNet(){
    this->totalCyc = 0;

    LayerDimension layer0(227,227,  3);
    LayerDimension layer1( 55, 55, 96);
    LayerDimension layer2( 27, 27, 96);
    LayerDimension layer3( 27, 27,256);
    LayerDimension layer4( 13, 13,256);
    LayerDimension layer5( 13, 13,384);
    LayerDimension layer6( 13, 13,256);

    double WorkLoad=0;

    #ifdef GENERATE_DATA
    this->MKDir("./AlexNet");
    this->MKLayerDir("./AlexNet","conv1");
    this->MKLayerDir("./AlexNet","conv2");
    this->MKLayerDir("./AlexNet","conv3");
    this->MKLayerDir("./AlexNet","conv4");
    this->MKLayerDir("./AlexNet","conv5");
    #endif // GENERATE_DATA

    std::cout<<"#############"<<std::endl
             <<"##  conv1  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer0.toString()<<"->"<<layer1.toString()<<std::endl;
    analyze(layer0,layer1,11,11,4,4,"./AlexNet/conv1/");
    WorkLoad += layer1.GetWorkLoad(layer0,11,11);

    std::cout<<"#############"<<std::endl
             <<"##  conv2  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer2.toString()<<"->"<<layer3.toString()<<"\n"<<std::endl;
    analyze(layer2,layer3, 5, 5,1,1,"./AlexNet/conv2/");
    WorkLoad += layer3.GetWorkLoad(layer2,5,5);

    std::cout<<"#############"<<std::endl
             <<"##  conv3  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer4.toString()<<"->"<<layer5.toString()<<"\n"<<std::endl;
    analyze(layer4,layer5, 3, 3,1,1,"./AlexNet/conv3/");
    WorkLoad += layer5.GetWorkLoad(layer4,3,3);

    std::cout<<"#############"<<std::endl
             <<"##  conv4  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<std::endl;
    analyze(layer5,layer5, 3, 3,1,1,"./AlexNet/conv4/");
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    std::cout<<"#############"<<std::endl
             <<"##  conv5  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer5.toString()<<"->"<<layer6.toString()<<"\n"<<std::endl;
    analyze(layer5,layer6, 3, 3,1,1,"./AlexNet/conv5/");
    WorkLoad += layer6.GetWorkLoad(layer5,3,3);

    #ifdef ANALYZE
        std::cout<<endl;
        std::cout<<"## total cycle(SA): "
                 <<this->totalCyc
                 <<" ##"
                 <<endl;
        std::cout<<"## DFU average bandwidth: "
                 <<1.0
                  *this->dfus.GetTotalReadMem()
                  *WEIGHT_SIZE
                  *SYSTOLIC_ARRAY_FREQUENCE
                  /this->totalCyc
                  /1024.0
                 <<" GBps ##"
                 <<endl;
        std::cout<<"## equivalent ops: "
                 <<1.0
                  *WorkLoad
                  *SYSTOLIC_ARRAY_FREQUENCE
                  /this->totalCyc
                 <<" Tops ##"
                 <<endl;
    #endif // ANALYZE
}

void Analyze::AnalyzeRealAlexNet(std::string path){
    this->totalCyc = 0;

    LayerDimension layer0(227,227,  3);
    LayerDimension layer1( 55, 55, 96);
    LayerDimension layer2( 27, 27, 96);
    LayerDimension layer3( 27, 27,256);
    LayerDimension layer4( 13, 13,256);
    LayerDimension layer5( 13, 13,384);
    LayerDimension layer6( 13, 13,256);

    double WorkLoad=0;

    std::cout<<"#############"<<std::endl
             <<"##  conv1  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer0.toString()<<"->"<<layer1.toString()<<std::endl;
    analyze(layer0,layer1,11,11,4,4,path+"./conv_0/","weights","input_features","");
    WorkLoad += layer1.GetWorkLoad(layer0,11,11);

    std::cout<<"#############"<<std::endl
             <<"##  conv2  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer2.toString()<<"->"<<layer3.toString()<<"\n"<<std::endl;
    analyze(layer2,layer3, 5, 5,1,1,path+"./conv_1/");
    WorkLoad += layer3.GetWorkLoad(layer2,5,5);

    std::cout<<"#############"<<std::endl
             <<"##  conv3  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer4.toString()<<"->"<<layer5.toString()<<"\n"<<std::endl;
    analyze(layer4,layer5, 3, 3,1,1,path+"./conv_2/");
    WorkLoad += layer5.GetWorkLoad(layer4,3,3);

    std::cout<<"#############"<<std::endl
             <<"##  conv4  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<std::endl;
    analyze(layer5,layer5, 3, 3,1,1,path+"./conv_3/");
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    std::cout<<"#############"<<std::endl
             <<"##  conv5  ##"<<std::endl
             <<"#############"<<std::endl;
    std::cout<<layer5.toString()<<"->"<<layer6.toString()<<"\n"<<std::endl;
    analyze(layer5,layer6, 3, 3,1,1,path+"./conv_4/");
    WorkLoad += layer6.GetWorkLoad(layer5,3,3);
    return;
}

void Analyze::AnalyzeVGG16(){
    this->totalCyc = 0;

    LayerDimension layer0(224,224,  3);
    LayerDimension layer1(224,224, 64);
    LayerDimension layer2(112,112, 64);
    LayerDimension layer3(112,112,128);
    LayerDimension layer4( 56, 56,128);
    LayerDimension layer5( 56, 56,256);
    LayerDimension layer6( 28, 28,256);
    LayerDimension layer7( 28, 28,512);
    LayerDimension layer8( 14, 14,512);

    double WorkLoad=0;

    #ifdef GENERATE_DATA
    this->MKDir("./VGG16");
    this->MKLayerDir("./VGG16","conv1_1");
    this->MKLayerDir("./VGG16","conv1_2");
    this->MKLayerDir("./VGG16","conv2_1");
    this->MKLayerDir("./VGG16","conv2_2");
    this->MKLayerDir("./VGG16","conv3_1");
    this->MKLayerDir("./VGG16","conv3_2");
    this->MKLayerDir("./VGG16","conv3_3");
    this->MKLayerDir("./VGG16","conv4_1");
    this->MKLayerDir("./VGG16","conv4_2");
    this->MKLayerDir("./VGG16","conv4_3");
    this->MKLayerDir("./VGG16","conv5_1");
    this->MKLayerDir("./VGG16","conv5_2");
    this->MKLayerDir("./VGG16","conv5_3");
    #endif // GENERATE_DATA

    std::cout<<"###############"<<std::endl
             <<"##  conv1_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer0.toString()<<"->"<<layer1.toString()<<std::endl;
    analyze(layer0,layer1,3,3,1,1,"./VGG16/conv1_1/");
    WorkLoad += layer1.GetWorkLoad(layer0,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv1_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer1.toString()<<"->"<<layer1.toString()<<"\n"<<endl;
    analyze(layer1,layer1,3,3,1,1,"./VGG16/conv1_2/");
    WorkLoad += layer1.GetWorkLoad(layer1,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv2_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer2.toString()<<"->"<<layer3.toString()<<"\n"<<endl;
    analyze(layer2,layer3,3,3,1,1,"./VGG16/conv2_1/");
    WorkLoad += layer3.GetWorkLoad(layer2,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv2_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer3.toString()<<"->"<<layer3.toString()<<"\n"<<endl;
    analyze(layer3,layer3,3,3,1,1,"./VGG16/conv2_2/");
    WorkLoad += layer3.GetWorkLoad(layer3,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer4.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer4,layer5,3,3,1,1,"./VGG16/conv3_1/");
    WorkLoad += layer5.GetWorkLoad(layer4,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3,1,1,"./VGG16/conv3_2/");
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3,1,1,"./VGG16/conv3_3/");
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer6.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer6,layer7,3,3,1,1,"./VGG16/conv4_1/");
    WorkLoad += layer7.GetWorkLoad(layer6,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3,1,1,"./VGG16/conv4_2/");
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3,1,1,"./VGG16/conv4_3/");
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3,1,1,"./VGG16/conv5_1/");
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3,1,1,"./VGG16/conv5_2/");
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3,1,1,"./VGG16/conv5_3/");
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    #ifdef ANALYZE
        cout<<endl;
        cout<<"## total cycle(SA): "
            <<this->totalCyc
            <<" ##"
            <<endl;
        cout<<"## DFU average bandwidth: "
            <<1.0
             *this->dfus.GetTotalReadMem()
             *WEIGHT_SIZE
             *SYSTOLIC_ARRAY_FREQUENCE
             /this->totalCyc
             /1024.0
            <<" GBps ##"
            <<endl;
        cout<<"## equivalent ops: "
            <<1.0
             *WorkLoad
             *SYSTOLIC_ARRAY_FREQUENCE
             /this->totalCyc
            <<" Tops ##"
            <<endl;
    #endif // ANALYZE
    return;
}

void Analyze::AnalyzeRealVGG16(std::string path){
    this->totalCyc = 0;

    LayerDimension layer0(224,224,  3,ZERO_PAD);
    LayerDimension layer1(224,224, 64,ZERO_PAD);
    LayerDimension layer2(112,112, 64,ZERO_PAD);
    LayerDimension layer3(112,112,128,ZERO_PAD);
    LayerDimension layer4( 56, 56,128,ZERO_PAD);
    LayerDimension layer5( 56, 56,256,ZERO_PAD);
    LayerDimension layer6( 28, 28,256,ZERO_PAD);
    LayerDimension layer7( 28, 28,512,ZERO_PAD);
    LayerDimension layer8( 14, 14,512,ZERO_PAD);

    double WorkLoad=0;

    std::cout<<"###############"<<std::endl
             <<"##  conv1_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer0.toString()<<"->"<<layer1.toString()<<std::endl;
    analyze(layer0,layer1,3,3,1,1,path+"./conv_0/");
    WorkLoad += layer1.GetWorkLoad(layer0,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv1_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer1.toString()<<"->"<<layer1.toString()<<"\n"<<endl;
    analyze(layer1,layer1,3,3,1,1,path+"./conv_1/");
    WorkLoad += layer1.GetWorkLoad(layer1,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv2_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer2.toString()<<"->"<<layer3.toString()<<"\n"<<endl;
    analyze(layer2,layer3,3,3,1,1,path+"./conv_2/");
    WorkLoad += layer3.GetWorkLoad(layer2,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv2_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer3.toString()<<"->"<<layer3.toString()<<"\n"<<endl;
    analyze(layer3,layer3,3,3,1,1,path+"./conv_3/");
    WorkLoad += layer3.GetWorkLoad(layer3,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer4.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer4,layer5,3,3,1,1,path+"./conv_4/");
    WorkLoad += layer5.GetWorkLoad(layer4,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3,1,1,path+"./conv_5/");
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv3_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3,1,1,path+"./conv_6/");
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer6.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer6,layer7,3,3,1,1,path+"./conv_7/");
    WorkLoad += layer7.GetWorkLoad(layer6,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3,1,1,path+"./conv_8/");
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv4_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3,1,1,path+"./conv_9/");
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_1  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3,1,1,path+"./conv_10/");
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_2  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3,1,1,path+"./conv_11/");
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    std::cout<<"###############"<<std::endl
             <<"##  conv5_3  ##"<<std::endl
             <<"###############"<<std::endl
             <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3,1,1,path+"./conv_12/");
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);
    return;
}

void Analyze::AnalyzeVGG19(){
    this->totalCyc = 0;

    LayerDimension layer0(224,224,GROUP_SIZE);
    LayerDimension layer1(224,224,64);
    LayerDimension layer2(112,112,64);
    LayerDimension layer3(112,112,128);
    LayerDimension layer4(56,56,128);
    LayerDimension layer5(56,56,256);
    LayerDimension layer6(28,28,256);
    LayerDimension layer7(28,28,512);
    LayerDimension layer8(14,14,512);

    double WorkLoad=0;

    cout<<"#############"<<endl;
    cout<<"## layer0  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer0.toString()<<"->"<<layer1.toString()<<endl;
    analyze(layer0,layer1,3,3);
    WorkLoad += layer1.GetWorkLoad(layer0,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer1  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer1.toString()<<"->"<<layer1.toString()<<"\n"<<endl;
    analyze(layer1,layer1,3,3);
    WorkLoad += layer1.GetWorkLoad(layer1,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer2  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer2.toString()<<"->"<<layer3.toString()<<"\n"<<endl;
    analyze(layer2,layer3,3,3);
    WorkLoad += layer3.GetWorkLoad(layer2,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer3  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer3.toString()<<"->"<<layer3.toString()<<"\n"<<endl;
    analyze(layer3,layer3,3,3);
    WorkLoad += layer3.GetWorkLoad(layer3,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer4  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer4.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer4,layer5,3,3);
    WorkLoad += layer5.GetWorkLoad(layer4,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer5  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3);
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer6  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3);
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer7  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer5.toString()<<"->"<<layer5.toString()<<"\n"<<endl;
    analyze(layer5,layer5,3,3);
    WorkLoad += layer5.GetWorkLoad(layer5,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer8  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer6.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer6,layer7,3,3);
    WorkLoad += layer7.GetWorkLoad(layer6,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer9  ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3);
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer10 ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3);
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer11 ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer7.toString()<<"->"<<layer7.toString()<<"\n"<<endl;
    analyze(layer7,layer7,3,3);
    WorkLoad += layer7.GetWorkLoad(layer7,3,3);

    std::cout<<std::endl
        <<"#############"<<std::endl
        <<"## layer12 ##"<<std::endl
        <<"#############"<<std::endl
        <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<std::endl;
    analyze(layer8,layer8,3,3);
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    std::cout<<std::endl
        <<"#############"<<std::endl
        <<"## layer13 ##"<<std::endl
        <<"#############"<<std::endl
        <<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<std::endl;
    analyze(layer8,layer8,3,3);
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer14 ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3);
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    cout<<endl;
    cout<<"#############"<<endl;
    cout<<"## layer15 ##"<<endl;
    cout<<"#############"<<endl;
    cout<<layer8.toString()<<"->"<<layer8.toString()<<"\n"<<endl;
    analyze(layer8,layer8,3,3);
    WorkLoad += layer8.GetWorkLoad(layer8,3,3);

    #ifdef ANALYZE
        cout<<endl;
        cout<<"## total cycle(SA): "
            <<this->totalCyc
            <<" ##"
            <<endl;
        cout<<"## DFU average bandwidth: "
            <<1.0
             *this->dfus.GetTotalReadMem()
             *WEIGHT_SIZE
             *SYSTOLIC_ARRAY_FREQUENCE
             /this->totalCyc
             /1024.0
            <<" GBps ##"
            <<endl;
        cout<<"## equivalent ops: "
            <<1.0
             *WorkLoad
             *SYSTOLIC_ARRAY_FREQUENCE
             /this->totalCyc
            <<" Tops ##"
            <<endl;
    #endif // ANALYZE
}

#ifdef GENERATE_DATA
void Analyze::GenKernel(int kN,int kH,int kW,int kD,int zeroRatio,int _16bitRatio,const string& prefix){
    assert(kN%KERNEL_GROUP_SIZE==0);

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
                            pattern[idx] = ((rand()%100)>=zeroRatio);
                        pOfs << pattern[idx];
                        idx++;
                    }
        pOfs << std::endl;
        for (int k=0;k<KERNEL_GROUP_SIZE;k++){
            for (const auto& it : pattern){
                int tempValue;
                int additionalValue;
                if (it){
                    if ((rand()%100)<_16bitRatio){///16-bit
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
                if (i<t && rand()%100>=zeroRatio){
                    if ((rand()%100)<_16bitRatio){///16-bit
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
