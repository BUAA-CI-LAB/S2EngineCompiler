#ifndef RUA_SIM_HPP
#define RUA_SIM_HPP

#include "net.hpp"
#include "FIFO.hpp"
#include "Interface.hpp"

#include "hwinfo.hpp"
#include "compiler.hpp"

#include <cstring>
#include <queue>
#include <stack>
#include <array>
#include <vector>

class RU{
    /**
     ** actually is used in CheckXL process
     ** since the checking process is work
     ** in the granularity of group
     **
     ** thus, there use vector to store
     ** corresponding data instead of queue
     **/
private:
    vector<XTransIn::FeatureType> xOut;
    /** x value input of SA array **/

public:
    RU(){
        this->Reset();
        return;
    }

    inline void Reset(){
        this->xOut.clear();
        return;
    }

    void GenXout(const vector<SparseDataInFIFO<XTransIn::FeatureType> >& xFIFO,
                 const vector<SparseLocInFIFO>& wLoc);

    bool CheckXOut(const vector<XTransIn>& SAXData);
    bool CheckXOut(const vector<XTransIn>& SAXData,int& beginPos);
    bool CheckXOut(const vector<XTransIn>& SAXData,int  beginPos,uint32_t len);

    void PrintXIn_out(const string& name){
        FILE* fp = fopen(name.c_str(),"w+");
        for (uint32_t i=0;i<this->xOut.size();i++)
            fprintf(fp,"%s ",std::to_string(this->xOut[i]).c_str());
        fclose(fp);
        return;
    }
};

class CE{
private:

    RU rus[SYS_GROUP];

    queue<SparseDataInFIFO<XTransIn::FeatureType> >* upperFIFOpt;
    queue<SparseDataInFIFO<XTransIn::FeatureType> >* lowerFIFOpt;

    vector<SparseDataInFIFO<XTransIn::FeatureType> > xFIFO;
    /** to save memory and time of copying,
     ** RUS shares the same xFIFO **/

    int currentPos;
    /** the function is similar to the PC register **/

    bool isTop,isBot;
    /** to mark the position of the CE,
     ** which imply whether the CE has
     ** upper or lower FIFO **/

    inline void SendRUPlaceHolder(){
        /** "broadcast" place holder to RU's FIFO **/
        this->xFIFO.emplace_back  (SparseDataInFIFO<XTransIn::FeatureType> (0,FEATURE_FILL_ZERO_POSITION,true));
        return;
    }
    inline void SendSFPlaceHolder(){
        /** "broadcast" place holder to sync FIFO **/
        this->upperFIFOpt->emplace(SparseDataInFIFO<XTransIn::FeatureType> (0,FEATURE_FILL_ZERO_POSITION,true));
        return;
    }
    inline void SendRU(const SparseDataInFIFO<XTransIn::FeatureType> & feature){
        /** "broadcast" the feature to RU's FIFO **/
        this->xFIFO.emplace_back(feature);
        return;
    }
    inline void SendSF(const SparseDataInFIFO<XTransIn::FeatureType> & feature){
        /** "broadcast" the feature to sync FIFO **/
        this->upperFIFOpt->emplace(feature);
        return;
    }

public:
    CE(){
        this->currentPos = 0;
        return;
    }
    inline void SetNeighborFIFO(
        queue<SparseDataInFIFO<XTransIn::FeatureType> >* upperFIFOpt,
        queue<SparseDataInFIFO<XTransIn::FeatureType> >* lowerFIFOpt){
        this->upperFIFOpt = upperFIFOpt;
        this->lowerFIFOpt = lowerFIFOpt;
        if (upperFIFOpt!=NULL)
            this->isTop = false;
        else
            this->isTop = true;

        if (lowerFIFOpt!=NULL)
            this->isBot = false;
        else
            this->isBot = true;
        return;
    }
    void PeriodPush(const SFIFO_Data& XData);
    inline bool CheckXOut(
        const vector<vector<vector<XTransIn> > >& SAXData,
        int beginPos[],
        int nowH
    );

    inline bool CheckXOut(
        const vector<SparseDataInFIFO<XTransIn::FeatureType> >& SAXData,
        int& beginPos
    );

    inline void GenXOut(const vector<vector<SparseLocInFIFO> >& wLoc){
        for (int i=0;i<SYS_GROUP;i++)
            this->rus[i].GenXout(this->xFIFO,wLoc[i]);
        return;
    }
    inline void PrintXIn_out(int height){
        for (int g=0;g<SYS_GROUP;g++)
            this->rus[g].PrintXIn_out(RU_X4SA_FILE_PATH_PREFIX
                                 +std::to_string(g)+string("_")
                                 +std::to_string(height)+TRANS_FILE_TYPE);
        return;
    }
};


class RUArray: public SA_RUA_Interface{
private:

    #ifdef PRINT_INTERMEDIA_INFO
    FILE *fpXIn,
         *fpLIn;
    FILE *fpLInAnalResult;
    #endif // PRINT_INTERMEDIA_INFO

    vector<int> LInCacheData[SYS_GROUP];
    /** to analyze the cache usage of Loc In **/

    queue<SparseDataInFIFO<XTransIn::FeatureType> > fifo[SYS_HEIGHT-1];
    /** since there are n-1 fifos between n CE **/

    vector<SFIFO_In> XIn; /// [SYS_HEIGHT]
    /** the FIFO connect to CE,
     ** contents the data and control command
     ** the data is stored as group serial
     ** used to generate ASM
     **/

    vector<SFIFO_Data> XData; /// [SYS_HEIGHT]
    /** the FIFO connect to CE,
     ** contents the data and control command
     ** stored the actual data which is loaded to the FIFO
     **/

