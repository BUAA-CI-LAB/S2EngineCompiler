#ifndef SYSSIM_HPP
#define SYSSIM_HPP

#include "net.hpp"
#include "Interface.hpp"

#include "hwinfo.hpp"
#include "compiler.hpp"

#include <array>
#include <stack>
#include <cstring>

class PE{
private:
    vector<WorkLoad> workList;
    int actualWorkload;

public:
    PE(){
        this->actualWorkload = 0;
        return;
    }
    inline void Alloc(int y,int x,int k){
        this->actualWorkload++;
        this->workList.emplace_back(y,x,k);
        return;
    }
    inline void Idle(){
        this->workList.emplace_back(false);
        return;
    }
    inline int GetActWorkLoad() const{
        return this->actualWorkload;
    }
    inline uint32_t GetWorkLoad() const{
        return this->workList.size();
    }
    inline bool IfWork(int idx) const{
        return this->workList[idx].IfWork();
    }
    inline int GetX(int idx) const{
        assert(this->IfWork(idx));
        return this->workList[idx].GetX();
    }
    inline int GetY(int idx) const{
        assert(this->IfWork(idx));
        return this->workList[idx].GetY();
    }
    inline int GetK(int idx) const{
        assert(this->IfWork(idx));
        return this->workList[idx].GetK();
    }
    inline void shrink_to_fit(){
        this->workList.shrink_to_fit();
        return;
    }
    inline void PrintAt(uint32_t idx) const{
        /** print to std **/
        assert(idx>=0 && idx<this->workList.size());
        if (this->workList[idx].IfWork())
            cout<<"("<<this->workList[idx].GetX()
                <<","<<this->workList[idx].GetY()
                <<","<<this->workList[idx].GetK()
                <<")";
        else
            cout<<"(#idle#)";
        return;
    }
    inline void PrintAt(uint32_t idx,FILE *file) const{
        /** print to file **/
        assert(idx>=0 && idx<this->workList.size());
        if (this->workList[idx].IfWork()){
            fprintf(file,"(%3d,%3d,%3d)",
                    this->workList[idx].GetX(),
                    this->workList[idx].GetY(),
                    this->workList[idx].GetK());
        }
        else{
            fprintf(file,"(#idle#)");
        }
        return;
    }
};

class Systolic{
private:
    SA_RUA_Interface& SRinterface;
    SA_DFU_Interface& SDinterface;

    vector<vector<vector<PE> > > pe; ///[SYS_GROUP][SYS_HEIGHT][SYS_WIDTH]

    /** the data before translate,
     ** i.e. WIn, XIn and XOut only contents
     ** the logical information, which could
     ** be used to generate ASM later
     **
     ** while the data after translate,
     ** i.e. WTran and XTran, contents
     ** the specialized data, which could
     ** be used to generate the test data
     ** check the correctness of the so-called\
     ** compile process**/

    vector<vector<DFIFO> > WIn;///[SYS_GROUP][SYS_WIDTH]
    /** kernel input sequence **/

    #ifndef REFORMED
    vector<vector<DFIFO> > XIn;///[SYS_GROUP][SYS_HEIGHT]
    /** the X load to SA,                **
     ** store the corresponding position **
     ** (x,y only, since input is the    **
     ** same for different kernel)       **/
    #else
    vector<vector<DFIFO> > XIn;///[1][SYS_HEIGHT];
    #endif // REFORMED

    SAWorkLoad saWorkLoad[SYS_GROUP];
    /** store the workload for each    **
     ** group of the systolic array    **
     ** to generate appropriate amount **
     ** of data while some PE is idle  **/

    #ifndef REFORMED
    vector<vector<vector<WTransIn> > > WTran; ///[SYS_GROUP][SYS_WIDTH]
    /** store the translated data for SA **
     ** which means the data that could  **
     ** be directly loaded into the SA   **/
    #else
    vector<vector<vector<SparseDataInFIFO<WTransIn> > > > WTran; ///[SYS_GROUP][SYS_WIDTH]
    #endif // REFORMED

    #ifndef REFORMED
    vector<vector<vector<XTransIn> > > XTran; ///[SYS_GROUP][SYS_HEIGHT]
    /** store the translated data for SA **
     ** which means the data that could  **
     ** be directly loaded into the SA   **/
    #else
    vector<vector<vector<SparseDataInFIFO<XTransIn::FeatureType> > > > XTran; ///[    1    ][SYS_HEIGHT]
    #endif // REFORMED

    vector<vector<DFIFO> > XOut;///[SYS_GROUP][SYS_WIDTH];
    /** the output of SA, store    **
     ** the corresponding position **
     ** (x,y,k)**/

