#include "../include/net.hpp"


const string LayerType[]={
/** CONV_LAYER = 0 **/ "conv",
/** POOL_LAYER = 1 **/ "pool",
/** RELU_LAYER = 2 **/ "relu",
/** ADD_LAYER  = 3 **/ "add",
/** INPUT_LAYER= 4 **/ "input"
};

int Layer::idxCounter=0;

bool Kernel::CheckPattern(const vector<bool>& pattern){
    assert(this->initial.size() == pattern.size());
    for (unsigned int i=0;i<this->initial.size();i++)
        if ((  pattern[i] &&(this->initial[i]==0))
         || ((!pattern[i])&&(this->initial[i]!=0))){
            std::cout<<"Error @"<<i<<std::endl;
            return false;
         }
    return true;
}

void Kernel::Reorder(const vector<int>& reorderSeq,const vector<bool>& pattern){
    this->reordered.clear();
    this->reordered.reserve(reorderSeq.size()*GROUP_SIZE);

    for (const auto& it : reorderSeq){
        #ifdef REMOVE_ALL_ZERO_GROUP
        assert(false);
        #endif // REMOVE_ALL_ZERO_GROUP
        bool allZero = true;
        for (uint32_t j=0;j<GROUP_SIZE;j++)
            if (pattern[it*GROUP_SIZE+j]){
                allZero = false;
                this->reordered.emplace_back(this->initial[it*GROUP_SIZE+j]);
            }
        if (allZero){
            /// wenzhi: a zero in all zero group has been inserted in the GenReorderSeq process
            assert(false);
            this->reordered.emplace_back(0);
        }
    }
    assert(this->reordered.size() >= MINIMA_WEIGHT_INTERVAL);
    this->reordered.shrink_to_fit();
    return;
}

void Kernel::PrintReorderedTo(FILE *fp)const {
    assert(this->reordered.size()!=0);
    for (unsigned int i=0;i<this->reordered.size();i++){
        #ifdef REFORMED
        assert(false);
        #endif // REFORMED
        #ifdef PRINT_TO_FILE
        if (i<this->reordered.size()-1){
            #ifndef MIXED_PRECISION
                fprintf(fp,"%4d 0\n",this->reordered[i]);
            #else
                if (MyMath::Is16Bit(this->reordered[i])){
                    fprintf(fp,"%s 0 1\n",std::to_string(MyMath:: LowBits(this->reordered[i])).c_str());/// weight isOut is16bit
                    fprintf(fp,"%s 0 1\n",std::to_string(MyMath::HighBits(this->reordered[i])).c_str());/// weight isOut is16bit
                }
                else
                    fprintf(fp,"%s 0 0\n",std::to_string(this->reordered[i]).c_str());/// weight isOut is8bit
            #endif // MIXED_PRECISION
        }
        else{
            #ifndef MIXED_PRECISION
                fprintf(fp,"%4d 1\n",this->reordered[i]);
            #else
                if (MyMath::Is16Bit(this->reordered[i])){
                    fprintf(fp,"%s 1 1\n",std::to_string(MyMath:: LowBits(this->reordered[i])).c_str());/// weight isOut is16bit
                    fprintf(fp,"%s 1 1\n",std::to_string(MyMath::HighBits(this->reordered[i])).c_str());/// weight isOut is16bit
                }
                else
                    fprintf(fp,"%s 1 0\n",std::to_string(this->reordered[i]).c_str());/// weight isOut is8bit
            #endif // MIXED_PRECISION
        }
        #endif // PRINT_TO_FILE
    }
    #ifdef PRINT_TO_FILE
    fprintf(fp,"\n");
    #endif // PRINT_TO_FILE
    return;
}

