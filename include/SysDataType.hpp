#ifndef SYS_DATA_TYPE_HPP
#define SYS_DATA_TYPE_HPP

#include "hwinfo.hpp"
#include "compiler.hpp"
#include "String.hpp"

#include <stdio.h>

#include <stack>
#include <vector>
#include <iostream>
#include <fstream>

#define FEATURE_FILL_ZERO_POSITION 1
#define  KERNEL_FILL_ZERO_POSITION 0

#define IDLE -1
#define NONE -2

enum FIFODataType{
        W_FIFO_DATA=0, ///weight
       WZ_FIFO_DATA=1, ///weight-zero
        P_FIFO_DATA=2, ///pattern
       PZ_FIFO_DATA=3, ///pattern-zero
        X_FIFO_DATA=4, ///x
       XZ_FIFO_DATA=5, ///x-zero
    X_OUT_FIFO_DATA=6, ///x-output
   XZ_OUT_FIFO_DATA=7, ///x-output-zero
     RAND_FIFO_DATA=8
};

enum FIFO_CTRL{
    RAB  =0, /// read from asyn FIFO, broadcast
    RAW  =1, /// read from asyn FIFO, write to sync FIFO,
    RAWB =2, /// read from asyn FIFO, write to sync FIFO, broadcast
    RAWBZ=3, /// read from asyn FIFO, write to sync FIFO, broadcast zero
    RSB  =4, /// read from sync FIFO, broadcast
    RSBZ =5, /// read from sync FIFO, broadcast zero
    RSWB =6, /// read from sync FIFO, write to sync FIFO, broadcast
    RSWBZ=7, /// read from sync FIFO, write to sync FIFO, broadcast zero
    BZ   =8  /// broadcast zero
};

struct str_list{
	int waste;
	int cut;
	int sysGroup;
};

class XTransIn{
public:
    typedef int64_t FeatureType;

private:
    FeatureType value;
    bool ifAny;

public:
    XTransIn(FeatureType value){
        this->value= value;
        this->ifAny= false;
        return;
    }
    XTransIn(bool ifAny){
        assert(ifAny== true);
        this->ifAny = true;
        return;
    }
    inline FeatureType GetValue() const{
        if (this->ifAny)
            return rand();
        return this->value;
    }
    inline bool IfAny() const{
        return this->ifAny;
    }
};

class OTransOut{
private:
    int value;
    #ifdef PE_IDLE_NOT_OUTPUT
    bool ifIdle;
    #endif // PE_IDLE_NOT_OUTPUT

public:
    OTransOut(int value){
        this->value= value;
        #ifdef PE_IDLE_NOT_OUTPUT
        this->ifIdle=false;
        #endif // PE_IDLE_NOT_OUTPUT
        return;
    }
    #ifdef PE_IDLE_NOT_OUTPUT
    inline bool IfIdle() const{
        return this->ifIdle;
    }
    #endif // PE_IDLE_NOT_OUTPUT
    inline int GetValue() const{
        return this->value;
    }
};

class WTransIn{
public:
    typedef int64_t WeightType;

private:
    WeightType weight;
    bool EOK;

public:
    WTransIn(WeightType weight,bool EOK){
        this->weight = weight;
        this->EOK    = EOK;
        return;
    }
    inline bool IfEOK() const{
        return this->EOK;
    }
    inline WeightType GetW() const{
        return this->weight;
    }
};

class SparseLoc{
public:
    typedef uint32_t LocType;

protected:
    LocType loc;

public:
    SparseLoc(LocType loc){
        this->loc = loc;
        return;
    }
    SparseLoc(const SparseLoc& sparseLoc){
        this->loc = sparseLoc.loc;
        return;
    }
    inline LocType GetLoc() const{
        return this->loc;
    }
};

