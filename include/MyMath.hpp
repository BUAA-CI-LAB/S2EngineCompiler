#ifndef MY_MATH_HPP
#define MY_MATH_HPP

#include "hwinfo.hpp"

#include "SysDataType.hpp"

class MyMath{
private:
    static inline bool IsPos(XTransIn::FeatureType number){
        return (number&(1<<(PARSUM_BIT_WIDTH-1)))==0;
    }
public:
    static inline bool Is16Bit(XTransIn::FeatureType number){
        #ifndef SIMULATE_16_BIT
        assert(((number>>(2*WEIGHT_BIT_WIDTH)) == -1)
             ||((number>>(2*WEIGHT_BIT_WIDTH)) ==  0));
        #endif // SIMULATE_16_BIT

        if (((((number>>WEIGHT_BIT_WIDTH)&((1<<WEIGHT_BIT_WIDTH)-1)) != ((1<<WEIGHT_BIT_WIDTH)-1))
          && (((number>>WEIGHT_BIT_WIDTH)&((1<<WEIGHT_BIT_WIDTH)-1)) != 0   ))
          ||((((number>>WEIGHT_BIT_WIDTH)&((1<<WEIGHT_BIT_WIDTH)-1)) == ((1<<WEIGHT_BIT_WIDTH)-1)) && ((number & (1<<(WEIGHT_BIT_WIDTH-1))) == 0))
          ||((((number>>WEIGHT_BIT_WIDTH)&((1<<WEIGHT_BIT_WIDTH)-1)) == 0   ) && ((number & (1<<(WEIGHT_BIT_WIDTH-1)))!=0)))
            return true;
        return false;
    }
    static inline XTransIn::FeatureType Gen16BitFeature(){
        constexpr XTransIn::FeatureType
                  tempValue = (1<<(2*WEIGHT_BIT_WIDTH-5)),
            additionalValue = (1<<   WEIGHT_BIT_WIDTH   );

        if (rand()%2==0)
            return (( (rand()%(tempValue-1))+1) + additionalValue);
        else
            return ((-(rand()% tempValue   )-1) - additionalValue);
    }
    static inline WTransIn::WeightType Gen16BitWeight(){
        constexpr WTransIn::WeightType
                  tempValue = (1<<(2*WEIGHT_BIT_WIDTH-5)),
            additionalValue = (1<<   WEIGHT_BIT_WIDTH   );

        if (rand()%2==0)
            return (( (rand()%(tempValue-1))+1) + additionalValue);
        else
            return ((-(rand()% tempValue   )-1) - additionalValue);
    }
    static inline XTransIn::FeatureType Add(XTransIn::FeatureType a,XTransIn::FeatureType b){
        if (MyMath::IsPos(a)&&MyMath::IsPos(b)){
            if (MyMath::IsPos(a+b))/// not overflow
                return (a+b);
            else{/// overflow set to max of parsum
                #ifdef PRINT_MATH_PROCESS
                std::cout<<"up-overflow : a="<<a<<" b="<<b<<std::endl;
                #endif // PRINT_MATH_PROCESS
                return ((1<<(PARSUM_BIT_WIDTH-1))-1);
            }
        }
        else if ((!MyMath::IsPos(a))
               &&(!MyMath::IsPos(b))){
            if   (!MyMath::IsPos(a+b))/// not overflow
                return (a+b);
            else{/// overflow, set to min of parsum
                #ifdef PRINT_MATH_PROCESS
                std::cout<<"down-overflow"<<std::endl;
                #endif // PRINT_MATH_PROCESS
                return (1<<(PARSUM_BIT_WIDTH-1));
            }
        }
        else/// one is positive and one is negative, will not generate overflow
            return a+b;
        assert(false);
        return 0;
    }
    static inline XTransIn::FeatureType HighBits(XTransIn::FeatureType dobleBit){
        return ((dobleBit>>WEIGHT_BIT_WIDTH));
    }
    static inline XTransIn::FeatureType  LowBits(XTransIn::FeatureType dobleBit){
        return ((dobleBit & ((1<<WEIGHT_BIT_WIDTH)-1)));
    }
    static inline XTransIn::FeatureType     Mult(XTransIn::FeatureType a,XTransIn::FeatureType b){
        return a*b;
    }
};

#endif // MY_MATH_HPP
