#include "../include/RUArraySim.hpp"

void RU::GenXout(const vector<SparseDataInFIFO<XTransIn::FeatureType> >& xFIFO,
                 const vector<SparseLocInFIFO>& wLoc){
    uint32_t i=0,j=0,nowSize = this->xOut.size();

    while (i<wLoc.size() && j<xFIFO.size()){
        assert((i==0 || wLoc[i-1].GetLoc()< wLoc[i].GetLoc())
             &&(j==0 ||xFIFO[j-1].GetLoc()<xFIFO[j].GetLoc()));
        if (wLoc[i].GetLoc()==xFIFO[j].GetLoc()){
            this->xOut.push_back(xFIFO[j].GetValue());
            i++; j++;
        }
        else if (wLoc[i].GetLoc()<xFIFO[j].GetLoc()){
            this->xOut.push_back(0);
            i++;
        }
        else
            j++;
    }
    while (i<wLoc.size()){
        this->xOut.push_back(0);
        i++;
    }
    assert(this->xOut.size()-nowSize == wLoc.size());
    return;
}

bool RU::CheckXOut(const vector<XTransIn>& SAXData,int beginPos, uint32_t len){
    /** group checking not implemented yet **/
    assert(false);
    if (this->xOut.size()!=len)
        return false;
    for (uint32_t i=0;i<len;i++)
        if (SAXData[beginPos+i].GetValue()!=this->xOut[i])
            return false;
    return true;
}

bool RU::CheckXOut(const vector<XTransIn>& SAXData,int& beginPos){
    for (uint32_t i=beginPos;i<this->xOut.size();i++)
        if (!SAXData[i].IfAny()
           &&SAXData[i].GetValue()!=this->xOut[i]){
            return false;
        }
    beginPos = this->xOut.size();
    return true;
}

bool RU::CheckXOut(const vector<XTransIn>& SAXData){
    assert(false);
    if (this->xOut.size()!=SAXData.size())
        return false;
    for (uint32_t i=0;i<this->xOut.size();i++)
        if (SAXData[i].GetValue()!=this->xOut[i])
            return false;
    return true;
}

bool CE::CheckXOut(
        const vector<vector<vector<XTransIn> > >& SAXData,
        int beginPos[],int nowH){
    for (int g=0;g<SYS_GROUP;g++)
        if (!this->rus[g].CheckXOut(SAXData[g][nowH],beginPos[g])){
            std::cout<<g<<","<<nowH<<","<<beginPos[g]<<std::endl;
            return false;
        }
    return true;
}

bool CE::CheckXOut(
        const vector<SparseDataInFIFO<XTransIn::FeatureType> >& SAXData,
        int& beginPos){
    for (const auto& xit : this->xFIFO){
        if (!xit.equal(SAXData[beginPos])){
            std::cout<<"CE data mismatch:"<<std::endl
                     <<"\tgenerated:"<<std::endl;
            xit.Print();
            std::cout<<"\trequired:"<<std::endl;
            SAXData[beginPos].Print();
            return false;
        }
        beginPos++;
    }
    return true;
}

