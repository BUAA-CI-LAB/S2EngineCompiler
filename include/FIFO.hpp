#ifndef FIFO_HPP
#define FIFO_HPP

#include "compiler.hpp"

#include "MyMath.hpp"

#include "SysDataType.hpp"

#include <functional>
#include <algorithm>
#include <vector>
#include <stack>

template<typename T>
class SFIFO{
/**
 **   # actually is not a FIFO since it does not #
 **   # share  the same behaver with class FIFO  #
 **
 **     the FIFO connect to SMU
 **     SMU -> CE
 **
 **    ## use ctrl data to seg different group ##
 **/
protected:
    std::vector<DataInSFIFO<T> > data;
    /** group serial or actual data **/

    #ifndef NDEBUG
    bool hasSorted;
    #endif // NDEBUG

public:
    SFIFO(){
        this->data.clear();
        #ifndef NDEBUG
        this->hasSorted = false;
        #endif // NDEBUG
        return;
    }
    inline enum FIFO_CTRL GetCtrl(uint32_t idx) const{
        assert(idx<this->data.size() && !this->hasSorted);
        return this->data[idx].GetCtrl();
    }
    inline const T& GetFeature(uint32_t idx) const{
        assert(idx<this->data.size());
        return this->data[idx].GetFeature();
    }
    inline bool IsEnd(uint32_t idx) const{
        return (idx == (this->data.size()-1));
    }
    inline uint32_t GetWorkLoad() const{
        return this->data.size();
    }
    inline uint32_t GetCtrlNum() const{
        uint32_t ctrlNum=0;
        for (const auto& it : this->data)
            if (it.IsCtrl())
                ctrlNum++;
        return ctrlNum;
    }
    inline bool IfCtrl(uint32_t idx) const{
        assert(idx<this->data.size());
        return this->data[idx].IsCtrl();
    }
    inline void AddCtrl(enum FIFO_CTRL ctrl){
        assert(!this->hasSorted);
        this->data.emplace_back(ctrl);
        return;
    }
    inline void AddFeature(const T& feature){
        assert(!this->hasSorted);
        this->data.emplace_back(feature);
        return;
    }
    inline void reserve(int len){
        assert(!this->hasSorted);
        this->data.reserve(len);
        return;
    }
    inline void shrink_to_fit(){
        assert(!this->hasSorted);
        this->data.shrink_to_fit();
        return;
    }
};

class SFIFO_In : public SFIFO<int32_t>{
private:
    #ifndef NDEBUG
    bool groupNumHasCalced;
    #endif // NDEBUG
    uint32_t groupNum;

    std::vector<bool> isZeroGroup;/// for zero padding

public:
    #ifndef NDEBUG
    SFIFO_In():SFIFO<int32_t>(){
        this->groupNumHasCalced = false;
        this->isZeroGroup.clear();
        return;
    }
    #endif // NDEBUG

    inline void Sort(){
        for (uint32_t i=0;i<this->isZeroGroup.size();i++)
            if (this->isZeroGroup[i]){
                this->isZeroGroup.erase(this->isZeroGroup.begin()+i);
                this->data.erase(this->data.begin()+i);
                i--;
            }
        int ctrlNum = 0;
        for (const auto& dit : this->data){
            if (dit.IsCtrl())
                ctrlNum++;
        }
        #ifndef NDEBUG
        this->hasSorted = true;
        #endif // NDEBUG
        std::sort(this->data.begin(),
                  this->data.end(),
               [](const DataInSFIFO<int32_t>& a,
                  const DataInSFIFO<int32_t>& b)
               -> bool {return a.GetKey()>b.GetKey();});
        for (auto it = this->data.cbegin(); it != this->data.cend();it++)
            if (it->IsCtrl()){
                this->groupNum = it - this->data.cbegin();
                break;
            }
        #ifndef NDEBUG
        this->groupNumHasCalced = true;
        #endif // NDEBUG
        return;
    }

    inline void AddFeature(const int32_t& feature){
        assert(!this->hasSorted);
        this->isZeroGroup.push_back(false);
        SFIFO::AddFeature(feature);
        return;
    }
    inline void AddCtrl(enum FIFO_CTRL ctrl){
        assert(!this->hasSorted);
        #ifdef STRAIGHT_OUT
        assert(ctrl == RAB || ctrl == BZ);
        #endif // STRAIGHT_OUT
        this->isZeroGroup.push_back(false);
        SFIFO::AddCtrl(ctrl);
        return;
    }
    inline void AddZeroGroup(){
        assert(!this->hasSorted);
        this->isZeroGroup.push_back(true);
        SFIFO::AddFeature(-1);
        return;
    }

