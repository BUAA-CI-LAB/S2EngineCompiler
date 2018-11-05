#include "../include/SysSim.hpp"

void Systolic::MapToSA(Layer& layer){

    assert(layer.getType()==CONV_LAYER);
    assert(!this->hasMap);

    const int lH = layer.getLH(),
              lW = layer.getLW();
    const int kN = layer.getKN();

	int pos[SYS_HEIGHT][2];
    /** the position of each PE's     *
     *  workload in the output layer **/

    const int plateNum = (kN + (SYS_WIDTH * SYS_GROUP) - 1) / (SYS_WIDTH * SYS_GROUP);
    /** the workload of each PE for    *
     *  the computation of a position **/

    int sH = 0;
    /// h w: the location used of the position in next layer
    for (int w=0;w<lW;w++)
        for (int h=0;h<lH;h++){
            pos[sH][0] = h;
            pos[sH][1] = w;
            sH++;
            if (sH == SYS_HEIGHT){
                /// complete the mapping of the full systolic array
                /// _h _w _g: the location used for the systolic array
                for (int _h=0;_h<SYS_HEIGHT;_h++){
                    int nowKN = 0;
                    for (int i=0;i<plateNum;i++)
                        for (int _g=0;_g<SYS_GROUP;_g++)
                            for (int _w=0;_w<SYS_WIDTH;_w++){
                                if (nowKN < kN){
                                    this->pe[_g][_h][_w].Alloc(pos[_h][0],pos[_h][1],nowKN);
                                    nowKN ++;
                                }
                                else
                                    this->pe[_g][_h][_w].Idle();
                            }
                    pos[_h][0] = -1;
                    pos[_h][1] = -1;
                }
                sH = 0;
            }
        }

    if (sH!=0){
        for (int _h=0;_h<SYS_HEIGHT;_h++){
            int nowKN = 0;
            for (int i=0;i<plateNum;i++)
                for (int _g=0;_g<SYS_GROUP;_g++)
                    for (int _w=0;_w<SYS_WIDTH;_w++){
                        if (_h >= sH){
                            this->pe[_g][_h][_w].Idle();
                            continue;
                        }
                        if (nowKN < kN){
                            this->pe[_g][_h][_w].Alloc(pos[_h][0],pos[_h][1],nowKN);
                            nowKN ++;
                        }
                        else
                            this->pe[_g][_h][_w].Idle();
                    }
            pos[_h][0] = -1;
            pos[_h][1] = -1;
        }
    }
    for (auto& git : this->pe)
        for (auto& hit : git)
            for (auto& wit : hit)
                wit.shrink_to_fit();

    assert(this->CheckPEHomo());
    #ifdef PRINT_INTERMEDIA_INFO
    this->PrintWorkLoad();
    this->PrintPE();
    #endif // PRINT_INTERMEDIA_INFO
    this->hasMap = true;
    return;
}

void Systolic::PrintOutput(const Layer& thisLayer,const std::string& prefix) const{
    assert(this->oHasGen);
    for (auto git = this->XOut.cbegin();git != this->XOut.cend();git++)
        for (auto wit = git->cbegin();wit != git->cend();wit++){
            std::ofstream ofs ((prefix + SA_OUTPUT_FILE_PATH_PREFIX
                              + std::to_string((git - this->XOut.cbegin())*SYS_WIDTH
                                              +(wit - git->cbegin()))
                              + TRANS_FILE_TYPE),
                               std::ofstream::out);
            if (!ofs){
                std::cout<<"Open File Error [Output Feature]"<<std::endl;
                return;
            }
            for (uint32_t i=0;i<wit->GetWorkLoad();i++){
                if (wit->IfIdle(i))
                    ofs<<"0 ";
                else
                    ofs<<thisLayer.getFeature(
                            wit->GetX(i),
                            wit->GetY(i),
                            wit->GetK(i))
                       <<" ";
                if (i % 2 != 0)
                    ofs<<std::endl;
            }
            ofs.close();
        }
    return;
}

void Systolic::PrintTransedX(const std::string& prefix) const{
    assert(this->xHasTra);
    for (auto git = this->XTran.cbegin();git != this->XTran.cend();git++)
        for (auto hit = git->cbegin();hit != git->cend();hit++){
            #ifndef REFORMED
            std::ofstream ofs ((prefix + SA_XIN_TRANS_FILE_PATH_PREFIX
                              + std::to_string(git - this->XTran.begin())+"_"
                              + std::to_string(hit - git->cbegin())
                              + TRANS_FILE_TYPE),
                                std::ofstream::out);
            #else
            std::ofstream ofs ((prefix + REFORMED_SA_XIN_TRANS_FILE_PATH_PREFIX
                              + std::to_string(hit - git->cbegin())
                              + TRANS_FILE_TYPE),
                                std::ofstream::out);
            #endif // REFORMED
            if (!ofs){
                std::cout<<"Open File Error [Trans Feature]"<<std::endl;
                return;
            }
            #ifndef REFORMED
                for (const auto& it : *hit){
                    #ifndef MIXED_PRECISION
                        /// feature
                        ofs<<it.GetValue()<<" ";
                    #else
                        if (MyMath::Is16Bit(it.GetValue())){
                            /// feature is16bit
                            ofs<<MyMath:: LowBits(it.GetValue())<<" 1 "<<std::endl
                               <<MyMath::HighBits(it.GetValue())<<" 1 "<<std::endl;
                        }
                        else{
                            /// feature is8bit
                            ofs<<it.GetValue()<<" 0 "<<std::endl;
                        }
                    #endif /// MIXED_PRECISION
                }
            #else
                for (const auto& it : *hit){
                    #ifndef MIXED_PRECISION
                        /// feature offset EOG
                        ofs<<it.GetValue()<<" "
                           <<it.GetLoc  ()<<" "
                           <<it.IsEOG   ()<<" "<<std::endl;
                    #else
                        if (MyMath::Is16Bit(it.GetValue())){
                            /// feature offset EOG is16bit
                            ofs<<MyMath:: LowBits(it.GetValue())<<" "<<it.GetLoc()<<" "<<it.IsEOG()<<" 1 "<<std::endl
                               <<MyMath::HighBits(it.GetValue())<<" "<<it.GetLoc()<<" "<<it.IsEOG()<<" 1 "<<std::endl;
                        }
                        else{
                            /// feature offset EOG is8bit
                            ofs<<it.GetValue()<<" "<<it.GetLoc()<<" "<<it.IsEOG()<<" 0 "<<std::endl;
                        }
                    #endif /// MIXED_PRECISION
                }
            #endif // REFORMED
            ofs.close();
        }
    return;
}