void CE::PeriodPush(const SFIFO_Data& XData){
    assert(XData.IfCtrl(this->currentPos));
    this->xFIFO.clear();
    enum FIFO_CTRL ctrl = XData.GetCtrl(this->currentPos++);

    assert(ctrl == RAB   || ctrl == RAWB
        || ctrl == RAWBZ || ctrl == RSB
        || ctrl == RSBZ  || ctrl == RSWB
        || ctrl == RSWBZ || ctrl == BZ);

    bool readFromAsynFIFO,
    /**  readFromSyncFIFO, **/
         writeToSyncFIFO,
         broadcastZero;
    /** readFromAsynFIFO and readFromSyncFIFO **
     ** cannot be true at the same time       **/

    if (ctrl == BZ){
        this->SendRUPlaceHolder();
        return;
    }
    if (ctrl == RAW || ctrl == RAWB
     || ctrl == RAB || ctrl == RAWBZ){
         readFromAsynFIFO = true;
     }
     else{
         assert(!this->isBot);
         readFromAsynFIFO = false;
    }

    if (ctrl == RAWB || ctrl == RAWBZ
     || ctrl == RSWB || ctrl == RSWBZ){
         assert(!this->isTop);
         writeToSyncFIFO = true;
    }
    else
         writeToSyncFIFO = false;

    if (ctrl == RSBZ
     || ctrl == RAWBZ
     || ctrl == RSWBZ){
        this->SendRUPlaceHolder();
        broadcastZero = true;
    }
    else
        broadcastZero = false;

    if (readFromAsynFIFO){
        if (XData.IfCtrl(this->currentPos)){
            /** which means the data group is empty **/
            assert(false);
            /** wenzhi @2018-10-12:
             **      A zero will be remained in a empty
             **      group thus this cannot happen.
             **/
            if (!broadcastZero)  this->SendRUPlaceHolder();
            if (writeToSyncFIFO) this->SendSFPlaceHolder();
            return;
        }
        while (true){
            /** since the group in SFIFO is split by ctrl
             ** there only need to read until the next CTRL
             **
             ** wenzhi @2018-10-12:
             **      Since the FIFO between CEs can contain
             **      several group of data at a same time,
             **      it is no longer sufficient to simply
             **      split different group by Ctrl code.
             **      The end-of-group is marked with an
             **      introduced mark-bit (EOG).
             **/
            assert(!XData.IfCtrl(this->currentPos));
            if (!broadcastZero ) this->SendRU(XData.GetFeature(this->currentPos));
            if (writeToSyncFIFO) this->SendSF(XData.GetFeature(this->currentPos));
            if (XData.IsEOG(this->currentPos++))
                break;
        }
    }
    else{
        /** read from sync FIFO while the placeholder **
         ** has been already inserted in sync FIFO    **/
        assert(this->lowerFIFOpt->size()>0);
        while (!(this->lowerFIFOpt->front().IsEOG())){
            if (!broadcastZero)  this->SendRU(this->lowerFIFOpt->front());
            if (writeToSyncFIFO) this->SendSF(this->lowerFIFOpt->front());
            this->lowerFIFOpt->pop();
        }
        assert(this->lowerFIFOpt->front().IsEOG());
        if (!broadcastZero)  this->SendRU(this->lowerFIFOpt->front());
        if (writeToSyncFIFO) this->SendSF(this->lowerFIFOpt->front());
        this->lowerFIFOpt->pop();
    }
    return;
}

bool RUArray::CheckXInHomo(){
    const uint32_t groupNum=this->XIn[0].GetCtrlNum();
    for (int h=1;h<SYS_HEIGHT;h++)
        if (this->XIn[h].GetCtrlNum()!=groupNum)
            return false;
    return true;
}
bool RUArray::CheckLInHomo(){
    const uint32_t kernelNum = this->LocIn[0].size();
    for (int g=1;g<SYS_GROUP;g++)
        if (this->LocIn[g].size()!=kernelNum)
            return false;
    return true;
}
bool RUArray::CheckXLInHomo(Layer& thisLayer){
    assert(this->hasGenLIn
        && this->hasGenXIn);
    uint32_t locGroupNum = 0;
    for (const auto& it : this->LocIn[0])
        locGroupNum += thisLayer.getGroupNumInKernel(it);
    return this->XIn[0].GetCtrlNum()
        == locGroupNum;
}

