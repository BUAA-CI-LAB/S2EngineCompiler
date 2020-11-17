#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "net.hpp"
#include "hwinfo.hpp"
#include "compiler.hpp"
#include <algorithm>
#include <vector>

using namespace std;

class Memory{
public:
    typedef uint32_t MemSizeType;
    typedef uint32_t  RecIdxType;

private:
    class Record{
        /// the class to record the basic information
        /// of the records in memory
    protected:
        Memory::MemSizeType _size;
        Memory::MemSizeType _begin;

    public:
        Record(Memory::MemSizeType _size,
               Memory::MemSizeType _begin)
            :_size (_size ),
             _begin(_begin){
            return;
        }

        inline Memory::MemSizeType GetMemSize() const{
            return this->_size;
        }
        inline Memory::MemSizeType GetBegAddr() const{
            return this->_begin;
        }
        inline virtual Memory::MemSizeType GetEndAddr() const{
            return this->_begin + this->_size - 1;
        }
    };

    const MemSizeType memVolume;
    const bool limited;

    MemSizeType memSize;

    vector<Record> recordList;

    /// ban copy constructor
	Memory(const Memory &);
	Memory &operator=(const Memory &);

protected:
    inline void PushRecordBack(MemSizeType _size,
                               MemSizeType _begin){
        assert(this->memSize + _size <= this->memVolume);
        this->memSize += _size;
        this->recordList.emplace_back(_size,_begin);
        return;
    }
    inline void RemoveRecord(RecIdxType idx){
        assert(this->recordList.size()>idx);
        this->memSize -= this->recordList[idx].GetMemSize();
        this->recordList.erase(this->recordList.begin()+idx);
        return;
    }
    inline MemSizeType GetSize   (RecIdxType idx) const{
        assert(this->recordList.size()>idx);
        return this->recordList[idx].GetMemSize();
    }
    inline MemSizeType GetBegAddr(RecIdxType idx) const{
        assert(this->recordList.size()>idx);
        return this->recordList[idx].GetBegAddr();
    }
    inline MemSizeType GetEndAddr(RecIdxType idx) const{
        assert(this->recordList.size()>idx);
        return this->recordList[idx].GetEndAddr();
    }

public:
    Memory(MemSizeType memVolume):
        memVolume(memVolume),
        limited(true){
        /// memory with limited size
        this->recordList.clear();
        this->memSize = 0;
        return;
    }
    Memory():memVolume(0),limited(false){
        /// memory with unlimited size
        this->recordList.clear();
        this->memSize = 0;
        return;
    }
    inline MemSizeType GetSize() const{
        return this->memSize;
    }
    inline MemSizeType MemVolume() const{
        return this->memVolume;
    }
};

class CycleBuf: private Memory{
public:
    typedef uint32_t RecKeyType;
protected:
    /**  Full: ( memTop + 1 ) % memVolume = memBot  **/
    /** Empty:   memTop == memBot  **/
    MemSizeType memBot;
    MemSizeType memTop;

    vector<RecKeyType> _key;

    inline void Release(MemSizeType extraSize){
        /// release memory record to get extraSize
        assert(extraSize <= this->MemVolume());
        MemSizeType tragetSize = this->GetSize() - extraSize;
        while (this->GetSize() > tragetSize){
            this->RemoveRecord(0);
            this->_key.erase(this->_key.begin());
        }
        return;
    }
    inline void ReleaseUntil(MemSizeType restMem){
        /// release memory record until the size of free space
        /// is bigger than the restMem
        assert(restMem <= this->MemVolume());
        MemSizeType tragetSize = this->MemVolume() - restMem;
        while (this->GetSize() > tragetSize){
            this->memBot = this->GetEndAddr(0) + 1;
            this->RemoveRecord(0);
            this->_key.erase(this->_key.begin());
        }
        return;
    }
    inline void RegRecord(RecKeyType recKey,MemSizeType memSize){
        /// add a new record to the record list,
        /// replace current records if necessary
        assert(memSize > 0 && memSize <= CYCBUF_BLOCK_SIZE);
        assert(this->_key.empty() || (this->GetEndAddr(this->_key.size()-1) % this->MemVolume() == this->memTop));

        this->ReleaseUntil  (memSize);
        this->_key.emplace_back(recKey);
        this->PushRecordBack(memSize,this->memTop);
        this->memTop = (this->GetEndAddr(this->_key.size()-1) % this->MemVolume());
        return;
    }

public:
    CycleBuf(MemSizeType memVolume):
      Memory(memVolume){
        assert(memVolume >= CYCBUF_BLOCK_SIZE);
        this->_key.clear();
        this->memBot = 0;
        this->memTop = 0;
        return;
    }
    CycleBuf():Memory(){
        this->_key.clear();
        this->memBot = 0;
        this->memTop = 0;
        return;
    }
    inline MemSizeType GetRec(RecIdxType recKey,MemSizeType memSize){
        /// the main handle for the outer,
        /// return the total DRAM access bits during the process
        vector<RecIdxType>::iterator found
            = find(this->_key.begin(),this->_key.end(),recKey);
        if (found == this->_key.end()){
            this->RegRecord(recKey,memSize);
            return memSize;
        }
        return 0;
    }
};