void Systolic::PrintTransedW(const std::string& prefix) const{
    assert(this->wHasTra);
    for (auto git = this->WTran.cbegin();git != this->WTran.cend();git++)
        for (auto wit = git->cbegin();wit != git->cend();wit++){
            std::ofstream ofs ((prefix + SA_WIN_TRANS_FILE_PATH_PREFIX
                              + std::to_string((git - this->WTran.cbegin())*SYS_WIDTH
                                              +(wit - git->cbegin()))
                              + TRANS_FILE_TYPE),
                                std::ofstream::out);
            if (!ofs){
                std::cout<<"Open File Error [Trans Weight]"<<std::endl;
                return;
            }
            #ifndef REFORMED
            for (const auto& it : *wit){
                #ifndef MIXED_PRECISION
                    ofs<<it.GetW() <<" "
                       <<it.IfEOK()<<" "; /// weight isEOK
                #else
                    if (MyMath::Is16Bit(it.GetW())){
                        /// weight isEOK is16bit
                        ofs<<MyMath:: LowBits(it.GetW())<<" "<<it.IfEOK()<<" 1 "<<std::endl
                           <<MyMath::HighBits(it.GetW())<<" "<<it.IfEOK()<<" 1 "<<std::endl;
                    }
                    else{
                        /// weight isEOK  is8bit
                        ofs<<it.GetW()<<" "<<it.IfEOK()<<" 0 "<<std::endl;
                    }
                #endif /// MIXED_PRECISION
            }
            #else
            for (const auto& it : *wit){
                #ifndef MIXED_PRECISION
                    /// weight isEOK offset EOG
                    ofs<<it.GetValue().GetW ()<<" "
                       <<it.GetValue().IfEOK()<<" "
                       <<it.GetLoc()<<" "
                       <<it.IsEOG ()<<" "<<std::endl;
                #else
                    if (MyMath::Is16Bit(it.GetValue().GetW())){
                        /// weight isEOK offset EOG is16bit
                        ofs<<MyMath:: LowBits(it.GetValue().GetW())<<" "<<it.GetValue().IfEOK()<<" "
                           <<it.GetLoc()<<" "<<it.IsEOG()<<" 1 "<<std::endl
                           <<MyMath::HighBits(it.GetValue().GetW())<<" "<<it.GetValue().IfEOK()<<" "
                           <<it.GetLoc()<<" "<<it.IsEOG()<<" 1 "<<std::endl;
                    }
                    else{
                        /// weight isEOK offset EOG is8bit
                        ofs<<it.GetValue().GetW  ()<<" "<<it.GetValue().IfEOK()<<" "
                           <<it.GetLoc()<<" "<<it.IsEOG()<<" 0 "<<std::endl;
                    }
                #endif /// MIXED_PRECISION
            }
            #endif // REFORMED
            ofs.close();
        }
    return;
}

void Systolic::TransWIn(const Layer& layer){
    assert(this->workLoadHasGen
        && this->wHasGen
        &&!this->wHasTra);
    const int workLoad = this->pe[0][0][0].GetWorkLoad();

    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++){
            #ifdef PRINT_TRANS_W_IN_PROCESS
            cout<<"(g:"<<g<<",w:"<<w<<"): "
                <<(SA_WIN_TRANS_FILE_PATH_PREFIX+String::NumToString(g*SYS_WIDTH+w)+TRANS_FILE_TYPE)
                <<endl;
            #endif // PRINT_TRANS_W_IN_PROCESS
            this->WTran[g][w].reserve(workLoad * layer.getKernelSize());
            for (int i=0;i<workLoad;i++){
                layer.PrintKernelTo((this->WIn[g][w].GetIdx(i)),this->WTran[g][w]);
                if (this->WIn[g][w].GetIdx(i)==IDLE)
                    assert(this->saWorkLoad[g].GetWWorkLoad(i)==layer.getKernelWorkLoad(IDLE));
            }
            this->WTran[g][w].shrink_to_fit();
        }
    this->wHasTra = true;
    return;
}