bool RUArray::CheckXDataHomo(){
    const uint32_t groupNum=this->XIn[0].GetCtrlNum();
    for (const auto& it : this->XData)
        if (it.GetCtrlNum()!=groupNum)
            return false;
    return true;
}
bool RUArray::CheckLDataHomo(const Layer& layer){
    int     groupNum = 0,
       locInGroupNum = 0;
    for (uint32_t i=0;i<this->LocData[0].size();i++)
        if (this->LocData[0][i].IsEOG())
            groupNum++;
    for (uint32_t k=0;k<this->LocIn[0].size();k++)
        locInGroupNum += layer.getGroupNumInKernel(this->LocIn[0][k]);

    if (groupNum!=locInGroupNum){
        std::cout<<"LocIn group number:"<<locInGroupNum
              <<" LocData group number:"<<groupNum<<std::endl;
        return false;
    }
    for (int g=1;g<SYS_GROUP;g++){
        int tempGroupNum = 0;
        for (uint32_t i=0;i<this->LocData[g].size();i++)
            if (this->LocData[g][i].IsEOG())
                tempGroupNum++;
        if (tempGroupNum != groupNum)
            return false;
    }
    return true;
}
bool RUArray::CheckXLDataHomo(){
    assert(this->hasTransLIn
        && this->hasTransXIn);
    #ifdef REMOVE_ALL_ZERO_GROUP
    assert(false);
    for (int g=0;g<SYS_GROUP;g++){
        int groupNum = 0;
        for (int i=0;i<this->LocData[g].size();i++)
            if (this->LocData[g][i].IsEOG())
                groupNum++;
        for (int i=0;i<this->LocIn[g].size();i++)
            groupNum -= layer.getGroupNumInKernel();
        if (groupNum!=0)
            return false;
    }
    #else
    int lInGroupNum = 0,
        wInGroupNum = 0;
    for (const auto& it : this->LocData[0])
        if (it.IsEOG())
            lInGroupNum++;
    wInGroupNum = this->XData[0].GetCtrlNum();
    if (lInGroupNum != wInGroupNum){
        std::cout<<"#"<<lInGroupNum<<"#"<<std::endl;
        for (const auto& it : this->XData)
            std::cout<<"#"<<it.GetCtrlNum()
                     <<","<<it.GetWorkLoad()
                     <<"#"<<std::endl;
        return false;
    }
    #endif // REMOVE_ALL_ZERO_GROUP
    return true;
}

void RUArray::TransXIn(const Layer& layer){
    assert(this->hasGenXIn
        &&!this->hasTransXIn);

    for (int h=0;h<SYS_HEIGHT;h++){
        const int XInSize = this->XIn[h].GetWorkLoad();
        this->XData[h].reserve(XInSize * GROUP_SIZE);
        for (int i=0;i<XInSize;i++)
            if (this->XIn[h].IfCtrl(i))
                this->XData[h].AddCtrl(this->XIn[h].GetCtrl(i));
            else{
                if (!this->XIn[h].IsZeroGroup(i))
                    layer.getSparseFeatureGroup(this->XIn[h].GetFeature(i),this->XData[h]);
                else
                    this->XData[h].AddZeroFeatureGroup();
            }
        this->XData[h].shrink_to_fit();
    }
    assert(this->CheckXDataHomo());
    this->hasTransXIn = true;
}

void RUArray::TransLIn(const Layer& layer){
    assert(this->hasGenLIn
        &&!this->hasTransLIn);

    for (int g=0;g<SYS_GROUP;g++){
        this->LocData[g].clear();
        this->LocData[g].reserve(layer.getKernelSize());
        for (const auto& it : this->LocIn[g]){
            layer.getSparseKernelGroup(it,this->LocData[g]);
            layer.FillKernel(this->LocData[g]);
        }
        this->LocData[g].shrink_to_fit();
    }
    assert(this->CheckLDataHomo(layer));
    this->hasTransLIn = true;
    return;
}

