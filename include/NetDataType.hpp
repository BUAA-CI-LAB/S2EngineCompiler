#ifndef NET_DATA_TYPE_HPP
#define NET_DATA_TYPE_HPP

#include "hwinfo.hpp"
#include "compiler.hpp"

#include <string>

#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>


enum LAYER_TYPE {
    CONV_LAYER = 0,
    POOL_LAYER = 1,
    RELU_LAYER = 2,
    ADD_LAYER  = 3,
    INPUT_LAYER= 4
};

enum PAD_TYPE{
    SAME_PAD,
    ZERO_PAD,
    NONE_PAD
};

class Pattern{
private:
    bool EOK;
    int loc;

public:
    Pattern(int loc,bool EOK){
        assert(loc<GROUP_SIZE);
        this->loc = loc;
        this->EOK = EOK;
        return;
    }
    inline void PrintTo(FILE *fp){
        if (this->EOK)
            fprintf(fp,"(%d,1)",this->loc);
        else
            fprintf(fp,"(%d,0)",this->loc);
        return;
    }
    inline int GetLoc(){
        return this->loc;
    }
    inline bool IfEOK(){
        return this->EOK;
    }
};

#endif // SYSSIM_HPP