void Systolic::TransXIn(Layer& thisLayer,Layer& lastLayer){
    assert(this->workLoadHasGen
        && thisLayer.getKW() % 2==1
        && thisLayer.getKH() % 2==1
        && this->CheckXInWorkLoad()
        && this->xHasGen
        &&!this->xHasTra);

    #ifdef REFORMED
    constexpr int g = 0;
    #else
    for (int g=0;g<SYS_GROUP;g++)
    #endif // REFORMED
    {
        for (int h=0;h<SYS_HEIGHT;h++){
            #ifdef PRINT_TRANS_X_IN_PROCESS
            cout<<"(g:"<<g<<",h:"<<h<<"): "
                <<(SA_XIN_TRANS_FILE_PATH_PREFIX+String::NumToString(g)+"_"+String::NumToString(h)+TRANS_FILE_TYPE)
                <<endl;
            #endif // PRINT_TRANS_X_IN_PROCESS

            this->XTran[g][h].reserve(this->XIn[g][h].GetWorkLoad() * thisLayer.getKernelSize());

            for (uint32_t i=0;i<this->XIn[g][h].GetWorkLoad();i++){
                if (this->pe[g][h][0].IfWork(i)){
                    lastLayer.MatchXTo(
                        this->XIn[g][h]   .GetX(i) * thisLayer.getSW(),
                        this->XIn[g][h]   .GetY(i) * thisLayer.getSH(),
                        this-> pe[g][h][0].GetK(i) / SYS_WIDTH,
                        thisLayer,this->XTran[g][h]
                    );
                }
                else{
                    #ifdef REFORMED
                    for (int i=0;i<thisLayer.getKernelGroupNum();i++)
                        this->XTran[g][h].emplace_back(0,FEATURE_FILL_ZERO_POSITION,true);
                    #else
                    for (int j=0;j<this->saWorkLoad[g].GetXWorkLoad(i);j++)
                        this->XTran[g][h].emplace_back((XTransIn::FeatureType)0);
                    #endif // REFORMED
                }
            }
            #ifndef REFORMED
            this->XTran[g][h].shrink_to_fit();
            #endif // REFORMED
        }
    }
    this->xHasTra = true;
    return;
}

void Systolic::GenWInput(Layer& layer){
    assert(this->CheckPEW() &&  !this->wHasGen);

    const int workLoad = this->pe[0][0][0].GetWorkLoad();

    this->reserveWIn(workLoad);

    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++)
            for (int i=0;i<workLoad;i++){
                if (this->pe[g][0][w].IfWork(i))
                    this->WIn[g][w].AddInputW(this->pe[g][0][w].GetK(i));
                else
                    this->WIn[g][w].AddZeroW();
            }
    assert(this->CheckWInWorkLoad());
    #ifdef PRINT_INTERMEDIA_INFO
    this->PrintWIn();
    #endif // PRINT_INTERMEDIA_INFO
    this->wHasGen = true;
    return;
}

void Systolic::GenXInput(Layer& layer){
    assert(this->CheckPEX() && !this->xHasGen);

    const int workLoad = this->pe[0][0][0].GetWorkLoad();

    this->reserveXIn(workLoad);

    #ifndef REFORMED
    for (int g=0;g<SYS_GROUP;g++)
    #else
    constexpr int g = 0;
    #endif //REFORMED
        for (int h=0;h<SYS_HEIGHT;h++)
            for (int i=0;i<workLoad;i++){
                if (this->pe[g][h][0].IfWork(i))
                    this->XIn[g][h].AddInputX(
                        this->pe[g][h][0].GetX(i),
                        this->pe[g][h][0].GetY(i));
                else{
                    if (this->saWorkLoad[g].IfAny(i))
                        this->XIn[g][h].AddAny();
                    else
                        this->XIn[g][h].AddZeroX();
                }
            }
    assert(this->CheckXInWorkLoad());
    #ifdef PRINT_INTERMEDIA_INFO
    this->PrintXIn();
    #endif // PRINT_INTERMEDIA_INFO
    this->xHasGen = true;
    return;
}

void Systolic::GenWorkLoad(Layer& layer){
    assert(!this->workLoadHasGen
        &&  this->hasMap);
    for (int g=0;g<SYS_GROUP;g++){
        this->saWorkLoad[g].reserve(this->pe[g][0][0].GetWorkLoad());
        for (uint32_t i=0;i<this->pe[g][0][0].GetWorkLoad();i++){
            int workLoad = 0;
            for (uint32_t h=0;h<SYS_HEIGHT;h++)
                for (uint32_t w=0;w<SYS_WIDTH;w++)
                    if (this->pe[g][h][w].IfWork(i)){
                        int k = this->pe[g][h][w].GetK(i);
                        if (workLoad == 0)
                            workLoad  = layer.getKernelWorkLoad(k);
                        else if (workLoad!=layer.getKernelWorkLoad(k)){
                            cout<<"workload un-match"<<endl;
                            assert(false);
                        }
                    }
            if (workLoad==0){
                #ifdef REMOVE_ALL_ZERO_GROUP
                assert(false);
                #endif // REMOVE_ALL_ZERO_GROUP
                this->saWorkLoad[g].AddWorkLoad(layer.getKernelWorkLoad(IDLE),true);
            }
            else
                this->saWorkLoad[g].AddWorkLoad(workLoad);
        }
    }
    this->workLoadHasGen=true;
    return;
}