void RUArray::GenXIn(const Layer& lastLayer,const Layer& thisLayer,const vector<vector<DFIFO> >& SAXIn){
    assert((thisLayer.getKD()%GROUP_SIZE==0)
         && thisLayer.getKW()%2==1
         && thisLayer.getKH()%2==1
         &&!this->hasGenXIn);

    const int workLoad = SAXIn[0][0].GetWorkLoad();
    const int groupPerline   = thisLayer.getKD()/GROUP_SIZE;
    const int groupPerKernel = thisLayer.getKH()
                             * thisLayer.getKW()*groupPerline;

    /**out-of-date, used in the out-of-date scheduling strategy **/
    /// bool lastIsIdle = true;/// wenzhi @ 2018-7-12 to fix bug

    bool beginOfZone = true;

    for (auto& it : this->XIn)
        it.reserve(groupPerKernel*2);

    for (int i=0;i<workLoad;i++){
        for (int h=0;h<SYS_HEIGHT;h++){
            int thisH,thisW;/// the location of the output activation
            bool idleLine = true;

            #ifndef REFORMED
            for (int g=0;g<SYS_GROUP;g++)
            #else
            constexpr int g = 0;
            #endif // REFORMED
            {
                if (!SAXIn[g][h].IfIdle(i)){
                    idleLine = false;
                    thisH = SAXIn[g][h].GetY(i);
                    thisW = SAXIn[g][h].GetX(i);
                    #ifndef REFORMED
                    break;
                    #endif // REFORMED
                }
            }

            #ifndef STRAIGHT_OUT
            bool endOfZone;/// to remark if it can get data from synchronized FIFO
            #endif // STRAIGHT_OUT
            if (idleLine){
                #ifndef RELAX_MAPPING_H
                for (int kh=0;kh<groupPerKernel;kh++)
                    this->XIn[h].AddCtrl(BZ);
                beginOfZone = true;
                #else
                if (!beginOfZone){
                    for (int kh=0;kh<groupPerKernel;kh++)
                        this->XIn[h].AddCtrl(RSWBZ);
                }
                else{
                    for (int kh=0;kh<groupPerKernel;kh++)
                        this->XIn[h].AddCtrl(BZ);
                }
                #endif // RELAX_MAPPING_H
            }
            else{
                #ifdef STRAIGHT_OUT
                for (int w = thisW * thisLayer.getSW();
                         w < thisW * thisLayer.getSW()
                                   + thisLayer.getKW();w++){
                    int lastW =  w - thisLayer.getWPadOff();
                    /// the location of the  input activation
                    for (int g=0;g<groupPerline;g++)
                        for (int kh=0;kh<thisLayer.getKH();kh++){
                            int lastH = thisH*thisLayer.getSH()-thisLayer.getHPadOff()+kh;
                            /// the location of the  input activation

                            this->XIn[h].AddCtrl(RAB);

                            if (thisLayer.getPadType()==SAME_PAD){
                                lastW =
                                    (lastW<0)?0:
                                    (lastW<lastLayer.getLW())?
                                     lastW:lastLayer.getLW()-1;
                                lastH =
                                    (lastH<0)?0:
                                    (lastH<lastLayer.getLH())?
                                     lastH:lastLayer.getLH()-1;
                                this->XIn[h].AddFeature(
                                        groupPerline*lastH*lastLayer.getLW()+
                                        groupPerline*lastW+g);
                            }
                            else if (thisLayer.getPadType()==ZERO_PAD){
                                if (lastW<0 || lastW>=lastLayer.getLW()
                                 || lastH<0 || lastH>=lastLayer.getLH()){
                                    this->XIn[h].AddZeroGroup();
                                }
                                else{
                                    this->XIn[h].AddFeature(
                                        groupPerline*lastH*lastLayer.getLW()+
                                        groupPerline*lastW+g);
                                }
                            }
                            else{
                                assert(false);
                            }
                        }
                }
                #else
                endOfZone = true;
                if (h<SYS_HEIGHT-1){
                    #ifndef REFORMED
                    for (int g=0;g<SYS_GROUP;g++)
                    #else
                    constexpr int g = 0;
                    #endif // REFORMED
                    {
                        if (!SAXIn[g][h+1].IfIdle(i)){
                            if ((thisH ==(SAXIn[g][h+1].GetY(i)-1))
                             && (thisW == SAXIn[g][h+1].GetX(i)   ))
                                endOfZone = false;
                            #ifndef REFORMED
                            break;
                            #endif // REFORMED
                        }
                    }
                }
                if (!endOfZone){
                    for (int w = thisW * thisLayer.getSW();
                             w < thisW * thisLayer.getSW()
                                       + thisLayer.getKW();w++){
                        int lastW =  w - thisLayer.getWPadOff();
                        /// the location of the  input activation
                        for (int g=0;g<groupPerline;g++){
                            for (int kh=0;kh<thisLayer.getSH();kh++){
                                if (kh>=thisLayer.getKH())
                                    break;
                                int lastH = thisH*thisLayer.getSH()-thisLayer.getHPadOff()+kh;
                                /// the location of the  input activation

                                if ((kh >= (thisLayer.getKH() - thisLayer.getSH()))
                                   || h == 0 || beginOfZone)
                                    this->XIn[h].AddCtrl(RAB);
                                else
                                    this->XIn[h].AddCtrl(RAWB);

                                if (thisLayer.getPadType()==SAME_PAD){
                                    lastW =
                                        (lastW<0)?0:
                                        (lastW<lastLayer.getLW())?
                                         lastW:lastLayer.getLW()-1;
                                    lastH =
                                        (lastH<0)?0:
                                        (lastH<lastLayer.getLH())?
                                         lastH:lastLayer.getLH()-1;
                                    this->XIn[h].AddFeature(
                                            groupPerline*lastH*lastLayer.getLW()+
                                            groupPerline*lastW+g);
                                }
                                else if (thisLayer.getPadType()==ZERO_PAD){
                                    if (lastW<0 || lastW>=lastLayer.getLW()
                                     || lastH<0 || lastH>=lastLayer.getLH()){
                                        this->XIn[h].AddZeroGroup();
                                    }
                                    else{
                                        this->XIn[h].AddFeature(
                                            groupPerline*lastH*lastLayer.getLW()+
                                            groupPerline*lastW+g);
                                    }
                                }
                                else{
                                    assert(false);
                                }
                            }
                            for (int kh=thisLayer.getSH();kh<thisLayer.getKH();kh++)
                                if ((kh >= (thisLayer.getKH() - thisLayer.getSH()))
                                   || h==0 || beginOfZone)
                                    this->XIn[h].AddCtrl(RSB );
                                else
                                    this->XIn[h].AddCtrl(RSWB);
                        }
                    }
                }
                else{
                    for (int w = thisW*thisLayer.getSW();
                             w < thisW*thisLayer.getSW()
                                     + thisLayer.getKW();w++){
                        int lastW = w - thisLayer.getWPadOff();
                        /// the location of the  input activation

                        for (int g=0;g<groupPerline;g++)
                            for (int kh = 0;kh<thisLayer.getKH();kh++){
                                int lastH = thisH*thisLayer.getSH()-thisLayer.getHPadOff()+kh;
                                /// the location of the  input activation

                                if (kh < (thisLayer.getKH()-thisLayer.getSH())){
                                    if (h==0 || beginOfZone)
                                        this->XIn[h].AddCtrl(RAB);
                                    else
                                        this->XIn[h].AddCtrl(RAWB);
                                }
                                else
                                    this->XIn[h].AddCtrl(RAB);

                                if (thisLayer.getPadType()==SAME_PAD){
                                    lastW =
                                        (lastW<0)?0:
                                        (lastW<lastLayer.getLW())?
                                         lastW:lastLayer.getLW()-1;
                                    lastH =
                                        (lastH<0)?0:
                                        (lastH<lastLayer.getLH())?
                                         lastH:lastLayer.getLH()-1;
                                    this->XIn[h].AddFeature(
                                            groupPerline*lastH*lastLayer.getLW()+
                                            groupPerline*lastW+g
                                        );
                                }
                                else if (thisLayer.getPadType()==ZERO_PAD){
                                    if (lastW<0 || lastW>=lastLayer.getLW()
                                     || lastH<0 || lastH>=lastLayer.getLH()){
                                        this->XIn[h].AddZeroGroup();
                                    }
                                    else{
                                        this->XIn[h].AddFeature(
                                            groupPerline*lastH*lastLayer.getLW()+
                                            groupPerline*lastW+g);
                                    }
                                }
                                else{
                                    assert(false);
                                }
                            }
                    }
                }
                beginOfZone = endOfZone;
                #endif // STRAIGHT_OUTPUT
            }
            for (uint32_t k=0;k<thisLayer.GetExtraGroup();k++)
                this->XIn[h].AddCtrl(BZ);
        }
    }

    for (auto& it : this->XIn)
        it.shrink_to_fit();

    assert(this->CheckXInHomo());
    this->groupNum  = this->XIn[0].GetCtrlNum();
    this->hasGenXIn = true;
    return;
}

