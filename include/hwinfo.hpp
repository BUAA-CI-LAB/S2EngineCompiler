#ifndef HDINFO_HPP
#define HDINFO_HPP
/**
 *  hardware information
 **/

/** frequency of different parts of the accelerator (unit:MHz) **/
#define SYSTOLIC_ARRAY_FREQUENCE 700
#define RU_ARRAY_FREQUENCE 1500

/** Bit-width **/
#define PARSUM_BIT_WIDTH 31
#define WEIGHT_BIT_WIDTH 8
#define    LOC_BIT_WIDTH 4
#define    EOG_BIT_WIDTH 1
#define    EOW_BIT_WIDTH 1
#define RELATIVE_LOC_BIT_WIDTH 2

//#define REFORMED

/** network related parameter **/
/// if the systolic array support 16-bit
#define MIXED_PRECISION

/** PE behavior **/
//#define PE_IDLE_NOT_OUTPUT

/** coding style for FIFO in RU array **/
#define STATUS_BIT_EOS /** use an extra bit to mark the end-of-segment **/
//#define BUBBLE_EOS

/** RU Sparse Strategy **/
    /// in RU array
    #define SPARSE_IN_GROUP_ADDRESS
    //#define SPARSE_RELATIVE_ADDRESS
    /// in SA output
    //#define SA_OUT_SPARSE_IN_GROUP_ADDRESS
    #define SA_OUT_SPARSE_RELATIVE_ADDRESS
    /// SMU to RU array
    #define SPARSE_IN_GROUP_ADDRESS_TO_RU
    //#define SPARSE_RELATIVE_ADDRESS_TO_RU

/** Storage size relevance **/
#define GROUP_SIZE 16 //consider the FIFO size between CE
#define WEIGHT_SIZE (WEIGHT_BIT_WIDTH\
                   +    LOC_BIT_WIDTH\
                   +    EOW_BIT_WIDTH) /// unit: b
#define    CYCBUF_BLOCK_SIZE (128) ///unit: WEIGHT_SIZE
#define TOTAL_DFU_CACHE_SIZE (1024*CYCBUF_BLOCK_SIZE*SYS_GROUP*SYS_WIDTH) /// CACHE for total DFU
#define    SA_COLUMN_PER_DFU (4)/// Number of columns of systolic array managed by each DFU
#define       DFU_CACHE_SIZE (TOTAL_DFU_CACHE_SIZE/(SYS_GROUP*SYS_WIDTH)*SA_COLUMN_PER_DFU)
#define       DFU_CACHE_NUM  ((SYS_GROUP*SYS_WIDTH)/SA_COLUMN_PER_DFU)

/** Systolic array size **/
#define SYS_ROW   16
#define SYS_COLUM 16

#define SYS_HEIGHT SYS_ROW
#ifdef REFORMED
    #define SYS_WIDTH  1
    #define SYS_GROUP  SYS_COLUM
#else
    #define SYS_WIDTH  SYS_COLUM
    #define SYS_GROUP  1
#endif // REFORMED

#define KERNEL_GROUP_SIZE SYS_WIDTH

#define MINIMA_WEIGHT_INTERVAL 15

/// for network sparsity
#define KERNEL_SPARSE_RATE   80
#define FEATURE_ZERO_PERCENT 80

/// for value precision
#define KERNEL_16_BIT_RATE 0     ///  KERNEL_16_BIT_RATE%    of the non-zero weight     is 16 bit
#define FEATURE_16_BIT_PERCENT 0 /// FEATURE_16_BIT_PERCENT% of the non-zero activation is 16 bit

#endif // HDINFO_HPP
