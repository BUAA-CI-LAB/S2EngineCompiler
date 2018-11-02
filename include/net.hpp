#ifndef NET_HPP
#define NET_HPP

#include "FIFO.hpp"
#include "NetDataType.hpp"
#include "SysDataType.hpp"
#include "MyMath.hpp"

#include "hwinfo.hpp"
#include "compiler.hpp"

#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>

using namespace std;

extern const string LayerType[];

class Kernel{
/**  kernel numbering order
 **  details seen design document
 **
 **  0   1   2   3   ... n-1
 **  n   n+1 n+2 n+3 ... 2n-1
 **          ......
 **/
private:
    vector<int>  reordered;
    vector<int>  initial;

public:
    Kernel(int kernelSize):initial(kernelSize){
        assert(kernelSize%GROUP_SIZE == 0);
        this->reordered.clear();
        return;
    }

    void Load(FILE* fpK);
    bool CheckPattern(const vector<bool>& pattern);

    void Reorder(const vector<int>& reshapeOrder,const vector<bool>& pattern);
    void PrintReorderedTo(FILE *fp)const;
    #ifndef REFORMED
    void PrintReorderedTo(vector<WTransIn>& wTrans)const;
    #else
    void PrintReorderedTo(vector<SparseDataInFIFO<WTransIn> >& wTrans,const vector<vector<SparseLoc> >& kernelLoc)const;
    #endif // REFORMED

    inline void AddValue(uint32_t idx,int value){
        assert(idx>=0 && idx<this->initial.size());
        this->initial[idx]=value;
        return;
    }
    inline int GetWeight(uint32_t idx) const{
        assert(idx>=0 && idx<this->initial.size());
        return this->initial[idx];
    }
    inline int GetReorderedSize() const{
        return this->reordered.size();
    }
};

class Layer{
private:
    const int  idx;
    static int idxCounter;
    const enum LAYER_TYPE type;
    const enum PAD_TYPE padding;
    const int lH,lW;
    const int kN,kH,kW,kD;
    const int sH,sW;
    int allGroupSize;
    Layer* preLayer;

    vector<Kernel> kernel;

    vector<vector<bool> > pattern;
    /** whether each position is non-zero
     ** for each group of the kernel **/

    vector<vector<vector<XTransIn::FeatureType> > > feature;
    /** [k][h][w] for some reason **/

    vector<vector<int> > reorderSeq;
    /** reordered group sequence **/

    vector<vector<SparseData<XTransIn::FeatureType> > > sparseFeature;
    /** feature is parted as group
     ** naming rule see design documentation **/

    vector<vector<vector<SparseLoc> > > kernelLoc;
    /** the location of every
     ** none-zero kernel in each group  **/

    bool hasLoadKernel,
         hasLoadFeature,
         hasLoadPattern,
         hasPartFeature,
         hasGenReorderSeq;

    #ifdef PRINT_INTERMEDIA_INFO
    FILE* reshapedW;
    FILE* reshapedX;
    FILE* reshapedG;
    FILE* reshapedLoc;
    FILE* fpFeature;
    #endif // PRINT_INTERMEDIA_INFO

    inline void AddInnerProduct(int x,int y,const Kernel& k,int kBegin,int kLen,XTransIn::FeatureType& parsum)const;

    /// ban copy constructor
	Layer(const Layer &);
	Layer &operator=(const Layer &);

public:
    Layer(int lH,int lW,int kN,
          int kH,int kW,int sH,int sW,int kD,
          enum PAD_TYPE padding = SAME_PAD)
          : idx(Layer::idxCounter++),
            type(CONV_LAYER),
            padding(padding),
            lH(lH),lW(lW),
            kN(kN),kH(kH),kW(kW),kD(kD),
            sH(sH),sW(sW),
               kernel( kN,Kernel(kH * kW * kD)),
              pattern((kN + KERNEL_GROUP_SIZE -1)
                          / KERNEL_GROUP_SIZE,vector<bool>(kH * kW * kD)),
              feature( kN,vector<vector<XTransIn::FeatureType> >
                            (lH,vector<XTransIn::FeatureType>(lW))),
           reorderSeq(kN/KERNEL_GROUP_SIZE),
        sparseFeature(lH*lW*kD/ GROUP_SIZE),
            kernelLoc(kN/KERNEL_GROUP_SIZE){

        assert(kN%KERNEL_GROUP_SIZE==0);

        this->hasLoadKernel  = false;
        this->hasLoadFeature = false;
        this->hasLoadPattern = false;
        this->hasPartFeature = false;
        this->hasGenReorderSeq=false;

        #ifdef PRINT_INTERMEDIA_INFO
        this->reshapedG  = fopen(RESHAPED_G_FILE_PATH  ,"w+");
        this->reshapedW  = fopen(RESHAPED_W_FILE_PATH  ,"w+");
        this->reshapedX  = fopen(RESHAPED_X_FILE_PATH  ,"w+");
        this->reshapedLoc= fopen(RESHAPED_LOC_FILE_PATH,"w+");
        this->fpFeature  = fopen(RESHAPED_F_FILE_PATH  ,"w+");
        #endif // PRINT_INTERMEDIA_INFO
        return;
    }