void RUArray::GenLIn(const vector<vector<DFIFO> >& WIn){
    assert(!this->hasGenLIn);
    for (uint32_t g=0;g<SYS_GROUP;g++){
        this->LocIn[g].clear();
        this->LocIn[g].reserve(WIn[g][0].GetWorkLoad());
        for (uint32_t i=0;i<WIn[g][0].GetWorkLoad();i++){
            if (WIn[g][0].IfIdle(i))
                this->LocIn[g].emplace_back(IDLE);
            else
                this->LocIn[g].emplace_back(WIn[g][0].GetIdx(i)/KERNEL_GROUP_SIZE);
        }
    }
    assert(this->CheckLInHomo());
    this->hasGenLIn = true;
    return;
}

#ifndef REFORMED
bool RUArray::CheckXL(const vector<vector<vector<XTransIn> > >& SAXData){
    assert(this->hasTransLIn
        && this->hasTransXIn
        && this->CheckXLDataHomo());

    uint32_t wlocIdx[SYS_GROUP];
    int  xinIdx[SYS_HEIGHT][SYS_GROUP];
    memset( xinIdx,0,sizeof( xinIdx));
    memset(wlocIdx,0,sizeof(wlocIdx));

    for (uint32_t i=0;i<this->groupNum;i++){
        /** prepare wloc input for RUArray **/
        for (int g=0;g<SYS_GROUP;g++){
            this->wloc[g].clear();
            while(true){
                assert(wlocIdx[g]<this->LocData[g].size());
                this->wloc[g].emplace_back(this->LocData[g][wlocIdx[g]]);
                if (this->LocData[g][wlocIdx[g]++].IsEOG())
                    break;
            }
            this->wloc[g].shrink_to_fit();
        }
        /** prepare XIn for RUArray **/
        for (int h=0;h<SYS_HEIGHT;h++)
            this->ce[h].PeriodPush(this->XData[h]);

        /** generate XOut using XIn & wloc **/
        for (int h=0;h<SYS_HEIGHT;h++)
            this->ce[h].GenXOut(this->wloc);

        /** check if it match the input
         ** of the systolic array       **/
        for (int h=0;h<SYS_HEIGHT;h++)
            if (!this->ce[h].CheckXOut(SAXData,xinIdx[h],h)){
                std::cout<<"## output error @"<<h<<" line ##"<<std::endl;
                return false;
            }
    }
    return true;
}
#else
bool  RUArray::CheckXL(const vector<vector<vector<SparseDataInFIFO<XTransIn::FeatureType> > > >& SAXData){
    vector<int> curPos(SYS_HEIGHT,0);
    for (uint32_t i=0;i<this->groupNum;i++){
        /** prepare XIn for RUArray **/
        for (int h=0;h<SYS_HEIGHT;h++){
            this->ce[h].PeriodPush(this->XData[h]);
        }
        for (int h=0;h<SYS_HEIGHT;h++){
            if (!this->ce[h].CheckXOut(SAXData[0][h],curPos[h])){
                std::cout<<"wenzhi height:"<<h<<" group idx:"<<i<<" curPos:"<<curPos[h]<<std::endl;
                return false;
            }
        }
    }
    return true;
}
#endif // REFORMED