class SparseLocInFIFO : public SparseLoc{
private:
    bool eog;

public:
    SparseLocInFIFO(LocType loc,bool eog=false):SparseLoc(loc){
        this->eog = eog;
        return;
    }
    SparseLocInFIFO(SparseLoc sparseLoc,bool eog=false):SparseLoc(sparseLoc){
        this->eog = eog;
        return;
    }
    inline void SetEOS(){
        this->eog = true;
        return;
    }
    inline bool IsEOG() const{
        return this->eog;
    }
};

template <typename T>
class SparseData{
protected:
	SparseLoc::LocType loc;
    T value;

public:
    SparseData(T value,SparseLoc::LocType loc)
        :value(value){
        #ifdef SPARSE_IN_GROUP_ADDRESS
            assert(loc<GROUP_SIZE && loc>=0);
        #else
            #ifdef SPARSE_RELATIVE_ADDRESS
                assert(false);
            #else
                assert(false);
            #endif // SPARSE_RELATIVE_ADDRESS
        #endif // SPARSE_IN_GROUP_ADDRESS
        this->loc = loc;
        return;
    }
    SparseData(const SparseData<T>& data)
        :value(data.value){
        this->loc = data.loc;
        return;
    }
    inline int GetLoc() const {
        assert(this->loc!=NONE);
        return this->loc;
    }
    inline const T& GetValue()const {
        assert(this->loc!=NONE);
        return this->value;
    }
};

template <typename T>
class SparseDataInFIFO : private SparseData<T>{
private:
	bool eog; /// End-Of-Segment

public:
    SparseDataInFIFO(T value,SparseLoc::LocType loc,bool eog):SparseData<T>(value,loc){
        this->eog=eog;
        return;
    }
    SparseDataInFIFO(T value,SparseLoc loc,bool eog):SparseData<T>(value,loc.GetLoc()){
        this->eog=eog;
        return;
    }
    SparseDataInFIFO(const SparseDataInFIFO& sdif):SparseData<T>(sdif){
        this->eog=sdif.eog; /// sdif: sparse data in FIFO
        return;
    }
    SparseDataInFIFO(const SparseData<T>& value,bool eog):SparseData<T>(value),eog(eog){
        return;
    }
    inline bool IsEOG() const{
        return this->eog;
    }
    inline SparseLoc::LocType GetLoc() const{
        assert(this->loc!=NONE);
        return this->loc;
    }
    inline const T& GetValue() const{
        return this->value;
    }
    inline bool equal(const SparseDataInFIFO& sdif) const{
        return this->eog   == sdif.eog
            && this->loc   == sdif.loc
            && this->value == sdif.value;
    }
    inline void Print() const{
        std::cout<<"value:"<<this->value<<" loc:"<<this->loc<<" eog:"<<this->eog<<std::endl;
    }
};

class StrList{
private:
    int waste;
    int cut;
    int sysGroup;

public:
    StrList(int waste,int cut,int sysGroup){
        this->cut      = cut;
        this->waste    = waste;
        this->sysGroup = sysGroup;
        return;
    }
    inline int getCut()     const{ return this->cut;}
    inline int getWaste()   const{ return this->waste;}
    inline int getSysGroup()const{ return this->sysGroup;}
};

template<typename T>
class DataInSFIFO{
    /** the FIFO in the CE array
     **            &&
     ** between the CE array and the global buffer
     **
     ** used to represent the group index of the feature
     ** or the compressed feature
     **/
private:

    enum{CTRL,DATA} type;
    union uXIn{
        enum FIFO_CTRL ctrl;
        /// used in both XIn and XData to represent the control word

        T feature;
        /// used in both XIn to represent the group index of feature
        /// used in both XData to represent the compressed feature

        uXIn(){
            memset(this,0,sizeof(*this));
            return;
        }
        uXIn(const uXIn& uxin):
            feature(uxin.feature){
            this->ctrl  = uxin.ctrl;
            return;
        }
        uXIn(enum FIFO_CTRL ctrl){
            this->ctrl = ctrl;
            return;
        }
        uXIn(const T& f):feature(f){
            return;
        }
    }uData;

public:

    DataInSFIFO(enum FIFO_CTRL ctrl)
            :uData(ctrl){
        this->type = CTRL;
        return;
    }
    DataInSFIFO(const T& feature)
            :uData(feature){
        this->type = DATA;
        return;
    }
    inline bool IsCtrl() const {
        return (this->type == CTRL);
    }
    inline enum FIFO_CTRL GetCtrl() const {
        assert(this->type == CTRL);
        return this->uData.ctrl;
    }
    inline const T& GetFeature() const{
        assert(this->type==DATA);
        return this->uData.feature;
    }
    inline T GetKey() const{
        return (this->type == CTRL) ? (T(-1)) : (this->uData.feature);
    }
    inline const std::string& CtrlToString() const{
        static const std::string FIFO_Ctrl[]={
            /*0*/"RAB"  ,/// read from asyn FIFO, broadcast
            /*1*/"RAW"  ,/// read from asyn FIFO, write to FIFO,
            /*2*/"RAWB" ,/// read from asyn FIFO, write to FIFO, broadcast
            /*3*/"RAWBZ",/// read from asyn FIFO, write to FIFO, broadcast zero
            /*4*/"RSB"  ,/// read from sync FIFO, broadcast
            /*5*/"RSBZ" ,/// read from sync FIFO, broadcast zero
            /*6*/"RSWB" ,/// read from sync FIFO, write to FIFO, broadcast
            /*7*/"RSWBZ",/// read from sync FIFO, write to sync FIFO, broadcast zero
            /*8*/"BZ"    // broadcast zero
        };

        assert(this->type == CTRL);
        return FIFO_Ctrl[this->uData.ctrl];
    }
};

class DataInDFIFO{
    /** the FIFO connect to SA **
     ** and pattern port of RU **
     ** an high-level describe **
     **  where data represent  **
     **      data location     **/
private:
    int value1;
    int value2;
    int value3;
    enum FIFODataType type;

public:
    DataInDFIFO(int idx,enum FIFODataType type){
        assert(idx>=0);
        assert(type == W_FIFO_DATA);
        this->value1 = idx;
        this->value2 = NONE;
        this->value3 = NONE;
        this->type= type;
        return;
    }
    DataInDFIFO(int x,int y,enum FIFODataType type){
        assert(x>=0 && y>=0);
        assert(type == X_FIFO_DATA);
        this->value1 = x;
        this->value2 = y;
        this->value3 = NONE;
        this->type= type;
        return;
    }
    DataInDFIFO(int x,int y,int k,enum FIFODataType type){
        assert(x>=0 && y>=0 && k>=0);
        assert(type == X_OUT_FIFO_DATA);
        this->value1 = x;
        this->value2 = y;
        this->value3 = k;
        this->type= type;
        return;
    }
    DataInDFIFO(enum FIFODataType type){
        switch(type){
        case WZ_FIFO_DATA:
            this->value1 = IDLE;
            this->value2 = NONE;
            this->value3 = NONE;
            break;
        case RAND_FIFO_DATA:
        case XZ_FIFO_DATA:
            this->value1 = IDLE;
            this->value2 = IDLE;
            this->value3 = NONE;
            break;
        case XZ_OUT_FIFO_DATA:
            this->value1 = IDLE;
            this->value2 = IDLE;
            this->value3 = IDLE;
            break;
        case PZ_FIFO_DATA:
//            break;
        default:
            assert(false);
        }
        this->type= type;
        return;
    }
    inline int GetIdx() const {
        assert( this->type == W_FIFO_DATA
            ||  this->type ==WZ_FIFO_DATA);
        return this->value1;
    }
    inline int GetX() const {
        assert( this->type == X_FIFO_DATA
            ||  this->type ==XZ_FIFO_DATA
            ||  this->type == X_OUT_FIFO_DATA
            ||  this->type ==XZ_OUT_FIFO_DATA
            ||  this->type ==  RAND_FIFO_DATA);
        return this->value1;
    }
    inline int GetY() const {
        assert( this->type == X_FIFO_DATA
            ||  this->type ==XZ_FIFO_DATA
            ||  this->type == X_OUT_FIFO_DATA
            ||  this->type ==XZ_OUT_FIFO_DATA
            ||  this->type ==  RAND_FIFO_DATA);
        return this->value2;
    }
    inline int GetK() const {
        assert( this->type == X_OUT_FIFO_DATA
            ||  this->type ==XZ_OUT_FIFO_DATA);
        return this->value3;
    }
    inline enum FIFODataType GetType() const {
        return this->type;
    }
    inline bool IfIdle() const {
        assert( this->type == W_FIFO_DATA
            ||  this->type ==WZ_FIFO_DATA
            ||  this->type == X_FIFO_DATA
            ||  this->type ==XZ_FIFO_DATA
            ||  this->type == X_OUT_FIFO_DATA
            ||  this->type ==XZ_OUT_FIFO_DATA
            ||  this->type ==  RAND_FIFO_DATA);

        if (this->type==WZ_FIFO_DATA
        ||  this->type==XZ_FIFO_DATA
        ||  this->type==XZ_OUT_FIFO_DATA
        ||  this->type==  RAND_FIFO_DATA)
            return true;
        else
            return false;
    }
};