#ifndef REFORMED
void Kernel::PrintReorderedTo(vector<WTransIn>& wTrans)const{
#else
void Kernel::PrintReorderedTo(vector<SparseDataInFIFO<WTransIn> >& wTrans,const vector<vector<SparseLoc> >& kernelLoc)const{
#endif // REFORMED
    assert(this->reordered.size()!=0);
    #ifdef REFORMED
    uint32_t   groupIdx = 0,
             inGroupIdx = 0;
    #endif // REFORMED
    for (unsigned int i=0;i<this->reordered.size();i++){
        #ifndef REFORMED
            wTrans.emplace_back(this->reordered[i],i==(this->reordered.size()-1));
        #else
            assert(  groupIdx < kernelLoc.size());
            assert(inGroupIdx < kernelLoc[groupIdx].size());
            wTrans.emplace_back(
                    WTransIn(this->reordered[i],i==(this->reordered.size()-1)),
                    kernelLoc[groupIdx][inGroupIdx],
                    (inGroupIdx==(kernelLoc[groupIdx].size()-1))
                );
            if (inGroupIdx==(kernelLoc[groupIdx].size()-1)){
                inGroupIdx = 0;
                  groupIdx++;
            }
            else
                inGroupIdx++;
        #endif // REFORMED
    }
    #ifdef REFORMED
    assert(inGroupIdx == 0 && groupIdx==kernelLoc.size());
    #endif // REFORMED
    return;
}

Net::Net(ifstream& netArch){
    int layerNum;
    if (!netArch){
        cout<<"netArch file do not exist"<<endl;
        return;
    }
    netArch>>layerNum;

    for (int i=0;i<layerNum;i++){
        netArch.get();
        char layerType;
        netArch>>layerType;
        int h,w,t,name;
        int pre,kH,kW,sH,sW,fN;
        switch(layerType){
        case 'I':
            netArch>>h;
            netArch>>w;
            netArch>>t;
            netArch>>name;
            cout<<layerType<<endl;
            cout<<"h:"<<h<<" w:"<<w<<" t:"<<t<<" name:"<<name<<endl;
            break;
        case 'C':
            netArch>>pre;
            netArch>>kH;
            netArch>>kW;
            netArch>>sH;
            netArch>>sW;
            netArch>>fN;
            netArch>>name;
            cout<<layerType<<endl;

            cout<<"pre:"<<pre<<" kH:"<<kH<<" kW:"<<kW
                <<" sH:"<<sH<<" sW:"<<sW<<" fN:"<<fN<<" name:"<<name<<endl;
            break;
        case 'R':
            netArch>>pre;
            netArch>>name;
            cout<<layerType<<endl;

            cout<<"pre:"<<pre<<" name:"<<name<<endl;
            break;
        default:
            assert(false);
        }
    }
}

#ifndef REFORMED
void Layer::MatchXTo(int x,int y,int kG,Layer& nextLayer,vector<XTransIn>& xTrans){
#else
void Layer::MatchXTo(int x,int y,int kG,Layer& nextLayer,vector<SparseDataInFIFO<XTransIn::FeatureType> >& xTrans){
#endif //REFORMED
    assert(this->kD%GROUP_SIZE==0
        && this->hasPartFeature);
    const uint32_t groupPerLine = (this->kN / GROUP_SIZE);
    for (const auto& it : nextLayer.reorderSeq[kG]){
        const int h = this->KernelGroupIdxToH(it,groupPerLine,nextLayer.kH),
                  w = this->KernelGroupIdxToW(it,groupPerLine,nextLayer.kH,nextLayer.kW),
                  k = this->KernelGroupIdxToK(it,groupPerLine);
        int thisX = 0,thisY = 0;
        if (nextLayer.padding==SAME_PAD){
            thisX = (x+w)<0?0:
                    (x+w)<this->lW?
                    (x+w):this->lW-1;
            thisY = (y+h)<0?0:
                    (y+h)<this->lH?
                    (y+h):this->lH-1;
        }
        else if (nextLayer.padding==ZERO_PAD){
            thisX = (x+w);
            thisY = (y+h);
            if (thisX < 0 || thisX >= this->lW
             || thisY < 0 || thisY >= this->lH){
                #ifdef REFORMED
                xTrans.emplace_back(0,FEATURE_FILL_ZERO_POSITION,1);
                #else
                const int kB = k * GROUP_SIZE;
                for (int i=0;i<GROUP_SIZE;i++)
                    if(nextLayer.pattern[kG][it*GROUP_SIZE+i])
                        xTrans.emplace_back(0);
                #endif // REFORMED
                continue;
            }
        }
        else
            assert(false);

        #ifdef REFORMED
        const int fGroupIdx = this->FeatureGroupIdx((this->kN / GROUP_SIZE),thisY,thisX,k);
        this->getSparseFeatureGroup(fGroupIdx,xTrans);
        #else
        const int kB = k * GROUP_SIZE;
        for (int i=0;i<GROUP_SIZE;i++)
            if(nextLayer.pattern[kG][it*GROUP_SIZE+i])
                xTrans.emplace_back(this->feature[kB+i][thisY][thisX]);
        #endif // REFORMED
    }
    return;
}

#ifdef GENERATE_DATA
void Layer::Compute(const Layer& lastLayer){
#else
bool Layer::Compute(const Layer& lastLayer){
#endif // GENERATE_DATA
    #ifdef GENERATE_DATA
    assert(!this->hasLoadFeature);
    #else
    assert(this->hasLoadFeature);
    assert(this->hasLoadBias);
    #endif // GENERATE_DATA
    assert(this->padding == SAME_PAD
        || this->padding == ZERO_PAD);
    assert(this->kH%2==1&&this->kW%2==1);
    int halfW = this->kW/2,
        halfH = this->kH/2;

    for (int k=0;k<this->kN;k++)
        for (int y=0;y<this->lH;y++)
            for (int x=0;x<this->lW;x++){
                XTransIn::FeatureType tempFeature;
                #ifdef GENERATE_DATA
                tempFeature = 0;
                #else
                tempFeature = this->bias[k];
                #endif // GENERATE_DATA
                for (int w=-halfW,kw=0;w<=halfW;w++,kw++){
                    for (int h=-halfH,kh=0;h<=halfH;h++,kh++){
                        int lastX = x * this->sW + w,
                            lastY = y * this->sH + h;
                        if (this->padding == SAME_PAD){
                            lastX = lastX>=0?lastX:0;
                            lastX = lastX>=lastLayer.getLW()?lastLayer.getLW()-1:lastX;
                            lastY = lastY>=0?lastY:0;
                            lastY = lastY>=lastLayer.getLH()?lastLayer.getLH()-1:lastY;
                        }
                        else if (this->padding == ZERO_PAD){
                            if (lastX<0 || lastX>=lastLayer.getLW()
                             || lastY<0 || lastY>=lastLayer.getLH())
                                continue;
                        }
                        else/// other padding style has not been exploited
                            assert(false);

                        lastLayer.AddInnerProduct(
                            lastX,lastY,this->kernel[k],
                            #ifdef XUCHENG_PROTOCOL
                            (kw*this->kH+kh)*this->kD,this->kD,
                            #else
                            (kw*this->kH+kh)*this->kD,this->kD,
                            #endif // XUCHENG_PROTOCOL
                            tempFeature
                        );
                        /// to calculate the accuracy output value
                    }
                }
                #ifdef GENERATE_DATA
                this->feature[k][y][x] = tempFeature;
                #else
                if (this->feature[k][y][x] != tempFeature){
                    std::cout<<"k:"<<k<<" y:"<<y<<" x:"<<x<<std::endl;
                    std::cout<<"loaded feature:"<<this->feature[k][y][x]<<std::endl;
                    std::cout<<"actual output :"<<tempFeature<<std::endl;
                    return false;
                }
                this->feature[k][y][x] -= this->bias[k];
                #endif // GENERATE_DATA
            }
    #ifdef GENERATE_DATA
    this->hasLoadFeature = true;
    return;
    #else
    return true;
    #endif
}

inline void Layer::AddInnerProduct(int x,int y,const Kernel& k,int kBegin,int kLen,XTransIn::FeatureType& parsum)const{
    assert(kLen==this->kN);
    for (int i=0;i<this->kN;i++)
        parsum += this->feature[i][y][x] * k.GetWeight(kBegin+i);
    return;
}

void Layer::PartitionFeature(){
    assert(this->hasLoadFeature
        &&!this->hasPartFeature
        &&(this->kD % GROUP_SIZE ==0));
    this->allGroupSize = 0;
    int groupPerLine = this->kN / GROUP_SIZE;
    this->sparseFeature.clear();
    this->sparseFeature.reserve(this->lH * this->lW * groupPerLine);
    for (int h=0;h<this->lH;h++)
        for (int w=0;w<this->lW;w++)
            for (int g=0;g<groupPerLine;g++){
                bool allZero = true;
                this->sparseFeature.emplace_back();
                this->sparseFeature.back().reserve(GROUP_SIZE);
                for (int i=0;i<GROUP_SIZE;i++)
                    if (this->feature[g*GROUP_SIZE+i][h][w]!=0){
                        allZero = false;
                        this->sparseFeature.back()
                            .emplace_back(this->feature[g*GROUP_SIZE+i][h][w],i);
                    }
                if (allZero){
                    this->sparseFeature.back().emplace_back(0,FEATURE_FILL_ZERO_POSITION);
                }
                this->sparseFeature.back().shrink_to_fit();
                this->allGroupSize+=this->sparseFeature.back().size();
            }
    this->hasPartFeature = true;
    return;
}

#ifndef GENERATE_DATA
void Layer::LoadBias  (const string& prefix){
    #ifndef XUCHENG_PROTOCOL
    assert(false);/// wenzhi: unspecified input protocol
    #endif // XUCHENG_PROTOCOL
    assert(!this->hasLoadBias);
    std::ifstream ifs(prefix + BIAS_FILE_PATH,std::ifstream::in);

    if (!ifs){
        std::cout<<"Open file error! [load bias]"<<std::endl;
        return;
    }

    std::string tempS;
    std::getline(ifs,tempS);

    for (auto& bit : this->bias){
        double tempFloat;
        ifs>>tempFloat;
        bit = (int64_t)tempFloat;
    }
    this->hasLoadBias = true;
    return;
}
#endif // GENERATE_DATA

#ifdef GENERATE_DATA
void Layer::LoadKernel(const string& prefix){
    assert(!this->hasLoadKernel);

    const int kernelSize = this->kH * this->kW * this->kD;
    std::ifstream ifs(prefix + KERNEL_FILE_PATH,std::ifstream::in);
    if (!ifs){
        std::cout<<"Open file error! [load kernel]:"<<prefix + KERNEL_FILE_PATH<<std::endl;
        return;
    }
    for (int k=0;k<this->kN;k++){
        for (int i=0;i<kernelSize;i++){
            WTransIn::WeightType value;
            ifs >> value;
            this-> kernel[k].AddValue(i,(WTransIn::WeightType)value);
        }
    }
    this->hasLoadKernel  = true;
    ifs.close();
    return;
}
#else
void Layer::LoadKernel(const string& prefix,int actD,int D){
    assert(!this->hasLoadKernel
        && !this->hasLoadPattern);

    const int columNum = this->kH * this->kW;
    std::ifstream ifs(prefix + KERNEL_FILE_PATH,std::ifstream::in);
    if (!ifs){
        std::cout<<"Open file error! [load kernel]:"<<prefix + KERNEL_FILE_PATH<<std::endl;
        return;
    }
    #ifdef XUCHENG_PROTOCOL
    std::string tempS;
    std::getline(ifs,tempS);
    #else
    assert(false);///wenzhi: unspecified input protocol
    #endif // XUCHENG_PROTOCOL
    for (int k=0;k<this->kN;k++){
        for (int c=0;c<columNum;c++){
            for (int i=0;i<actD;i++){
                float value;
                ifs >> value;
                this-> kernel[k].AddValue(c*D + i,(WTransIn::WeightType)value);
                this->pattern[k][c*D + i] = (((WTransIn::WeightType)value)!=0);
            }
            for (int i=actD;i<D;i++){
                this-> kernel[k].AddValue(c*D + i,0);
                this->pattern[k][c*D + i] = false;
            }
        }
    }
    this->hasLoadKernel  = true;
    this->hasLoadPattern = true;
    ifs.close();
    return;
}
#endif // GENERATE_DATA

#ifdef GENERATE_DATA
void Layer::LoadPattern(const string& prefix){
    assert(this->kN % KERNEL_GROUP_SIZE ==0
        &&!this->hasLoadPattern);
    const uint32_t kernelSize = this->kH * this->kW * this->kD;

    std::ifstream ifs(prefix + PATTERN_FILE_PATH,std::ifstream::in);

    if (!ifs){
        std::cout<<"Open file error! [load pattern]"<<std::endl;
        return;
    }
    for (auto& git : this->pattern){
        assert(git.size()==kernelSize);
        for (uint32_t i=0;i<kernelSize;i++){
            char c;
            ifs.get(c);
            while (c!='0' && c!='1'){
                assert(c!=EOF);
                ifs.get(c);
            }
            git[i]=(c=='1');
        }
    }
    this->hasLoadPattern = true;
    ifs.close();
    return;
}
#endif // GENERATE_DATA

#ifdef GENERATE_DATA
void Layer::LoadFeature(const string& prefix,const string& fileName){
    assert(!this->hasLoadFeature);

    std::ifstream ifs(prefix + fileName,std::ifstream::in);

    if (!ifs){
        std::cout<<"Open file error! [load feature]"<<std::endl;
        return;
    }
    for (auto& kit : this->feature)
        for (auto& hit : kit)
            for (auto& wit : hit){
                XTransIn::FeatureType tempFeature;
                ifs>>tempFeature;
                wit = tempFeature;
            }
    this->hasLoadFeature = true;
    ifs.close();
    return;
}
#else
void Layer::LoadFeature(const string& prefix,int actD,int D, const string& fileName){
    assert(!this->hasLoadFeature);

    std::ifstream ifs(prefix + fileName,std::ifstream::in);

    if (!ifs){
        std::cout<<"Open file error! [load feature]"<<std::endl;
        return;
    }
    #ifdef XUCHENG_PROTOCOL
    std::string tempS;
    std::getline(ifs,tempS);
    #else
    assert(false);
    #endif // XUCHENG_PROTOCOL
    for (int k=0;k<actD;k++)
        #ifdef XUCHENG_MISTAKE
        for (int w=0;w<this->lW;w++)
            for (int h=0;h<this->lH;h++){
        #else
        for (auto& hit : this->feature[k])
            for (auto& wit : hit){
        #endif // XUCHENG_MISTAKE
                #ifdef XUCHENG_PROTOCOL
                float tempFeature;
                #else
                XTransIn::FeatureType tempFeature;
                #endif // XUCHENG_PROTOCOL
                ifs>>tempFeature;
                #ifdef XUCHENG_MISTAKE
                this->feature[k][h][w] = (XTransIn::FeatureType)tempFeature;
                #else
                wit = (XTransIn::FeatureType)tempFeature;
                #endif // XUCHENG_MISTAKE
            }
    for (int k=actD; k<D;k++)
        for (auto& hit : this->feature[k])
            for (auto& wit : hit)
                wit = 0;
    this->hasLoadFeature = true;
    ifs.close();
    return;
}
#endif // GENERATE_DATA

bool Layer::CheckPattern(){
    assert(this->kN%KERNEL_GROUP_SIZE == 0);
    int kernelGroupNum = this->kN/KERNEL_GROUP_SIZE;
    for (int g=0;g<kernelGroupNum;g++)
        for (int k=0;k<KERNEL_GROUP_SIZE;k++)
            if (!this->kernel[g*KERNEL_GROUP_SIZE + k].CheckPattern(this->pattern[g])){
                std::cout<<"\tin"<<k<<std::endl;
                return false;
            }
    return true;
}
bool Layer::CheckReshapedSize(){
    assert(this->kN%KERNEL_GROUP_SIZE == 0);
    int kernelGroupNum = this->kN/KERNEL_GROUP_SIZE;
    for (int g=0;g<kernelGroupNum;g++)
        for (int k=1;k<KERNEL_GROUP_SIZE;k++)
            if (this->kernel[g*KERNEL_GROUP_SIZE  ].GetReorderedSize()
             != this->kernel[g*KERNEL_GROUP_SIZE+k].GetReorderedSize())
                return false;
    return true;
}

#ifndef REFORMED
void Layer::PrintKernelTo(int kIdx,vector<WTransIn>& wTran)const{
#else
void Layer::PrintKernelTo(int kIdx,vector<SparseDataInFIFO<WTransIn> >& wTran)const{
#endif /// REFORMED
    if(kIdx==IDLE){
        #ifdef REMOVE_ALL_ZERO_GROUP
        assert(false);
        #endif // REMOVE_ALL_ZERO_GROUP
        for (uint32_t i=0;i<this->reorderSeq[0].size()-1;i++){
            #ifndef REFORMED
            wTran.emplace_back(0,false);
            #else
            wTran.emplace_back(WTransIn(0,false),0,true);
            #endif ///REFORMED
        }
        #ifndef REFORMED
        wTran.emplace_back(0,true);
        #else
        wTran.emplace_back(WTransIn(0,true),0,true);
        #endif ///REFORMED
        return;
    }
    assert(kIdx>=0 && kIdx<((int)this->kernel.size()));
    #ifndef REFORMED
    this->kernel[kIdx].PrintReorderedTo(wTran);
    #else
    this->kernel[kIdx].PrintReorderedTo(wTran,this->kernelLoc[this->KerIdx2GroupIdx(kIdx)]);
    #endif // REFORMED
    return;
}

void Layer::GenReorderSeq(){
    assert(this->kD%GROUP_SIZE==0
        &&!this->hasGenReorderSeq);
    int groupPerLine = this->kD / GROUP_SIZE;
    /** group per line of kernel
     ** (thus, the thickness of last layer)**/

    const uint32_t
        kSize = this->kH
              * this->kW
              * this->kD;

    assert(this->pattern.size() == (unsigned)(this->kN/KERNEL_GROUP_SIZE));
    assert(this->pattern[0].size()==kSize);

    for (int kG = 0;kG<(this->kN/KERNEL_GROUP_SIZE);kG++){
        uint32_t groupSerial = 0;
        this-> kernelLoc[kG].clear();
        this-> kernelLoc[kG].reserve(this->kW * this->kH * groupPerLine);
        this->reorderSeq[kG].clear();
        this->reorderSeq[kG].reserve(this->kW * this->kH * groupPerLine);

        int nonZeroNum = 0;
        for (int g=0;g<groupPerLine * this->kW * this->kH;g++){
            const int groupBase = g * GROUP_SIZE;
            bool allZero = true;
            for (int i=0;i<GROUP_SIZE;i++)
                if (this->pattern[kG][groupBase+i]){
                    allZero = false;
                    nonZeroNum++;
                }
            if (allZero){
                this->pattern[kG][groupBase + KERNEL_FILL_ZERO_POSITION] = true;
                nonZeroNum++;
            }
        }

        for (int w=0;w<this->kW;w++)
            for (int g=0;g<groupPerLine;g++)
                for (int h=0;h<this->kH;h++){
                    const int nowGroup  = this->KernelGroupIdx((this->kD / GROUP_SIZE),h,w,g);
                    const int groupBase = nowGroup * GROUP_SIZE;
                    #ifdef REMOVE_ALL_ZERO_GROUP
                    assert(false);/** an unsuccessful attempt here by wenzhi ... **/
                    #endif //REMOVE_ALL_ZERO_GROUP
                    assert(this->reorderSeq[kG].size()==groupSerial);
                    this-> kernelLoc[kG].emplace_back();
                    this->reorderSeq[kG].emplace_back(nowGroup);

                    bool allZero = true;
                    this-> kernelLoc[kG].back().reserve(GROUP_SIZE);
                    for (int i=0;i<GROUP_SIZE;i++)
                        if (this->pattern[kG][groupBase+i]){
                            allZero = false;
                            this->kernelLoc[kG].back().emplace_back(i);
                        }
                        else{
                            if (nonZeroNum < MINIMA_WEIGHT_INTERVAL){
                                allZero = false;
                                this->pattern[kG][groupBase+i] = true;
                                this->kernelLoc[kG].back().emplace_back(i);
                                nonZeroNum++;
                            }
                        }
                    if (allZero){
                        assert(false);/// wenzhi: a zero in all zero group has been inserted
                        this->  pattern[kG][groupBase  ] = true;
                        this->kernelLoc[kG].back().emplace_back(KERNEL_FILL_ZERO_POSITION);
                    }
                    this->kernelLoc[kG].back().shrink_to_fit();
                    groupSerial++;
                }
    }
    this->hasGenReorderSeq = true;
    return;
}

void Layer::Reorder(){
    assert(this->kN % KERNEL_GROUP_SIZE ==0
        && this->hasGenReorderSeq);
    int groupNum = this->kN / KERNEL_GROUP_SIZE;
    for (int g=0;g<groupNum;g++){
        for (int k = 0;k<KERNEL_GROUP_SIZE;k++){
            this->kernel[g*KERNEL_GROUP_SIZE + k].Reorder(this->reorderSeq[g],this->pattern[g]);
            assert(this->kernel[g*KERNEL_GROUP_SIZE+k].GetReorderedSize()
                == this->kernel[g*KERNEL_GROUP_SIZE  ].GetReorderedSize());
            #ifdef PRINT_INTERMEDIA_INFO
            this->kernel[g*KERNEL_GROUP_SIZE + k].PrintReorderedTo(this->reshapedW);
            #endif // PRINT_INTERMEDIA_INFO
        }
    }
    return;
}

#ifdef PRINT_INTERMEDIA_INFO
void Layer::PrintFeature(){
    for (int k=0;k<this->kN;k++){
        for (int y=0;y<this->lH;y++){
            for (int x=0;x<this->lW;x++){
                #ifdef PRINT_TO_STD
                cout<<this->feature[k][y][x]<<" ";
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                fprintf(fpFeature,"%d ",this->feature[k][y][x]);
                #endif // PRINT_TO_FILE
            }
            #ifdef PRINT_TO_STD
            cout<<endl;
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(fpFeature,"\n");
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(fpFeature,"\n");
        #endif // PRINT_TO_FILE
    }
}

void Layer::PrintReorderedLoc(){
    for (unsigned int kG=0;kG<this->kernelLoc.size();kG++){
        assert(this->kernelLoc[kG].size()!=0);
        for (unsigned int g=0;g<this->kernelLoc[kG].size();g++){
            for (unsigned int i=0;i<this->kernelLoc[kG][g].size();i++){
                #ifdef PRINT_TO_STD
                cout.width(4);
                if (i == this->kernelLoc[kG][g].size() - 1)
                    cout<<"("<<this->kernelLoc[kG][g][i].GetLoc()<<",1)"<<endl;
                else
                    cout<<"("<<this->kernelLoc[kG][g][i].GetLoc()<<",0)"<<endl;
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                if (i == this->kernelLoc[kG][g].size() - 1)
                    fprintf(this->reshapedLoc,"(%4d,1)",this->kernelLoc[kG][g][i].GetLoc());
                else
                    fprintf(this->reshapedLoc,"(%4d,0)",this->kernelLoc[kG][g][i].GetLoc());
                #endif // PRINT_TO_FILE
            }
            #ifdef PRINT_TO_STD
            cout<<"#"<<endl;
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(this->reshapedLoc,"#\n");
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->reshapedLoc,"\n");
        #endif // PRINT_TO_FILE
    }
    return;
}

void Layer::PrintReorder(){
    assert(this->reorderSeq[0].size()!=0);
    for (unsigned int g=0;g<this->reorderSeq.size();g++){
        for (unsigned int i=0;i<this->reorderSeq[g].size();i++){
            #ifdef PRINT_TO_STD
            cout.width(4);
            cout<<this->reorderSeq[g][i]<<endl;
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(this->reshapedG,"%4d",this->reorderSeq[g][i]);
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->reshapedG,"\n");
        #endif // PRINT_TO_FILE
    }
    return;
}
#endif // PRINT_INTERMEDIA_INFO