#ifdef PRINT_INTERMEDIA_INFO
void RUArray::PrintLocAnalResult(){
    int workLoad = this->LInCacheData[0].size();
    int maxCacheData = 0;
    for (int i=0;i<workLoad;i++){
        int nowSize = 0;
        for (int g=0;g<SYS_GROUP;g++)
            nowSize += this->LInCacheData[g][i];
        fprintf(this->fpLInAnalResult,"%d ",nowSize);
        maxCacheData =
        maxCacheData>nowSize?
        maxCacheData:nowSize;
    }
    fprintf(this->fpLInAnalResult,"\n%d",maxCacheData);
    return;
}

void RUArray::PrintXIn(){
    for (int i=0;i<SYS_HEIGHT;i++){
        fprintf(this->fpXIn,"##%d##\n",i);
        this->XIn[i].PrintTo(this->fpXIn);
        fprintf(this->fpXIn,"##end##\n");
    }
}

void RUArray::PrintLIn(){
    for (int i=0;i<SYS_GROUP;i++){
        for (int j=0;j<this->LocIn[i].size();j++)
            fprintf(this->fpLIn,"%4d ",this->LocIn[i][j]);
        fprintf(this->fpLIn,"\n");
    }
}
#endif //PRINT_INTERMEDIA_INFO