void Systolic::GenOutput(Layer& layer){
    assert(this->CheckPEX() && !this->oHasGen);

    int workLoad = this->pe[0][0][0].GetWorkLoad();

    this->reserveXOut(SYS_HEIGHT * workLoad);

    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++)
            for (int i=0;i<workLoad;i++)
                for (int h=0;h<SYS_HEIGHT;h++){
                    #ifdef PE_IDLE_NOT_OUTPUT
                    assert(false);
                    #else
                    /** this idle PE will receive zero as X-input
                     ** and output zero
                     **/
                    if (this->pe[g][h][w].IfWork(i)){
                        this->XOut[g][w].AddOutputX(
                            this->pe[g][h][w].GetX(i),
                            this->pe[g][h][w].GetY(i),
                            this->pe[g][h][w].GetK(i));
                    }
                    else
                        this->XOut[g][w].AddZeroXOut();
                    #endif // PE_IDLE_NOT_OUTPUT
                }
    this->oHasGen = true;
    #ifdef PRINT_INTERMEDIA_INFO
    this->PrintXOut();
    #endif // PRINT_INTERMEDIA_INFO
    assert(this->CheckXOutWorkLoad());
    return;
}

bool Systolic::CheckXOutWorkLoad() const{
    #ifdef PE_IDLE_NOT_OUTPUT
    assert(false);
    #else
    assert(this->oHasGen);
    const uint32_t workLoad =
        SYS_HEIGHT*this->pe[0][0][0].GetWorkLoad();
    #endif // PE_IDLE_NOT_OUTPUT
    for (const auto& git : this->XOut)
        for (const auto& wit : git)
            if (wit.GetWorkLoad() != workLoad)
                return false;
    return true;
}
bool Systolic::CheckXInWorkLoad() const{
    const uint32_t workLoad = this->pe[0][0][0].GetWorkLoad();
    #ifndef REFORMED
    for (const auto& git : this->XIn)
    #else
    const auto& git = this->XIn[0];
    #endif // REFORMED
        for (const auto& hit : git)
            if (hit.GetWorkLoad() != workLoad)
                return false;
    return true;
}
bool Systolic::CheckWInWorkLoad() const{
    const uint32_t workLoad = this->pe[0][0][0].GetWorkLoad();
    for (const auto& git : this->WIn)
        for (const auto& wit : git)
            if (wit.GetWorkLoad() != workLoad){
                return false;
            }
    return true;
}
bool Systolic::CheckPEHomo() const{
    const uint32_t workLoad = this->pe[0][0][0].GetWorkLoad();
    for (int g=0;g<SYS_GROUP;g++)
        for (int h=0;h<SYS_HEIGHT;h++)
            for (int w=0;w<SYS_WIDTH;w++)
                if (this->pe[g][h][w].GetWorkLoad() != workLoad){
                    return false;
                }
    return true;
}
bool Systolic::CheckPEW() const{
    assert(this->CheckPEHomo());
    const int workLoad = this->pe[0][0][0].GetWorkLoad();
    for (int i=0;i<workLoad;i++)
        for (int g=0;g<SYS_GROUP;g++)
            for (int w=0;w<SYS_WIDTH;w++){
                int k;
                if (this->pe[g][0][w].IfWork(i)){
                    k = this->pe[g][0][w].GetK(i);
                    for (int h=1;h<SYS_HEIGHT;h++)
                        if (this->pe[g][h][w].IfWork(i)
                        &&  this->pe[g][h][w].GetK(i)!=k){
                            return false;
                        }
                }
                else
                    for (int h=1;h<SYS_HEIGHT;h++)
                        if (this->pe[g][h][w].IfWork(i)){
                            assert(false);
                            return false;
                        }
            }
    return true;
}
bool Systolic::CheckPEX() const{
    assert(this->CheckPEHomo());
    int workLoad = this->pe[0][0][0].GetWorkLoad();
    for (int i=0;i<workLoad;i++)
        for (int h=1;h<SYS_HEIGHT;h++){
            int x,y;
            if (!this->pe[0][h][0].IfWork(i)){
                for (int g=0;g<SYS_GROUP;g++)
                    for (int w=0;w<SYS_WIDTH;w++)
                        if (this->pe[g][h][w].IfWork(i))
                            return false;
            }
            else{
                x = pe[0][h][0].GetX(i);
                y = pe[0][h][0].GetY(i);
                for (int g=0;g<SYS_GROUP;g++)
                    for (int w=0;w<SYS_WIDTH;w++)
                        if (   this->pe[g][h][w].IfWork(i)
                        &&( x!=this->pe[g][h][w].GetX(i)
                        ||  y!=this->pe[g][h][w].GetY(i)))
                            return false;
            }
        }
    return true;
}

void Systolic::reserveXOut(int _size){
    for (auto& git : this->XOut)
        for (auto& wit : git)
            wit.reserve(_size);
    return;
}

void Systolic::reserveXIn(int _size){
    for (auto& git : this->XIn)
        for (auto& hit : git)
            hit.reserve(_size);
    return;
}

void Systolic::reserveWIn(int _size){
    for (auto& git : this->WIn)
        for (auto& wit : git)
            wit.reserve(_size);
    return;
}