    inline bool IsZeroGroup(uint32_t idx) const{
        assert(idx<this->data.size());
        return this->isZeroGroup[idx];
    }

    inline uint32_t GetGroupNum() const{
        assert(this->groupNumHasCalced
            && this->isZeroGroup.size() == this->data.size()
            && this->data.size() > this->groupNum
            &&(this->groupNum == 0
            ||(this->data[this->groupNum  ].IsCtrl()
            &&!this->data[this->groupNum-1].IsCtrl())));
        return this->groupNum;
    }

    inline void CopyGroupTo(std::vector<DataInSFIFO<int> >& merged) const{
        merged.insert(merged.end(),this->data.begin(),
                                   this->data.begin()+this->groupNum);
        return;
    }

    inline void Clear(){
        #ifndef NDEBUG
        this->hasSorted = true;
        #endif // NDEBUG
        std::vector<DataInSFIFO<int> >().swap(this->data);
        return;
    }

    inline void PrintHFTo(std::ofstream& ofs) const{
        assert(!(!ofs) && !this->hasSorted);
        /// human friendly
        for (const auto& it : this->data)
            if (it.IsCtrl())
                ofs<<"(C,"<<it.CtrlToString()<<+")"<<std::endl;
            else
                ofs<<"(D,"<<it.GetFeature()<<")"<<std::endl;
        return;
    }
    inline void PrintMFTo(std::ofstream& ofs) const{
        assert(!(!ofs) && !this->hasSorted);
        /// machine friendly, omit string manipulation in input
        for (const auto& it : this->data)
            if (it.IsCtrl())
                ofs<<"0 "<<it.GetCtrl()<<std::endl;
            else
                ofs<<"1 "<<it.GetFeature()<<std::endl;
        return;
    }
};

class SFIFO_Data : public SFIFO<const SparseDataInFIFO<XTransIn::FeatureType> >{
public:
//    static constexpr uint8_t
//        BitWidth =  WEIGHT_BIT_WIDTH
//                   +   LOC_BIT_WIDTH
//                   +   EOG_BIT_WIDTH
//                   + 1;/// extra mark to distinguish CTRL and DATA

    static constexpr uint8_t
        bitwidth = XTransIn::bitwidth + SparseDataInFIFO<XTransIn::FeatureType>::extraBitwidth;

    inline const XTransIn::FeatureType& GetValue(uint32_t idx) const{
        assert(idx<this->data.size() && !this->hasSorted);
        return this->data[idx].GetFeature().GetValue();
    }
    inline int GetLoc(uint32_t idx) const{
        assert(idx<this->data.size() && !this->hasSorted);
        return this->data[idx].GetFeature().GetLoc();
    }
    inline bool IsEOG(uint32_t idx) const{
        assert(idx<this->data.size() && !this->hasSorted);
        return this->data[idx].GetFeature().IsEOG();
    }
    inline void AddZeroFeatureGroup(){
        this->data.emplace_back(SparseDataInFIFO<XTransIn::FeatureType>(0,FEATURE_FILL_ZERO_POSITION,true));
        return;
    }
    inline void AddFeature(const SparseData<XTransIn::FeatureType>& feature,bool EOG){
        this->data.emplace_back(SparseDataInFIFO<XTransIn::FeatureType>(feature,EOG));
        return;
    }
    inline uint64_t GetTotalSize() const{
        #ifndef MIXED_PRECISION
        return this->data.size() - this->GetCtrlNum();
        #else
        ///assert(false); to do: improve this for 16-bit data
        return this->data.size() - this->GetCtrlNum();
        #endif // MIXED_PRECISION
    }
    inline void PrintHFTo(std::ofstream& ofs) const {
        assert(!(!ofs) && !this->hasSorted);
        /// human friendly
        for (const auto& it : this->data)
            if (it.IsCtrl())
                ofs<<"(C,"<<it.CtrlToString()<<")"<<std::endl;
            else
                ofs<<"(D,"<<it.GetFeature().GetValue()
                   <<  ","<<it.GetFeature().GetLoc()
                   <<  ","<<it.GetFeature().IsEOG()
                   <<  ")"<<std::endl;
        return;
    }
    inline void PrintMFTo(std::ofstream& ofs) const {
        assert(!(!ofs) && !this->hasSorted);
        /// machine friendly, omit string manipulation in input
        for (const auto& it : this->data)
            if (it.IsCtrl())
                ofs<<"0 "<<it.GetCtrl()<<std::endl;
            else{
                #ifndef MIXED_PRECISION
                ofs<<"1 "<<it.GetFeature().GetValue()
                   << " "<<it.GetFeature().GetLoc()
                   << " "<<it.GetFeature().IsEOG()
                   <<std::endl;
                #else
                if (MyMath::Is16Bit(it.GetFeature().GetValue())){
                    /// isData feature offset EOG is16bit
                    ofs<< "1 "<<MyMath:: LowBits(it.GetFeature().GetValue())
                       <<  " "<<it.GetFeature().GetLoc()
                       <<  " "<<it.GetFeature().IsEOG()
                       <<" 1 "<<std::endl
                       << "1 "<<MyMath::HighBits(it.GetFeature().GetValue())
                       <<  " "<<it.GetFeature().GetLoc()
                       <<  " "<<it.GetFeature().IsEOG()
                       <<" 1 "<<std::endl;
                }
                else{
                    /// isData feature offset EOG is8bit
                    ofs<< "1 "<<it.GetFeature().GetValue()
                       <<  " "<<it.GetFeature().GetLoc()
                       <<  " "<<it.GetFeature().IsEOG()
                       <<" 0 "<<std::endl;
                }
                #endif // MIXED_PRECISION
            }
        return;
    }
};