void RUArray::PrintXData(const std::string& prefix){
    for (int h=0;h<SYS_HEIGHT;h++){
        std::ofstream HFofs((prefix + RU_XDATA_FILE_PATH_PREFIX
                           + std::to_string(h) + "_HF"
                           + TRANS_FILE_TYPE),
                             std::ofstream::out);
        std::ofstream MFofs((prefix + RU_XDATA_FILE_PATH_PREFIX
                           + std::to_string(h) + "_MF"
                           + TRANS_FILE_TYPE),
                             std::ofstream::out);

        if (!HFofs){
            std::cout<<"Open File Error [Human friendly Trans Feature]"<<std::endl;
            return;
        }
        if (!MFofs){
            std::cout<<"Open File Error [Machine friendly Trans Feature]"<<std::endl;
            return;
        }
        this->XData[h].PrintHFTo(HFofs);
        this->XData[h].PrintMFTo(MFofs);
        MFofs.close();
        HFofs.close();
    }
    return;
}
void RUArray::PrintLData(const std::string& prefix){
    for (int g=0;g<SYS_GROUP;g++){
        std::ofstream ofs((prefix + RU_LDATA_FILE_PATH_PREFIX
                         + std::to_string(g)
                         + TRANS_FILE_TYPE),
                           std::ofstream::out);

        if (!ofs){
            std::cout<<"Open File Error [Trans Location]"<<std::endl;
            return;
        }
        for (const auto& it : this->LocData[g])
            ofs<<it.GetLoc()<<" "<<it.IsEOG()<<std::endl;
        ofs.close();
    }
    return;
}

void RUArray::AnalyzeXIn(const Layer& lastLayer){
    assert(this->hasGenXIn
        &&!this->hasAnal);

    vector<DataInSFIFO<int> > merged;
    uint32_t totalGroupNum = 0;
    for (auto& it : this->XIn){
        it.Sort();
        totalGroupNum += it.GetGroupNum();
    }
    merged.reserve(totalGroupNum);

    this->inRowDenseDataSize  = 0;
    this->inRowSparseDataSize = 0;
    for (auto& it : this->XIn){
        if (it.GetGroupNum()==0)
            continue;
        int nowGroup = it.GetFeature(0);
        for (uint32_t i=0;i<it.GetGroupNum();i++){
            assert(!it.IfCtrl(i));
            if (nowGroup!= it.GetFeature(i)){
                this->inRowDenseDataSize++;
                this->inRowSparseDataSize
                 += lastLayer.getSparseFeatureGroupSize(nowGroup);
                nowGroup = it.GetFeature(i);
            }
        }
        this->inRowDenseDataSize++;
        this->inRowSparseDataSize
         += lastLayer.getSparseFeatureGroupSize(nowGroup);

        it.CopyGroupTo(merged);
        it.Clear();
    }
    this->inRowSparseDataSize*= SFIFO_Data::bitwidth;
    this->inRowDenseDataSize *= GROUP_SIZE * WEIGHT_BIT_WIDTH;

    std::sort(merged.begin(),merged.end(),
           [](const DataInSFIFO<int32_t>& a,
              const DataInSFIFO<int32_t>& b)
           -> bool {return a.GetFeature()>b.GetFeature();});

    this->globSparseDataSize = 0;
    this-> globDenseDataSize = 0;
    int nowGroup = merged.front().GetFeature();
    for (const auto& it : merged){
        assert(!it.IsCtrl());
        if (nowGroup != it.GetFeature()){
            this->globDenseDataSize++;
            this->globSparseDataSize
             += lastLayer.getSparseFeatureGroupSize(nowGroup);
            nowGroup = it.GetFeature();
            continue;
        }
    }
    this->globDenseDataSize++;
    this->globDenseDataSize*= GROUP_SIZE * WEIGHT_BIT_WIDTH;
    this->globSparseDataSize+=lastLayer.getSparseFeatureGroupSize(nowGroup);
    this->globSparseDataSize*=SFIFO_Data::bitwidth;
    this->hasAnal = true;
    return;
}
 