bool Systolic::CheckXW(Layer& thisLayer) const{
    assert(this->wHasTra
        && this->xHasTra
        && this->oHasGen);
    #ifndef REFORMED
    uint32_t lastSite[SYS_HEIGHT];
    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++){
            int outputSerial = 0;
            memset(lastSite,0,sizeof(lastSite));
            for (uint32_t i=0;i<this->pe[g][0][0].GetWorkLoad();i++){
                for (int h=0;h<SYS_HEIGHT;h++){
                    #ifdef PRINT_CHECK_XW_PROCESS
                    cout<<"(g:"<<g<<",h:"<<h<<",w:"<<w<<")"<<endl;
                    #endif // PRINT_CHECK_XW_PROCESS
                    assert(this->XTran[g][h].size()==this->WTran[g][w].size());
                    XTransIn::FeatureType t = 0;
                    while (lastSite[h]<this->WTran[g][w].size()){
                        t +=  this->XTran[g][h][lastSite[h]].GetValue()
                            * this->WTran[g][w][lastSite[h]].GetW();
                        if (this->WTran[g][w][lastSite[h]].IfEOK()){
                            lastSite[h]++;
                            break;
                        }
                        lastSite[h]++;
                        assert(lastSite[h]<this->WTran[g][w].size());
                    }
                    if (!this->XOut[g][w].IfIdle(outputSerial)){
                        if (!thisLayer.FeatureEqual(
                            this->XOut[g][w].GetX(outputSerial),
                            this->XOut[g][w].GetY(outputSerial),
                            this->XOut[g][w].GetK(outputSerial),t)){
                                cout<<"CheckXW fail!"<<endl;
                                cout<<"("<<this->XOut[g][w].GetX(outputSerial)<<","
                                         <<this->XOut[g][w].GetY(outputSerial)<<","
                                         <<this->XOut[g][w].GetK(outputSerial)<<")@["<<g<<","<<w<<"]"<<endl;
                                return false;
                        }
                    }
                    else
                        if (t!=0){
                            cout<<"outputSerial:"<<outputSerial<<endl;
                            cout<<"(g:"<<g<<",h:"<<h<<",w:"<<w<<"):"<<t<<endl;
                            cout<<"CheckXW fail!"<<endl;
                            return false;
                        }
                    outputSerial++;
                }
            }
        }
    #else
    uint32_t lastWIdx[SYS_HEIGHT];
    uint32_t lastXIdx[SYS_HEIGHT];
    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++){
            int outputSerial = 0;
            memset(lastWIdx,0,sizeof(lastWIdx));
            memset(lastXIdx,0,sizeof(lastXIdx));
            for (uint32_t i=0;i<this->pe[g][0][0].GetWorkLoad();i++)
                for (int h=0;h<SYS_HEIGHT;h++){
                    #ifdef PRINT_CHECK_XW_PROCESS
                    std::cout<<"(g:"<<g<<",h:"<<h<<",w:"<<w<<")"<<std::endl;
                    std::cout<<"XTran size:"<<this->XTran[0][h].size()
                            <<",WTran size:"<<this->WTran[g][w].size()<<std::endl;
                    std::cout<<"wIdx:"<<lastWIdx[h]
                            <<" xIdx:"<<lastXIdx[h]<<std::endl;///wenzhi
                    #endif // PRINT_CHECK_XW_PROCESS
                    XTransIn::FeatureType tempTrue  = 0;
                    XTransIn::FeatureType tempRound = 0;

                    while (lastWIdx[h]<this->WTran[g][w].size()
                         &&lastXIdx[h]<this->XTran[0][h].size()){
                        int tempWIdx,tempXIdx;
                        if (this->AlignWX(this->WTran[g][w],lastWIdx[h],tempWIdx,
                                          this->XTran[0][h],lastXIdx[h],tempXIdx))
                            break;
                        tempTrue += this->XTran[0][h][tempXIdx].GetValue()
                                  * this->WTran[g][w][tempWIdx].GetValue().GetW();

                        tempRound = MyMath::Add(/// a = a + b
                                tempRound,///  low * low
                                MyMath::Mult(MyMath:: LowBits(this->XTran[0][h][tempXIdx].GetValue()),
                                             MyMath:: LowBits(this->WTran[g][w][tempWIdx].GetValue().GetW()))
                            );
                        tempRound = MyMath::Add(/// a = a + b
                                tempRound,/// high * low
                                MyMath::Mult(MyMath::HighBits(this->XTran[0][h][tempXIdx].GetValue()),
                                             MyMath:: LowBits(this->WTran[g][w][tempWIdx].GetValue().GetW()))<<WEIGHT_BIT_WIDTH
                            );
                        tempRound = MyMath::Add(/// a = a + b
                                tempRound,///  low * high
                                MyMath::Mult(MyMath:: LowBits(this->XTran[0][h][tempXIdx].GetValue()),
                                             MyMath::HighBits(this->WTran[g][w][tempWIdx].GetValue().GetW()))<<WEIGHT_BIT_WIDTH
                            );
                        tempRound = MyMath::Add(/// a = a + b
                                tempRound,/// high * high
                                MyMath::Mult(MyMath::HighBits(this->XTran[0][h][tempXIdx].GetValue()),
                                             MyMath::HighBits(this->WTran[g][w][tempWIdx].GetValue().GetW()))<<(2*WEIGHT_BIT_WIDTH)
                            );
                        if (this->WTran[g][w][tempWIdx].GetValue().IfEOK())
                            break;
                    }
                    if (tempTrue != tempRound)
                        std::cout<<"#rounding error# true:"<<tempTrue
                                            <<" ,actually:"<<tempRound<<std::endl;
//                    assert(tempTrue == tempRound);
                    if (!this->XOut[g][w].IfIdle(outputSerial)){
                        if (!thisLayer.FeatureEqual(
                            this->XOut[g][w].GetX(outputSerial),
                            this->XOut[g][w].GetY(outputSerial),
                            this->XOut[g][w].GetK(outputSerial),tempTrue)){
                                cout<<"CheckXW fail!"<<endl;
                                return false;
                        }
                        else{
                            thisLayer.SetFeature(
                                this->XOut[g][w].GetX(outputSerial),
                                this->XOut[g][w].GetY(outputSerial),
                                this->XOut[g][w].GetK(outputSerial),tempRound);
                        }
                    }
                    else
                        if (tempTrue!=0 || tempRound!=0){
                            cout<<"outputSerial:"<<outputSerial<<endl;
                            cout<<"(g:"<<g<<",h:"<<h<<",w:"<<w<<"):"<<tempTrue<<endl;
                            cout<<"CheckXW fail!!!"<<endl;
                                return false;
                        }
                    outputSerial++;
                }
        }
    #endif //REFORMED
    return true;
}