    inline int GetLayerIdx()const{
        return this->idx;
    }

    inline int KerIdx2GroupIdx(int kerIdx) const{
        return kerIdx/SYS_WIDTH;
    }

    inline int getLH()const {return this->lH;}
    inline int getLW()const {return this->lW;}
    inline int getKD()const {return this->kD;}

    inline int getKW()const {return this->kW;}
    inline int getKH()const {return this->kH;}
    inline int getKN()const {return this->kN;}

    inline int getSW()const {return this->sW;}
    inline int getSH()const {return this->sH;}

    inline int getKernelSize() const{
        return this->kH * this->kW * this->kD;
    }

    inline void Clean(){
        vector<Kernel>().swap(this->kernel);
        vector<vector<int > >().swap(this->reorderSeq);
        vector<vector<bool> >().swap(this->pattern   );
        vector<vector<vector<SparseLoc> > >().swap(this->kernelLoc);
        vector<vector<vector    <XTransIn::FeatureType> > >().swap(this->feature);
        vector<vector<SparseData<XTransIn::FeatureType> > >().swap(this->sparseFeature);
        return;
    }

    inline int getGroupNumInKernel(uint32_t idx) const{
        assert(idx>=0 && idx<this->kernelLoc.size());
        return this->kernelLoc[idx].size();
    }

    inline enum PAD_TYPE getPadType() const{
        return this->padding;
    }

    inline int getKernelWorkLoad(int k) const{
        if (k==IDLE){
            #ifdef REMOVE_ALL_ZERO_GROUP
            assert(false);
            #endif // REMOVE_ALL_ZERO_GROUP
            return this->reorderSeq[0].size();
        }
        assert(k>=0 && k<this->kN);
        return this->kernel[k].GetReorderedSize();
    }

    inline int getKernelGroupNum() const{
        return this->reorderSeq[0].size();
    }

    inline int getKernelLoc(int k,int g,uint32_t l) const{
        assert(k>=0 && k<this->kN);
        assert(l>=0 && l<this->kernelLoc[k][g].size());
        return this->kernelLoc[k][g][l].GetLoc();
    }

    inline void getSparseKernelGroup(int kernel,vector<SparseLocInFIFO>& sparseLoc) const{
        if (kernel==IDLE){
            #ifdef REMOVE_ALL_ZERO_GROUP
            assert(false);
            #endif // REMOVE_ALL_ZERO_GROUP
            for (uint32_t g=0;g<this->kernelLoc[0].size();g++)
                sparseLoc.emplace_back(0,true);
            return;
        }
        assert(kernel>=0 && kernel<this->kN);
        for (uint32_t g=0;g<this->kernelLoc[kernel].size();g++){
            for (uint32_t i=0;i<this->kernelLoc[kernel][g].size()-1;i++)
                sparseLoc.emplace_back(this->kernelLoc[kernel][g][i]);

            sparseLoc.emplace_back(
                    this->kernelLoc
                          [kernel][g]
                          [this->kernelLoc[kernel][g].size()-1].GetLoc(),true
                    );
        }
    }

    inline void getSparseFeatureGroup(uint32_t g,SFIFO_Data& sparseFeature) const{
        assert(this->hasPartFeature
            && g>=0 && g<this->sparseFeature.size());
        for (auto it = this->sparseFeature[g].cbegin(); it != this->sparseFeature[g].cend();it++){
            sparseFeature.AddFeature(*it,it == (this->sparseFeature[g].cend()-1));///this->sparseFeature[g][i]
        }
        return;
    }

    inline int getSparseFeatureGroupSize(uint32_t g) const{
        assert(this->hasPartFeature
            && g>=0 && g<this->sparseFeature.size());
        return this->sparseFeature[g].size();
    }

    inline int getAllGroupSize() const{
        assert(this->hasPartFeature);
        return this->allGroupSize;
    }

    inline int getFeatureGroupNum() const{
        return this->lW * this->lH * this->kN / GROUP_SIZE;
    }

