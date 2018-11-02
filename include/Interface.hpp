#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include "net.hpp"
#include "FIFO.hpp"

class SA_RUA_Interface{
public:
    #ifndef REFORMED
    virtual bool CheckXL(const vector<vector<vector<XTransIn> > >& SAXData) = 0;
    #else
    virtual bool CheckXL(const vector<vector<vector<SparseDataInFIFO<XTransIn::FeatureType> > > >& SAXData) = 0;
    #endif // REFORMED

    virtual void   GenXIn(const Layer& lastLayer,
                          const Layer& thisLayer,
                          const vector<vector<DFIFO> >& XIn) = 0;
    virtual void   GenLIn(const vector<vector<DFIFO> >& WIn) = 0;

    virtual void TransXIn(const Layer& layer) = 0;
    virtual void TransLIn(const Layer& layer) = 0;
};


class SA_DFU_Interface{
public:
    virtual void CalcDFUDRAMAccess(int groupIdx,const Layer& layer,int kernelIdx) = 0;
};

#endif // INTERFACE_HPP