bool Systolic::AlignWX(
        const vector<SparseDataInFIFO<WTransIn> >& WTran, ///[SYS_GROUP][SYS_WIDTH]
        uint32_t& lastWIdx,int& tempWIdx,
        const vector<SparseDataInFIFO<XTransIn::FeatureType> >& XTran, ///[    1    ][SYS_HEIGHT]
        uint32_t& lastXIdx,int& tempXIdx) const{
    while(WTran[lastWIdx].GetLoc()!=XTran[lastXIdx].GetLoc()){
        assert( lastWIdx < WTran.size() && lastXIdx < XTran.size());
        if (WTran[lastWIdx].GetLoc() < XTran[lastXIdx].GetLoc()){
            if (WTran[lastWIdx].IsEOG()){
                if (XTran[lastXIdx].IsEOG()){
                    if (WTran[lastWIdx].GetValue().IfEOK()){
                        lastXIdx++;
                        lastWIdx++;
                        return true;
                    }
                    /// start the next Group
                    lastXIdx++;
                    lastWIdx++;
                }
                else
                    lastXIdx++;
            }
            else
                lastWIdx++;
        }
        else{
            /// WTran[lastWIdx].GetLoc() > XTran[lastXIdx].GetLoc()
            if (XTran[lastXIdx].IsEOG()){
                if (WTran[lastWIdx].IsEOG()){
                    if (WTran[lastWIdx].GetValue().IfEOK()){
                        lastXIdx++;
                        lastWIdx++;
                        return true;
                    }
                    lastXIdx++;
                    lastWIdx++;
                }
                else
                    lastWIdx++;
            }
            else
                lastXIdx++;
        }
    }
    assert(WTran[lastWIdx].GetLoc() == XTran[lastXIdx].GetLoc());
    tempWIdx = lastWIdx;
    tempXIdx = lastXIdx;
    if (WTran[lastWIdx].GetValue().IfEOK()){
        while(!XTran[lastXIdx].IsEOG()) lastXIdx++;
        while(!WTran[lastWIdx].IsEOG()) lastWIdx++;
        lastXIdx++;
        lastWIdx++;
        return false;
    }
    if (WTran[lastWIdx].IsEOG() && XTran[lastXIdx].IsEOG()){
        lastWIdx++;
        lastXIdx++;
        return false;
    }
    if (!WTran[lastWIdx].IsEOG())
        lastWIdx++;
    if (!XTran[lastXIdx].IsEOG())
        lastXIdx++;
    return false;
}


int Systolic::GetUpDataReuse(const Layer& thisLayer) const{
    int workLoad = this->WIn[0][0].GetWorkLoad();
    int kerneNum = thisLayer.getKN();
    vector<int> reuseTime(kerneNum,0);
    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++){
            int nowCalc=0;
            for (int i=0;i<workLoad;i++)
                if (this->WIn[g][w].GetIdx(i)==IDLE)
                    nowCalc++;
            for (int i=0;i<workLoad;i++){
                int nowIdx = 0;
                assert(nowCalc<=workLoad);
                if (nowCalc==workLoad)
                    break;
                if (reuseTime[this->WIn[g][w].GetIdx(i)]==0){
                    nowIdx = this->WIn[g][w].GetIdx(i);
                    for (int j=i;j<workLoad;j++)
                        if (this->WIn[g][w].GetIdx(j) == nowIdx)
                            reuseTime[nowIdx]++;
                    nowCalc += reuseTime[nowIdx];
                    continue;
                }
                assert(false);
            }
        }
    for (int i=0;i<kerneNum;i++){
        assert(reuseTime[i]!=0 && reuseTime[i]==reuseTime[0]);
    }
    return reuseTime[0];
}

void Systolic::GetDownDataSize(const Layer& thisLayer,int& dataSize,int& reluSize) const{
    assert(this->oHasGen);
    dataSize=0;
    reluSize=0;
    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++)
            for (uint32_t i=0;i<this->XOut[g][w].GetWorkLoad();i++){
                if (!this->XOut[g][w].IfIdle(i))
                    dataSize += WEIGHT_BIT_WIDTH;
                if (!this->XOut[g][w].IfIdle(i)
                &&  thisLayer.getFeature(
                        this->XOut[g][w].GetX(i),
                        this->XOut[g][w].GetY(i),
                        this->XOut[g][w].GetK(i))>0)
                    reluSize += WEIGHT_BIT_WIDTH+RELATIVE_LOC_BIT_WIDTH;
            }
    return;
}

