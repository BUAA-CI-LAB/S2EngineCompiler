#ifndef DFU_HPP
#define DFU_HPP

#include "Memory.hpp"
#include "Interface.hpp"

class DFU: public SA_DFU_Interface{
private:
    DFUCache dfuCaches[DFU_CACHE_NUM];

public:
    inline void CalcDFUDRAMAccess(
            int  saColIdx,const Layer& layer,int kernelIdx
        ) override{
        assert(saColIdx>=0 && saColIdx<(SYS_GROUP * SYS_WIDTH));
        this->dfuCaches[saColIdx/SA_COLUMN_PER_DFU].CalcDFUDRAMAccess(
            layer,kernelIdx);
        return;
    }
    inline Memory::MemSizeType   GetCurReadMem() const{
        Memory::MemSizeType tempMemSize = 0;
        for (int dfuCacheIdx=0;dfuCacheIdx<DFU_CACHE_NUM;dfuCacheIdx++)
            tempMemSize += this->dfuCaches[dfuCacheIdx].GetCurReadMem();
        return tempMemSize;
    }
    inline Memory::MemSizeType GetTotalReadMem() const{
        Memory::MemSizeType tempMemSize = 0;
        for (int dfuCacheIdx=0;dfuCacheIdx<DFU_CACHE_NUM;dfuCacheIdx++)
            tempMemSize += this->dfuCaches[dfuCacheIdx].GetTotalReadMem();
        return tempMemSize;
    }
    inline Memory::MemSizeType   GetCurReadMem(int dfuCacheIdx) const{
        assert(dfuCacheIdx>=0&&dfuCacheIdx<DFU_CACHE_NUM);
        return this->dfuCaches[dfuCacheIdx].GetCurReadMem();
    }
    inline Memory::MemSizeType GetTotalReadMem(int dfuCacheIdx) const{
        assert(dfuCacheIdx>=0 && dfuCacheIdx<DFU_CACHE_NUM);
        return this->dfuCaches[dfuCacheIdx].GetTotalReadMem();
    }
};

#endif // DFU_HPP
 