class DFUCache{
public:
    typedef uint32_t  TransMemType;
    typedef uint32_t  LayerIdxType;
    typedef uint32_t  BlockIdxType;
    typedef uint32_t KernelIdxType;

private:
    class BlockKey{
    private:
        static BlockIdxType keyCounter;
         LayerIdxType  layerIdx;
        KernelIdxType kernelIdx;
         BlockIdxType  blockIdx;
         BlockIdxType  blockKey;

    public:
        BlockKey(
             LayerIdxType  layerIdx,
            KernelIdxType kernelIdx,
             BlockIdxType  blockIdx){
            this-> layerIdx =  layerIdx;
            this-> blockIdx =  blockIdx;
            this-> blockKey =  BlockKey::keyCounter++;
            this->kernelIdx = kernelIdx;
            return;
        }

        inline bool Equal(
             LayerIdxType  layerIdx,
            KernelIdxType kernelIdx,
             BlockIdxType  blockIdx){
            return this-> layerIdx ==  layerIdx
                && this-> blockIdx ==  blockIdx
                && this->kernelIdx == kernelIdx;
        }

        inline BlockIdxType Key(){
            return this->blockKey;
        }
    };

///    ReadOnlyMem dram; /// not necessary unless to generate instructions
    CycleBuf scratchPad;

    TransMemType    totalReadMem;
    TransMemType curLayerReadMem;

    LayerIdxType nowLayerIdx;

    vector<BlockKey> blockKeys;

    inline BlockIdxType AllocBlockIdx(
             LayerIdxType  layerIdx,
            KernelIdxType kernelIdx,
             BlockIdxType  blockIdx){
        for (auto& blockKey : blockKeys)
            if (blockKey.Equal(
                         layerIdx,kernelIdx,blockIdx))
                return blockKey.Key();
        this->blockKeys.emplace_back(
                    layerIdx,kernelIdx,blockIdx);
        return this->blockKeys.back().Key();
    }

public:
    DFUCache():
        scratchPad(DFU_CACHE_SIZE){
        this->   totalReadMem = 0;
        this->curLayerReadMem = 0;
        this->nowLayerIdx  = 0;
        return;
    }
/**    inline void AllocToDRAM(Layer& layer,KernelType kerIdx){
 *         if (this->dram.RegRecord(layer.getKernelWorkLoad(kerIdx)))
 *             return;
 *         assert(false);
 *         return;
 *     }
 */
    inline void CalcDFUDRAMAccess(const Layer& layer,KernelIdxType kerIdx){
        Memory::MemSizeType kernelSize = layer.getKernelWorkLoad(kerIdx);
        LayerIdxType layerIdx = layer.GetLayerIdx();
        BlockIdxType serial   = 0;
        if (layerIdx != this->nowLayerIdx){
            this->nowLayerIdx     = layerIdx;
            this->curLayerReadMem = 0;
        }
        while (true){
            Memory::MemSizeType tempReadMem;
            BlockIdxType blockIdx = this->AllocBlockIdx(layerIdx,kerIdx,serial);
            if (kernelSize  > CYCBUF_BLOCK_SIZE){
                kernelSize -= CYCBUF_BLOCK_SIZE;
                tempReadMem = this->scratchPad.GetRec(blockIdx,CYCBUF_BLOCK_SIZE);
                this->   totalReadMem += tempReadMem;
                this->curLayerReadMem += tempReadMem;
            }
            else{
                tempReadMem = this->scratchPad.GetRec(blockIdx,kernelSize);
                this->   totalReadMem += tempReadMem;
                this->curLayerReadMem += tempReadMem;
                break;
            }
            serial++;
        }
        return;
    }
    inline Memory::MemSizeType   GetCurReadMem() const{
        return this->curLayerReadMem;
    }
    inline Memory::MemSizeType GetTotalReadMem() const{
        return this->totalReadMem;
    }
};

#endif // MEMORY_HPP
 