int Systolic::GetUpCacheSize() const{
    assert(this->hasAnal);
    int workLoad = this->cacheData[0][0].size();
    int maxCacheData = 0;
    for (int i=0;i<workLoad;i++){
        int nowSize = 0;
        for (int g=0;g<SYS_GROUP;g++)
            for (int w=0;w<SYS_WIDTH;w++)
                nowSize += this->cacheData[g][w][i];
        maxCacheData =
        maxCacheData>nowSize?
        maxCacheData:nowSize;
    }
    return maxCacheData;
}

int Systolic::GetUpDataSize() const{
    assert(this->hasAnal);
    return this->upDataSize;
}

int Systolic::GetLeftDataSize() const{
    int leftDataSize = 0;
    #ifndef REFORMED
    for (int g=0;g<SYS_GROUP;g++)
    #else
    constexpr int g = 0;
    #endif // REFORMED
        for (int h=0;h<SYS_HEIGHT;h++)
            leftDataSize+=this->XTran[g][h].size()*WEIGHT_BIT_WIDTH;
    return leftDataSize;
}

void Systolic::AnalyzeInputData(const Layer& thisLayer){
    assert(this->wHasGen
       && !this->hasAnal);
    this->upDataSize = 0;
    int workLoad = this->WIn[0][0].GetWorkLoad();
    int kerneNum = thisLayer.getKN();
    for (int g=0;g<SYS_GROUP;g++)
        for (int w=0;w<SYS_WIDTH;w++){
            vector<int> mm(workLoad,0);
            /** memory management, the amount of data will be
             ** used in the beginning of the workload
             ** count in bit**/
            vector<bool> lastUse(kerneNum,false),
                         firstLoad(kerneNum,false);
            this->cacheData[g][w].clear();
            this->cacheData[g][w].resize(workLoad);
            for (int i=workLoad-1;i>=0;i--){
                if (this->WIn[g][w].GetIdx(i)==IDLE)
                    continue;
                if (!lastUse[this->WIn[g][w].GetIdx(i)]){
                     lastUse[this->WIn[g][w].GetIdx(i)]=true;
                     if (i<workLoad-1)
                        mm[i+1]-=thisLayer.getKernelWorkLoad(this->WIn[g][w].GetIdx(i));
                }
            }
            for (int i=0;i<workLoad;i++){
                if (this->WIn[g][w].GetIdx(i)==IDLE)
                    continue;
                if (!firstLoad[this->WIn[g][w].GetIdx(i)]){
                     firstLoad[this->WIn[g][w].GetIdx(i)]=true;
                     this->upDataSize+=thisLayer.getKernelWorkLoad(this->WIn[g][w].GetIdx(i));
                     mm[i]+=thisLayer.getKernelWorkLoad(this->WIn[g][w].GetIdx(i));
                }
            }

            this->cacheData[g][w][0]
                 = mm[0] * WEIGHT_BIT_WIDTH;
            for (int i=1;i<workLoad;i++)
                this->cacheData[g][w][i]
              = this->cacheData[g][w][i-1]
               +mm[i] * WEIGHT_BIT_WIDTH;
        }
    this->upDataSize *= WEIGHT_BIT_WIDTH;
    this->hasAnal = true;
    return;
}

int Systolic::GetInputCyc(const Layer& thisLayer) const{
    assert(this->wHasGen);
    const int workLoad = this->WIn[0][0].GetWorkLoad();
    int maxCyc = 0;
    for (const auto& git : this->WIn)
        for (const auto& wit : git){
            int tempCyc=0;
            for (int i=0;i<workLoad;i++)
                tempCyc += thisLayer.getKernelWorkLoad(wit.GetIdx(i));
            maxCyc=maxCyc>tempCyc?maxCyc:tempCyc;
        }
    return maxCyc;
}