    vector<int> cacheData[SYS_GROUP][SYS_WIDTH];
    /** used for cache analysis **/

    #ifdef PRINT_INTERMEDIA_INFO
    FILE *PEfile,   /** output file of PE  **/
         *XInfile,  /** output file of XIn **/
         *WInfile,  /** output file of WIn **/
         *XOutfile; /** output file of XOut**/

    FILE *fpWInData, /** output file of WTran **/
         *fpXInData, /** output file of XTran **/
         *fpXOutData;/** output file of "XOutTran", although actually it is not created... **/

    FILE *fpUpAnalResult;
    /** output file of upper caching data analyze **/
    #endif // PRINT_INTERMEDIA_INFO

    int upDataSize;
    ///the amount of data that required to be loaded off-chip

    bool wHasGen,xHasGen,oHasGen,
         wHasTra,xHasTra,oHasTra,
         workLoadHasGen,
         hasMap,hasAnal;

    bool CheckPEW() const;
    bool CheckPEX() const;
    bool CheckPEHomo() const;
    bool CheckWInWorkLoad() const;
    bool CheckXInWorkLoad() const;
    bool CheckXOutWorkLoad() const;
    bool CheckSameGroup() const;

    #ifdef PRINT_INTERMEDIA_INFO
    void PrintWIn();
    void PrintXIn();
    void PrintXOut();
    void PrintWorkLoad();
    void PrintPE();
    void PrintPEActWorkLoad();
    #endif // PRINT_INTERMEDIA_INFO

    inline void reserveWIn (int _size);
    inline void reserveXIn (int _size);
    inline void reserveXOut(int _size);

    inline bool AlignWX(
        const vector<SparseDataInFIFO<WTransIn> >& WTran, ///[SYS_GROUP][SYS_WIDTH]
        uint32_t& lastWIdx,int& tempWIdx,
        const vector<SparseDataInFIFO<XTransIn::FeatureType> >& XTran, ///[    1    ][SYS_HEIGHT]
        uint32_t& lastXIdx,int& tempXIdx) const;

private:
    /// ban copy constructor
	Systolic(const Systolic &);
	Systolic &operator=(const Systolic &);

public:
    Systolic(SA_RUA_Interface& SRif,
             SA_DFU_Interface& SDif)
            :SRinterface(SRif),
             SDinterface(SDif),
             pe (SYS_GROUP,vector<vector<PE> >(SYS_HEIGHT,vector<PE>(SYS_WIDTH))),///[SYS_GROUP][SYS_HEIGHT][SYS_WIDTH]
             WIn(SYS_GROUP,vector<DFIFO>(SYS_WIDTH,DFIFO())),///[SYS_GROUP][SYS_WIDTH];
             #ifndef REFORMED
             XIn  (SYS_GROUP,vector<DFIFO>(SYS_HEIGHT,DFIFO())),///[SYS_GROUP][SYS_HEIGHT];
             WTran(SYS_GROUP,vector<vector<WTransIn> >(SYS_WIDTH ,vector<WTransIn>())),
             XTran(SYS_GROUP,vector<vector<XTransIn> >(SYS_HEIGHT,vector<XTransIn>())),///[SYS_GROUP][SYS_HEIGHT]
             #else
             XIn  (    1    ,vector<DFIFO>(SYS_HEIGHT,DFIFO())),///[    1    ][SYS_HEIGHT];
             WTran(SYS_GROUP,vector<vector<SparseDataInFIFO<WTransIn> > >(SYS_WIDTH ,vector<SparseDataInFIFO<WTransIn> >())),
             XTran(    1    ,vector<vector<SparseDataInFIFO<XTransIn::FeatureType> > >(SYS_HEIGHT,vector<SparseDataInFIFO<XTransIn::FeatureType> >())),///[    1    ][SYS_HEIGHT]
             #endif // REFORMED
             XOut (SYS_GROUP,vector<DFIFO>(SYS_WIDTH,DFIFO()))
        {
        #ifdef PRINT_INTERMEDIA_INFO
        this->PEfile  = fopen(PE_FILE_PATH,"w+");
        this->XInfile = fopen(SA_XIN_FILE_PATH,"w+");
        this->WInfile = fopen(SA_WIN_FILE_PATH,"w+");
        this->XOutfile= fopen(SA_XOUT_FILE_PATH,"w+");
        this->fpWInData = fopen( SA_WIN_DATA_FILE_PATH,"w+");
        this->fpXInData = fopen( SA_XIN_DATA_FILE_PATH,"w+");
        this->fpXOutData= fopen(SA_XOUT_DATA_FILE_PATH,"w+");
        this->fpUpAnalResult=fopen(SA_UPPER_ANAL_OUT_FILE_PATH,"w+");
        #endif // PRINT_TO_FILE
        hasMap  = false;
        wHasGen = false;
        xHasGen = false;
        oHasGen = false;
        wHasTra = false;
        xHasTra = false;
        oHasTra = false;
        hasAnal = false;
        workLoadHasGen = false;
        return;
    }