    inline int getFeature(int x,int y,int k) const{
        return this->feature[k][y][x];
    }

    void LoadKernel (const string& prefix = "./");
    void LoadPattern(const string& prefix = "./");
    void LoadFeature(const string& prefix = "./");

    void Compute(const Layer& lastLayer);
    void GenReorderSeq();
    /** generate the sparse location
     ** of each group of kernels
     **/

    void Reorder();
    /** compress each kernel base on
     ** the generated sparse location
     **/

    #ifndef REFORMED
    void MatchXTo(int x,int y,int k,Layer& nextLayer,vector<XTransIn>& xTrans);
    #else
    void MatchXTo(int x,int y,int k,Layer& nextLayer,vector<SparseDataInFIFO<XTransIn::FeatureType> >& xTrans);
    #endif // REFORMED

    inline bool FeatureEqual(int x,int y,int k,XTransIn::FeatureType t) const{
        assert(this->hasLoadFeature);
        return this->feature[k][y][x]==t;
    }

    inline void SetFeature(int x,int y,int k,XTransIn::FeatureType t){
        assert(this->hasLoadFeature);
        this->feature[k][y][x]=t;
        return;
    }

    bool CheckPattern();
    bool CheckReshapedSize();

    void PartitionFeature();
    void PartitionKernel ();

    #ifdef PRINT_INTERMEDIA_INFO
    void PrintFeature();
    void PrintReorder();
    void PrintReorderedLoc();
    #endif // PRINT_INTERMEDIA_INFO

    #ifndef REFORMED
    void PrintKernelTo(int kIdx,vector<WTransIn>& wTrans)const;
    #else
    void PrintKernelTo(int kIdx,vector<SparseDataInFIFO<WTransIn> >& wTrans)const;
    #endif // REFORMED

    inline enum LAYER_TYPE getType()const{
        return this->type;
    }

    inline void Print(){
        cout<<"layer idx:"<<this->idx<<"  type:"<<LayerType[this->type]<<"\n"
            <<"height:"<<this->lH<<" width:"<<this->lW<<"\n"
            <<"Kernel height:"<<this->kH<<" width:"<<this->kW<<" number:"<<this->kN
            <<endl;
    }
    ~Layer(){
        #ifdef PRINT_INTERMEDIA_INFO
        if (this->reshapedW  !=NULL) fclose(this->reshapedW);
        if (this->reshapedX  !=NULL) fclose(this->reshapedX);
        if (this->reshapedG  !=NULL) fclose(this->reshapedG);
        if (this->fpFeature  !=NULL) fclose(this->fpFeature);
        if (this->reshapedLoc!=NULL) fclose(this->reshapedLoc);
        #endif // PRINT_INTERMEDIA_INFO
        return;
    }
};


class Net{
private:
    vector<Layer> layer;

public:
    Net(ifstream& netArch);

    inline void Print(){
        for (unsigned int i=0;i<this->layer.size();i++)
            this->layer[i].Print();
        return;
    }
    void Reshape(int smuCache,int ceFIFO);
};


class LayerDimension{
private:
    const int h;
    const int w;
    const int d;
    const int actD;

    const uint8_t featureSparse;
    const uint8_t  kernelSparse;

public:
    LayerDimension(int h,int w,int d,
                   uint8_t featureSparse = FEATURE_ZERO_PERCENT,
                   uint8_t  kernelSparse = KERNEL_SPARSE_RATE)
        :h(h),w(w),d((d+GROUP_SIZE-1)/GROUP_SIZE),actD(d),
         featureSparse(featureSparse),
          kernelSparse( kernelSparse){
        assert(this->featureSparse>=0 && this->featureSparse<=100);
        assert(this-> kernelSparse>=0 && this-> kernelSparse<=100);
        return;
    }
    inline int GetH   () const{return this->h;}
    inline int GetW   () const{return this->w;}
    inline int GetD   () const{return this->d * GROUP_SIZE;}
    inline int GetActD() const{return this->actD;}
    inline int GetFS  () const{return this->featureSparse;}
    inline int GetKS  () const{return this-> kernelSparse;}
    inline string toString() const{
        return string("(")+std::to_string(this->h)
              +string(",")+std::to_string(this->w)
              +string(",")+std::to_string(this->actD)
              +string(")");
    }
    inline double GetWorkLoad(const LayerDimension& lastLayer,int kH,int kW) const{
        return (2.0 * this->h * this->w * this->actD) /1024.0 * ( 1.0 * kH * kW * lastLayer.GetActD())/1024.0;
    }
};
#endif // NET_HPP