class StrMem{
private:
    int now;
    int cut;
    int waste;
    int sysGroup;

public:
    StrMem(int now,int waste,int cut){
        this->cut  = cut;
        this->now  = now;
        this->waste= waste;
        this->sysGroup=0;
        return;
    }
    StrMem(int now,int waste,int cut,int sysGroup){
        assert(sysGroup!=0 && SYS_HEIGHT%sysGroup==0);
        this->cut  = cut;
        this->now  = now;
        this->waste= waste;
        this->sysGroup=sysGroup;
        return;
    }
    StrMem(int now,struct str_list& ele){
        this->now   = now;
        this->cut   = ele.cut;
        this->waste = ele.waste;
        this->sysGroup=ele.sysGroup;
        return;
    }
    StrMem(const StrMem& mem){
        this->cut  = mem.cut;
        this->now  = mem.now;
        this->waste= mem.waste;
        this->sysGroup= mem.sysGroup;
        return;
    }
    inline int getCut(){return this->cut;}
    inline int getNow(){return this->now;}
    inline int getWaste()   {return this->waste;}
    inline int getSysGroup(){return this->sysGroup;}
    inline void Print(){
        std::cout<<"len:"<<this->now<<" waste"<<this->waste<<std::endl;
        return;
    }
};

class WorkLoad{
private:
    int x;
    int y;
    int k;
    bool ifWork;

public:
    WorkLoad(int y,int x,int k){
        this->x = x;
        this->y = y;
        this->k = k;
        this->ifWork = true;
        return;
    }
    WorkLoad (bool ifWork){
        assert(ifWork == false );
        this->ifWork = false;
    }
    inline bool IfWork() const{return this->ifWork;}
    inline int   GetX () const{return this->x;}
    inline int   GetY () const{return this->y;}
    inline int   GetK () const{return this->k;}
};

class SAWorkLoad{
private:
    std::vector<int> xWorkLoad;
    std::vector<bool> ifAny;

public:
    inline void AddWorkLoad(int xWorkLoad,bool ifAny=false){
        this->  ifAny  .push_back(  ifAny  );
        this->xWorkLoad.emplace_back(xWorkLoad);
        return;
    }
    inline int GetXWorkLoad(uint32_t idx) const{
        assert(idx>=0 && idx<this->xWorkLoad.size());
        return this->xWorkLoad[idx];
    }
    inline int GetWWorkLoad(uint32_t idx) const{
        assert(idx>=0 && idx<this->xWorkLoad.size());
        return this->xWorkLoad[idx];
    }
    inline bool IfAny(uint32_t idx) const{
        assert(idx>=0 && idx<this->ifAny.size());
        return this->ifAny[idx];
    }
    inline void reserve(int _size){
        this->  ifAny  .reserve(_size);
        this->xWorkLoad.reserve(_size);
        return;
    }
};
#endif // SYS_DATA_TYPE_HPP