class DFIFO{
/**
 **   # actually is not a FIFO since it does not #
 **   # share  the same behaver with class FIFO  #
 **
 **     the FIFO connect to DFU
 **     DFU ->    RU    Array
 **     DFU -> Systolic Array
 **
 **     and the FIFO connect RU and SA
 **/
private:
    std::vector<DataInDFIFO> dataList;

public:
    inline void Sort(){
        for(auto it=this->dataList.begin();it!=this->dataList.end();){
            if(it->IfIdle())
                it=this->dataList.erase(it);
            else
                ++it;
        }
        std::sort(this->dataList.begin(),this->dataList.end(),
               [](const DataInDFIFO& a,
                  const DataInDFIFO& b)
               -> bool {
                    if (a.GetK()!=b.GetK())
                        return a.GetK()>b.GetK();
                    if (a.GetY()!=b.GetY())
                        return a.GetY()>b.GetY();
                    return a.GetX()>b.GetX();
                });
        return;
    }
    inline void AddInputW(int idx){
        this->dataList.emplace_back(idx,W_FIFO_DATA);
        return;
    }
    inline void AddInputX(int x,int y){
        this->dataList.emplace_back(x,y,X_FIFO_DATA);
        return;
    }
    inline void AddOutputX(int x,int y,int k){
        this->dataList.emplace_back(x,y,k,X_OUT_FIFO_DATA);
        return;
    }
    inline void AddInputP(int idx){
        this->dataList.emplace_back(idx,P_FIFO_DATA);
        return;
    }
    inline void AddZeroW(){
        this->dataList.emplace_back(WZ_FIFO_DATA);
        return;
    }
    inline void AddZeroP(){
        this->dataList.emplace_back(PZ_FIFO_DATA);
        return;
    }
    inline void AddZeroX(){
        this->dataList.emplace_back(XZ_FIFO_DATA);
        return;
    }
    inline void AddZeroXOut(){
        this->dataList.emplace_back(XZ_OUT_FIFO_DATA);
        return;
    }
    inline void AddAny(){
        this->dataList.emplace_back(RAND_FIFO_DATA);
    }
    inline int GetIdx(uint32_t idx) const{
        assert(idx <this->dataList.size());
        return this->dataList[idx].GetIdx();
    }
    inline int GetX(uint32_t idx)   const{
        assert(idx <this->dataList.size());
        return this->dataList[idx].GetX();
    }
    inline int GetY(uint32_t idx)   const{
        assert(idx <this->dataList.size());
        return this->dataList[idx].GetY();
    }
    inline int GetK(uint32_t idx)   const{
        assert(idx <this->dataList.size());
        return this->dataList[idx].GetK();
    }
    inline uint32_t GetWorkLoad()   const{
        return this->dataList.size();
    }
    inline bool IfIdle(uint32_t idx)const{
        assert(idx <this->dataList.size());
        return this->dataList[idx].IfIdle();
    }
    inline void reserve(int _size){
        this->dataList.reserve(_size);
        return;
    }
    inline void Reset(){
        this->dataList.clear();
        return;
    }
};

#endif // FIFO_HPP
 