    void MapToSA(Layer& layer);

    void GenWInput(Layer& layer);
    void GenXInput(Layer& layer);
    void GenOutput(Layer& layer);
    void GenRAInput(Layer& layer);
    void GenWorkLoad(Layer& layer);

    void TransWIn (const Layer& layer);
    void TransXIn (Layer& layer,Layer& lastLayer);
    void TransXOut(Layer& layer);


    void PrintOutput  (const Layer& thisLayer,
                       const std::string& prefix = "./") const;
    void PrintTransedW(const std::string& prefix = "./") const;
    void PrintTransedX(const std::string& prefix = "./") const;

    bool CheckXW(Layer& thisLayer) const;

    int GetUpCacheSize() const;
    ///the minimal required size of the cache

    int GetUpDataSize() const;
    ///the amount of data that required to be loaded off-chip

    int GetInputCyc(const Layer& thisLayer) const;
    ///i.e. the ideal required cycle if the x is sufficient

    int GetUpDataReuse (const Layer& layer) const;
    ///the reused time of the up-data

    void GetDownDataSize(const Layer& thisLayer,int& dataSize,int& reluSize) const;
    ///the total output data size

    int GetLeftDataSize() const;
    ///the total input data from left of systolic array

    inline bool XInputHasGen(){
        return this->xHasGen;
    }

    void AnalyzeInputData(const Layer& thisLayer);

    static float BestMap(int h,int w,int kN,int kH);

    inline void GenRUXIn(const Layer& lastLayer,const Layer& thisLayer){
        this->SRinterface.GenXIn(lastLayer,thisLayer,this->XIn);
        return;
    }
    inline void GenRULIn(){
        this->SRinterface.GenLIn(this->WIn);
        return;
    }
    inline void TransRUXIn(const Layer& thisLayer){
        this->SRinterface.TransXIn(thisLayer);
        return;
    }
    inline void TransRULIn(const Layer& layer){
        this->SRinterface.TransLIn(layer);
        return;
    }
    inline bool CheckRUXL(){
        return this->SRinterface.CheckXL(this->XTran);
    }
    inline void CalcDFUDRAMAccess(const Layer& layer){
        assert(this->wHasGen);
        for (uint32_t i=0;i<this->WIn[0][0].GetWorkLoad();i++)
            for (uint32_t g=0;g<SYS_GROUP;g++)
                for (uint32_t w=0;w<SYS_WIDTH;w++)
                    this->SDinterface.CalcDFUDRAMAccess(
                        g*SYS_WIDTH + w,layer,
                        this->WIn[g][w].GetIdx(i)
                    );
        return;
    }
    inline void ClearWTrans(){
        #ifndef REFORMED
        vector<vector<vector<WTransIn> > >().swap(this->WTran);
        #else
        vector<vector<vector<SparseDataInFIFO<WTransIn> > > >().swap(this->WTran);
        #endif // REFORMED
        this->wHasTra = false;
        return;
    }
    inline void ClearXTrans(){
        #ifndef REFORMED
        vector<vector<vector<XTransIn> > >().swap(this->XTran);
        #else
        vector<vector<vector<SparseDataInFIFO<XTransIn::FeatureType> > > >().swap(this->XTran);
        #endif // REFORMED
        this->xHasTra = false;
        return;
    }
    inline void ClearOutput(){
        vector<vector<DFIFO > >().swap(this->XOut);
        this->oHasGen = false;
        return;
    }
    inline void ClearWIn(){
        vector<vector<DFIFO > >().swap(this->WIn);
        this->wHasGen = false;
        return;
    }
    inline void ClearXIn(){
        vector<vector<DFIFO > >().swap(this->XIn);
        this->xHasGen = false;
        return;
    }
    ~Systolic(){
        #ifdef PRINT_INTERMEDIA_INFO
        fclose(  PEfile);
        fclose( XInfile);
        fclose( WInfile);
        fclose(XOutfile);
        fclose( fpWInData);
        fclose( fpXInData);
        fclose(fpXOutData);
        fclose(fpUpAnalResult);
        #endif // PRINT_INTERMEDIA_INFO
    }
};

#endif // SYSSIM_HPP