    vector<vector<int> > LocIn;///[SYS_GROUP]
    /** the FIFO connect to RU as shown in design document,
     ** contents the order of the kernel input
     **/

    vector<vector<SparseLocInFIFO> > LocData;///[SYS_GROUP]
    /** the FIFO connect to RU as shown in design document,
     ** contents the coordinate of the nonzero data in each group
     ** and whether is the last nonzero element in the group
     **/

    vector<CE> ce;///[SYS_HEIGHT]

    vector<vector<SparseLocInFIFO> > wloc;///[SYS_GROUP]

    bool hasGenXIn,
         hasGenLIn,
         hasTransXIn,
         hasTransLIn,
         hasAnal;


    float inRowAveReuse,
           globAveReuse;

    uint32_t groupNum;

    uint32_t
        inRowSparseDataSize,
         globSparseDataSize,
         inRowDenseDataSize,
          globDenseDataSize;

    vector<vector<int> > cacheData;///[SYS_GROUP]

    bool   CheckXInHomo();
    bool   CheckLInHomo();

    bool   CheckXDataHomo();
    bool   CheckLDataHomo(const Layer& layer);

private:
    /// ban copy constructor
	RUArray(const RUArray &);
	RUArray &operator=(const RUArray &);

public:
    RUArray():
        XIn      (SYS_HEIGHT),
        XData    (SYS_HEIGHT),
        LocIn    (SYS_GROUP,vector<int>()),            ///[SYS_GROUP]
        LocData  (SYS_GROUP,vector<SparseLocInFIFO>()),///[SYS_GROUP]
        ce       (SYS_HEIGHT),                         ///[SYS_HEIGHT]
        wloc     (SYS_GROUP,vector<SparseLocInFIFO>()),///[SYS_GROUP]
        cacheData(SYS_GROUP,vector<int>())             ///[SYS_GROUP]
    {
        assert(((1<< LOC_BIT_WIDTH   )>=GROUP_SIZE)
             &&((1<<(LOC_BIT_WIDTH-1))< GROUP_SIZE));
        #ifdef PRINT_INTERMEDIA_INFO
        this->fpXIn   = fopen(RU_XIN_FILE_PATH,"w+");
        this->fpLIn   = fopen(RU_LIN_FILE_PATH,"w+");

        this->fpLInAnalResult
            = fopen(RU_LOC_ANAL_OUT_FILE_PATH,"w+");
        #endif // PRINT_INTERMEDIA_INFO

        this->hasGenXIn = false;
        this->hasGenLIn = false;
        this->hasTransXIn=false;
        this->hasTransLIn=false;
        this->hasAnal=false;

        this->ce[0].SetNeighborFIFO(NULL,&(this->fifo[0]));
        for (int h=1;h<SYS_HEIGHT-1;h++)
            this->ce[h].SetNeighborFIFO
                (&(this->fifo[h-1])
                ,&(this->fifo[ h ]));
        this->ce[SYS_HEIGHT-1].SetNeighborFIFO(&(this->fifo[SYS_HEIGHT-2]),NULL);
        return;
    }

    #ifndef REFORMED
    bool  CheckXL(const vector<vector<vector<XTransIn> > >& SAXData) override;
    #else
    bool  CheckXL(const vector<vector<vector<SparseDataInFIFO<XTransIn::FeatureType> > > >& SAXData) override;
    #endif // REFORMED

    bool  CheckXLInHomo(Layer& thisLayer);
    bool  CheckXLDataHomo();

    void   GenXIn(const Layer& lastLayer,
                  const Layer& thisLayer,
                  const vector<vector<DFIFO> >& XIn) override;
    void   GenLIn(const vector<vector<DFIFO> >& WIn) override;
    void TransXIn(const Layer& layer);
    void TransLIn(const Layer& layer);

    inline void ClearLIn(){
        vector<vector<int> >().swap(this->LocIn);
        return;
    }
    inline void ClearXIn(){
        vector<SFIFO_In>().swap(this->XIn);
        return;
    }

    void PrintXData(const std::string& prefix = "./");
    void PrintLData(const std::string& prefix = "./");

    void AnalyzeXIn(const Layer& lastLayer);

    inline uint32_t GetInRowSparseDataSize()const{
        assert(this->hasAnal);
        return inRowSparseDataSize;
    }
    inline uint32_t GetGlobSparseDataSize()const{
        assert(this->hasAnal);
        return globSparseDataSize;
    }
    inline uint32_t GetInRowDenseDataSize()const{
        assert(this->hasAnal);
        return inRowDenseDataSize;
    }
    inline uint32_t GetGlobDenseDataSize()const{
        assert(this->hasAnal);
        return globDenseDataSize;
    }

    inline uint64_t GetTotalXInSize() const{
        uint64_t totalSize = 0;
        for (const auto& hit : this->XData)
            totalSize += hit.GetTotalSize();
        return totalSize *  SFIFO_Data::bitwidth;
    }

    ~RUArray(){
        #ifdef PRINT_INTERMEDIA_INFO
        fclose(fpXIn);
        fclose(fpLIn);
        fclose(fpLInAnalResult);
        #endif // PRINT_INTERMEDIA_INFO
    }

    #ifdef PRINT_INTERMEDIA_INFO
    void PrintXIn();
    void PrintLIn();
    void PrintLocAnalResult();
    inline void PrintXIn_out(){
        for (int h=0;h<SYS_HEIGHT;h++)
            this->ce[h].PrintXIn_out(h);
        return;
    }
    #endif // PRINT_INTERMEDIA_INFO
};

#endif // RUA_SIM_HPP