#ifdef PRINT_INTERMEDIA_INFO
void Systolic::PrintXOut(){
    #ifdef PE_IDLE_NOT_OUTPUT
    assert(false);
    #else
    assert(this->oHasGen);
    int workLoad =
        SYS_HEIGHT*this->pe[0][0][0].GetWorkLoad();
    #endif // PE_IDLE_NOT_OUTPUT
    for (int i=0;i<workLoad;i++){
        for (int g=0;g<SYS_GROUP;g++)
            for (int w=0;w<SYS_WIDTH;w++){
                #ifdef PRINT_TO_STD
                cout.width(4);
                cout<<"("
                    <<this->XOut[g][w].GetX(i)<<",";
                cout.width(4);
                cout<<this->XOut[g][w].GetY(i)<<",";
                cout.width(4);
                cout<<this->XOut[g][w].GetK(i)<<")";
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                fprintf(this->XOutfile,"(%3d,%3d,%3d)",
                        this->XOut[g][w].GetX(i),
                        this->XOut[g][w].GetY(i),
                        this->XOut[g][w].GetK(i));
                #endif // PRINT_TO_FILE
            }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->XOutfile,"\n");
        #endif // PRINT_TO_FILE
    }
    #ifdef PRINT_TO_STD
    cout<<endl;
    #endif // PRINT_TO_STD
    #ifdef PRINT_TO_FILE
    fprintf(this->XOutfile,"\n");
    #endif // PRINT_TO_FILE
    return;
}
void Systolic::PrintWIn(){
    int workLoad = this->pe[0][0][0].GetWorkLoad();
    for (int i=0;i<workLoad;i++){
        for (int g=0;g<SYS_GROUP;g++)
            for (int w=0;w<SYS_WIDTH;w++){
                #ifdef PRINT_TO_STD
                cout.width(3);
                cout<<this->WIn[g][w].GetIdx(i);
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                fprintf(this->WInfile,"%3d",
                        this->WIn[g][w].GetIdx(i));
                #endif // PRINT_TO_FILE
            }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->WInfile,"\n");
        #endif // PRINT_TO_FILE
    }
    return;
}
void Systolic::PrintXIn(){
    #ifdef PRINT_INTERMEDIA_INFO
    int workLoad = this->pe[0][0][0].GetWorkLoad();
    for (int i=0;i<workLoad;i++){
        for (int h=0;h<SYS_HEIGHT;h++){
            #ifndef REFORMED
            for (int g=0;g<SYS_GROUP;g++)
            #else
            constexpr int g = 0;
            #endif // REFORMED
            {
                #ifdef PRINT_TO_STD
                cout.width(3);
                cout<<"("<<this->XIn[g][h].GetX(i)<<",";
                cout.width(3);
                cout<<this->XIn[g][h].GetY(i)<<")";
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                fprintf(this->XInfile,"(%3d,%3d)",
                        this->XIn[g][h].GetX(i),
                        this->XIn[g][h].GetY(i));
                #endif // PRINT_TO_FILE
            }
            #ifdef PRINT_TO_STD
            cout<<endl;
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(this->XInfile,"\n");
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->XInfile,"\n");
        #endif // PRINT_TO_FILE
    }
    #endif // PRINT_INTERMEDIA_INFO
    return;
}
void Systolic::PrintPE(){
    int workLoad = this->pe[0][0][0].GetWorkLoad();
    for (int i=0;i<workLoad;i++){
        for (int h=0;h<SYS_HEIGHT;h++){
            for (int g=0;g<SYS_GROUP;g++)
                for (int w=0;w<SYS_WIDTH;w++){
                    #ifdef PRINT_TO_STD
                    this->pe[g][h][w].PrintAt(i);
                    #endif // PRINT_TO_STD
                    #ifdef PRINT_TO_FILE
                    this->pe[g][h][w].PrintAt(i,this->PEfile);
                    #endif // PRINT_TO_FILE
                }
            #ifdef PRINT_TO_STD
            cout<<endl;
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(this->PEfile,"\n");
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->PEfile,"\n");
        #endif // PRINT_TO_FILE
    }
    return;
}
void Systolic::PrintPEActWorkLoad(){
    #ifdef PRINT_INTERMEDIA_INFO
    #ifdef PRINT_TO_FILE
    FILE *fp = fopen(PE_ACT_WORK_LOAD,"w+");
    #endif // PRINT_TO_FILE
    for (int g=0;g<SYS_GROUP;g++){
        #ifdef PRINT_TO_STD
        cout<<"group:"<<g<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(fp,"group:%d\n",g);
        #endif // PRINT_TO_FILE
        for (int w=0;w<this->pe[g][0][0].GetWorkLoad();w++){
            #ifdef PRINT_TO_STD
            cout<<this->saWorkLoad[g].GetWWorkLoad(w)<<" ";
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(fp,"%6d ",this->saWorkLoad[g].GetWWorkLoad(w));
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(fp,"\n");
        #endif // PRINT_TO_FILE
    }
    #ifdef PRINT_TO_STD
    cout<<endl;
    #endif // PRINT_TO_STD
    #ifdef PRINT_TO_FILE
    fprintf(fp,"\n");
    #endif // PRINT_TO_FILE
    for (int g=0;g<SYS_GROUP;g++){
        for (int h=0;h<SYS_HEIGHT;h++){
            for (int w=0;w<SYS_WIDTH;w++){
                #ifdef PRINT_TO_STD
                cout<<this->pe[g][h][w].GetActWorkLoad()<<" ";
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                fprintf(fp,"%6d ",this->pe[g][h][w].GetActWorkLoad());
                #endif // PRINT_TO_FILE
            }
            #ifdef PRINT_TO_STD
            cout<<endl;
            #endif // PRINT_TO_STD
            #ifdef PRINT_TO_FILE
            fprintf(fp,"\n");
            #endif // PRINT_TO_FILE
        }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(fp,"\n");
        #endif // PRINT_TO_FILE
    }
    #ifdef PRINT_TO_FILE
    fclose(fp);
    #endif // PRINT_TO_FILE
    #endif // PRINT_INTERMEDIA_INFO
    return;
}

void Systolic::PrintWorkLoad(){
    for (int h=0;h<SYS_HEIGHT;h++){
        for (int g=0;g<SYS_GROUP;g++)
            for (int w=0;w<SYS_WIDTH;w++){
                #ifdef PRINT_TO_STD
                cout<<this->pe[g][h][w].GetWorkLoad()<<" ";
                #endif // PRINT_TO_STD
                #ifdef PRINT_TO_FILE
                fprintf(this->PEfile,"%d ",this->pe[g][h][w].GetWorkLoad());
                #endif // PRINT_TO_FILE
            }
        #ifdef PRINT_TO_STD
        cout<<endl;
        #endif // PRINT_TO_STD
        #ifdef PRINT_TO_FILE
        fprintf(this->PEfile,"\n");
        #endif // PRINT_TO_FILE
    }
    #ifdef PRINT_TO_STD
    cout<<endl;
    #endif // PRINT_TO_STD
    #ifdef PRINT_TO_FILE
    fprintf(this->PEfile,"\n");
    #endif // PRINT_TO_FILE
    return;
}
#endif // PRINT_INTERMEDIA_INFO
