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
        int beginPos[],
        int nowH){
    for (int g=0;g<SYS_GROUP;g++)
        if (!this->rus[g].CheckXOut(SAXData[g][nowH],beginPos[g])){
            std::cout<<g<<","<<nowH<<","<<beginPos[g]<<std::endl;
            return false;
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

    if (groupNum!=locInGroupNum)
        return false;
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
            else
                layer.getSparseFeatureGroup(this->XIn[h].GetFeature(i),this->XData[h]);
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
        for (const auto& it : this->LocIn[g])
            layer.getSparseKernelGroup(it,this->LocData[g]);
        this->LocData[g].shrink_to_fit();
    }
    assert(this->CheckLDataHomo(layer));
    this->hasTransLIn = true;
    return;
}

void RUArray::GenXIn(const Layer& lastLayer,const Layer& thisLayer,const vector<vector<DFIFO> >& SAXIn){
    assert(thisLayer.getPadType()==SAME_PAD
        &&(thisLayer.getKD()%GROUP_SIZE==0)
        && thisLayer.getKW()%2==1
        && thisLayer.getKH()%2==1
        &&!this->hasGenXIn);

    const int workLoad = SAXIn[0][0].GetWorkLoad();
    const int wHalf = (thisLayer.getKW()-1)/2,
              hHalf = (thisLayer.getKH()-1)/2;
    const int groupPerline   = thisLayer.getKD()/GROUP_SIZE;
    const int groupPerKernel = thisLayer.getKH()
                             * thisLayer.getKW()*groupPerline;

    /**out-of-date, used in the out-of-date scheduling strategy **/
    /// bool lastIsIdle = true;/// wenzhi @ 2018-7-12 to fix bug

    bool beginOfZone = true;

    for (auto& it : this->XIn)
        it.reserve(groupPerKernel*2);

    for (int i=0;i<workLoad;i++)
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

            bool endOfZone;/// to remark if it can get data from synchronized FIFO
            if (idleLine){
                for (int kh=0;kh<groupPerKernel;kh++)
                    this->XIn[h].AddCtrl(BZ);
                beginOfZone = true;
            }
            else{
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
                    assert(thisLayer.getPadType()==SAME_PAD);

                    for (int w = thisW*thisLayer.getSW()-wHalf;
                             w<= thisW*thisLayer.getSW()+wHalf;w++){
                        int lastW = /// the location of the  input activation
                            (w<0)?0:
                            (w<lastLayer.getLW())?
                             w:lastLayer.getLW()-1;
                        for (int g=0;g<groupPerline;g++){
                            for (int kh=0;kh<thisLayer.getSH();kh++){
                                if (kh>=thisLayer.getKH())
                                    break;
                                int lastH = /// the location of the  input activation
                                    ((thisH*thisLayer.getSH()-hHalf+kh)<0)?0:
                                    ((thisH*thisLayer.getSH()-hHalf+kh)<lastLayer.getLH())?
                                     (thisH*thisLayer.getSH()-hHalf+kh):lastLayer.getLH()-1;

                                if ((kh >= (thisLayer.getKH() - thisLayer.getSH()))
                                   || h == 0 || beginOfZone)
                                    this->XIn[h].AddCtrl(RAB);
                                else
                                    this->XIn[h].AddCtrl(RAWB);

                                this->XIn[h].AddFeature(
                                        groupPerline*lastH*lastLayer.getLW()+
                                        groupPerline*lastW+g
                                    );
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
                    assert(thisLayer.getPadType()==SAME_PAD);

                    for (int w = thisW*thisLayer.getSW()-wHalf;
                             w<= thisW*thisLayer.getSW()+wHalf;w++){
                        int lastW =
                            (w<0)?0:
                            (w<lastLayer.getLW())?
                             w:lastLayer.getLW()-1;

                        for (int g=0;g<groupPerline;g++)
                            for (int kH = 0;kH<thisLayer.getKH();kH++){
                                int lastH =
                                    ((thisH*thisLayer.getSH()-hHalf+kH)<0)?0:
                                    ((thisH*thisLayer.getSH()-hHalf+kH)<lastLayer.getLH())?
                                     (thisH*thisLayer.getSH()-hHalf+kH):lastLayer.getLH()-1;

                                if (kH < (thisLayer.getKH()-thisLayer.getSH())){
                                    if (h==0 || beginOfZone)
                                        this->XIn[h].AddCtrl(RAB);
                                    else
                                        this->XIn[h].AddCtrl(RAWB);
                                }
                                else
                                    this->XIn[h].AddCtrl(RAB);

                                this->XIn[h].AddFeature(
                                        groupPerline*lastH*lastLayer.getLW()+
                                        groupPerline*lastW+g
                                    );
                            }
                    }
                }
                beginOfZone = endOfZone;
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

    for (int i=0;i<this->groupNum;i++){
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

void RUArray::AnalyzeInputData(const Layer& thisLayer,const Layer& lastLayer){
    assert(this->hasGenLIn
        && this->hasGenXIn
        &&!this->hasAnal);
    this->globDataSize  = 0;
    this->inRowDataSize = 0;
    this->loadDataSize  = 0;
    int maxInputCycle = 0;
    for (const auto& it : this->XIn){
        int thisRowCycle = 0;
        for (uint32_t i=0;i<it.GetWorkLoad();i++){
            if (it.IfCtrl(i)){
                thisRowCycle++;
                this->loadDataSize++;
            }
            else{
                thisRowCycle+=lastLayer.getSparseFeatureGroupSize(it.GetFeature(i));
                this->loadDataSize+=lastLayer.getSparseFeatureGroupSize(it.GetFeature(i));
            }
        }
        if (thisRowCycle > maxInputCycle)
            maxInputCycle = thisRowCycle;
    }
    maxInputCycle *= SFIFO_Data::BitWidth;
    for (const auto& wit : this->LocIn){
        int thisColumCycle = 0;
        for (const auto& kit : wit)
            thisColumCycle += thisLayer.getKernelWorkLoad(kit);
        if (thisColumCycle * WEIGHT_BIT_WIDTH > maxInputCycle)
            maxInputCycle = thisColumCycle * WEIGHT_BIT_WIDTH;
    }
    this->inputCycle    = maxInputCycle;
    this->loadDataSize *= SFIFO_Data::BitWidth;

    vector<DataInSFIFO<int> > merged;
    uint32_t totalGroupNum = 0;
    for (auto& it : this->XIn){
        it.Sort();
        totalGroupNum += it.GetGroupNum();
    }
    merged.reserve(totalGroupNum);

    uint32_t inRowDataUsed = 0;
       this->inRowDataSize = 0;
    for (auto& it : this->XIn){
        int nowGroup = it.GetFeature(0);
        uint32_t nowUsed  = 0;
        for (uint32_t i=0;i<it.GetGroupNum();i++){
            assert(!it.IfCtrl(i));
            if (nowGroup!= it.GetFeature(i)){
                      inRowDataUsed += lastLayer.getSparseFeatureGroupSize(nowGroup) * nowUsed;
                this->inRowDataSize += lastLayer.getSparseFeatureGroupSize(nowGroup);
                nowGroup = it.GetFeature(i);
                nowUsed  = 1;
                continue;
            }
            nowUsed++;
        }
              inRowDataUsed += lastLayer.getSparseFeatureGroupSize(nowGroup) * nowUsed;
        this->inRowDataSize += lastLayer.getSparseFeatureGroupSize(nowGroup);

        it.CopyGroupTo(merged);
        it.Clear();
    }
    this->inRowAveReuse = (1.0) * inRowDataUsed
                          / this->inRowDataSize;
    this->inRowDataSize*= SFIFO_Data::BitWidth;

    std::sort(merged.begin(),
              merged.end(),
           [](const DataInSFIFO<int32_t>& a,
              const DataInSFIFO<int32_t>& b)
           -> bool {return a.GetFeature()>b.GetFeature();});

    uint32_t globDataUsed = 0;
       this->globDataSize = 0;
    int nowGroup = merged.front().GetFeature();
    uint32_t nowUsed  = 0;
    for (const auto& it : merged){
        assert(!it.IsCtrl());
        if (nowGroup!= it.GetFeature()){
                  globDataUsed += lastLayer.getSparseFeatureGroupSize(nowGroup) * nowUsed;
            this->globDataSize += lastLayer.getSparseFeatureGroupSize(nowGroup);
            nowGroup = it.GetFeature();
            nowUsed  = 1;
            continue;
        }
        nowUsed++;
    }
          globDataUsed += lastLayer.getSparseFeatureGroupSize(nowGroup) * nowUsed;
    this->globDataSize += lastLayer.getSparseFeatureGroupSize(nowGroup);

    this->globAveReuse = (1.0) * globDataUsed
                         / this->globDataSize;
    this->globDataSize*= SFIFO_Data::BitWidth;

    this->upDataSize = 0;
    const int workLoad = this->LocIn[0].size();
    const int kerneNum = thisLayer.getKN();
    for (int g=0;g<SYS_GROUP;g++){
        vector<int> mm(workLoad,0);
        /** memory management, the amount of data will be
         ** used in the beginning of the workload
         ** count in bit**/
        vector<bool> lastUse(kerneNum,false),
                   firstLoad(kerneNum,false);
        this->cacheData[g].clear();
        this->cacheData[g].resize(workLoad);
        for (int i=workLoad-1;i>=0;i--){
            if (this->LocIn[g][i]==IDLE)
                continue;
            if (!lastUse[this->LocIn[g][i]]){
                 lastUse[this->LocIn[g][i]]=true;
                 if (i<workLoad-1)
                    mm[i+1]-=thisLayer.getKernelWorkLoad(this->LocIn[g][i]);
            }
        }
        for (int i=0;i<workLoad;i++){
            if (this->LocIn[g][i]==IDLE)
                continue;
            if (!firstLoad[this->LocIn[g][i]]){
                 firstLoad[this->LocIn[g][i]]=true;
                 this->upDataSize+=thisLayer.getKernelWorkLoad(this->LocIn[g][i]);
                 mm[i]+=thisLayer.getKernelWorkLoad(this->LocIn[g][i]);
            }
        }
        #ifdef SPARSE_IN_GROUP_ADDRESS
        this->cacheData[g][0] =   mm[0] * LOC_BIT_WIDTH;
        for (int i=1;i<workLoad;i++)
            this->cacheData[g][i]
         =  this->cacheData[g][i-1]+mm[i] * LOC_BIT_WIDTH;
        #endif // SPARSE_IN_GROUP_ADDRESS
        #ifdef SPARSE_RELATIVE_ADDRESS
        this->cacheData[g][0] =   mm[0] * RELATIVE_LOC_BIT_WIDTH;
        for (int i=1;i<workLoad;i++)
            this->cacheData[g][i]
         =  this->cacheData[g][i-1]+mm[i] * RELATIVE_LOC_BIT_WIDTH;
        #endif // SPARSE_RELATIVE_ADDRESS
    }
    #ifdef SPARSE_IN_GROUP_ADDRESS
    this->upDataSize *= LOC_BIT_WIDTH;
    #endif // SPARSE_IN_GROUP_ADDRESS
    #ifdef SPARSE_RELATIVE_ADDRESS
    this->upDataSize *= RELATIVE_LOC_BIT_WIDTH;
    #endif // SPARSE_RELATIVE_ADDRESS

    vector<int> reuseTime(kerneNum/KERNEL_GROUP_SIZE,0);
    for (int g=0;g<SYS_GROUP;g++){
        int nowCalc=0;
        for (int i=0;i<workLoad;i++)
            if (this->LocIn[g][i]==IDLE)
                nowCalc++;
        for (int i=0;i<workLoad;i++){
            int nowIdx;
            assert(nowCalc<=workLoad);
            if (nowCalc==workLoad)
                break;
            if (reuseTime[this->LocIn[g][i]]==0){
                nowIdx = this->LocIn[g][i];
                for (int j=i;j<workLoad;j++)
                    if (this->LocIn[g][j] == nowIdx)
                        reuseTime[nowIdx]++;
                nowCalc += reuseTime[nowIdx];
                continue;
            }
            assert(false);
        }
    }
    for (int i=0;i<kerneNum/KERNEL_GROUP_SIZE;i++)
        assert(reuseTime[i]!=0 && reuseTime[i]==reuseTime[0]);
    this->upDataReuseTime = reuseTime[0];
    this->hasAnal = true;
    return;
}
