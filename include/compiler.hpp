#ifndef COMPILER_HPP
#define COMPILER_HPP

//#define NDEBUG
#include <assert.h>

//#define REMOVE_ALL_ZERO_GROUP

#define MY_DEBUG

/** check style of RU array **/
#define GLOBAL_CHECK /** easy to implement but waste lots of memory  **/
//#defien GROUP_CHECK  /** a little harder to implement but save memory **/

#define GEN_RAND_DATA_FOR_ANY_INPUT

//#define PRINT_TRANS_W_IN_PROCESS
//#define PRINT_TRANS_X_IN_PROCESS
//#define PRINT_CHECK_XW_PROCESS
//#define PRINT_MATH_PROCESS

#define PE_FILE_PATH "pe.txt"
#define PE_ACT_WORK_LOAD "PEActWorkLoad.txt"
#define SA_XIN_FILE_PATH  "SA_XIn.txt"
#define SA_WIN_FILE_PATH  "SA_WIn.txt"
#define SA_XOUT_FILE_PATH "SA_XOut.txt"

#define TRANS_FILE_TYPE string(".txt")
#define SA_XIN_DATA_FILE_PATH  "SA_XIn_Data.txt"
#define SA_WIN_DATA_FILE_PATH  "SA_WIn_Data.txt"
#define          SA_XIN_TRANS_FILE_PATH_PREFIX (std::string("./transed")\
                                              + std::string("_") + std::to_string(SYS_ROW)\
                                              + std::string("_") + std::to_string(SYS_COLUM)\
                                              + std::string("/SA_XIn/SA_XIn_Trans_"))
#define REFORMED_SA_XIN_TRANS_FILE_PATH_PREFIX (std::string("./transed")\
                                              + std::string("_") + std::to_string(SYS_ROW)\
                                              + std::string("_") + std::to_string(SYS_COLUM)\
                                              + std::string("/SA_XIn_reformed/SA_XIn_"))
#define SA_WIN_TRANS_FILE_PATH_PREFIX (std::string("./transed")\
                                     + std::string("_") + std::to_string(SYS_ROW)\
                                     + std::string("_") + std::to_string(SYS_COLUM)\
                                     + std::string("/SA_WIn/SA_WIn_Trans_"))
#define SA_OUTPUT_FILE_PATH_PREFIX (std::string("./transed")\
                                  + std::string("_") + std::to_string(SYS_ROW)\
                                  + std::string("_") + std::to_string(SYS_COLUM)\
                                  + std::string("/SA_Out/SA_Output_"))
#define SA_UPPER_ANAL_OUT_FILE_PATH "SA_WIN_ANAL.txt"

#define SA_XOUT_DATA_FILE_PATH "SA_XOut_Data.txt"

#define RU_XIN_FILE_PATH   "RU_XIN.txt"
#define RU_LIN_FILE_PATH   "RU_LIN.txt"

#define RU_LDATA_FILE_PATH_PREFIX (std::string("./transed")\
                                 + std::string("_") + std::to_string(SYS_ROW)\
                                 + std::string("_") + std::to_string(SYS_COLUM)\
                                 + std::string("/RU_LIn/RU_LIn_"))
#define RU_XDATA_FILE_PATH_PREFIX (std::string("./transed")\
                                 + std::string("_") + std::to_string(SYS_ROW)\
                                 + std::string("_") + std::to_string(SYS_COLUM)\
                                 + std::string("/RU_XIn/RU_XIn_"))
#define RU_X4SA_FILE_PATH_PREFIX (std::string("./transed")\
                                + std::string("_") + std::to_string(SYS_ROW)\
                                + std::string("_") + std::to_string(SYS_COLUM)\
                                + std::string("/RU_XOut/RU_XOut_"))
#define RU_LOC_ANAL_OUT_FILE_PATH "RU_LIN_ANAL.txt"

#define RESHAPED_G_FILE_PATH "reshapedG.txt"
#define RESHAPED_W_FILE_PATH "reshapedW.txt"
#define RESHAPED_X_FILE_PATH "reshapedX.txt"
#define RESHAPED_O_FILE_PATH "reshapedO.txt"
#define RESHAPED_F_FILE_PATH "reshapedF.txt"
#define RESHAPED_LOC_FILE_PATH "reshapedLoc.txt"

#define SA_UP_CACHE_SIZE_FILE_PATH     "./Analyze/SAUpCacheSize.txt"
#define SA_UP_DATA_SIZE_FILE_PATH      "./Analyze/SAUpDataSize.txt"
#define SA_UP_DATA_REUSE_FILE_PATH     "./Analyze/SAUpDataReuse.txt"
#define SA_UP_DRAM_BANDWIDTH_FILE_PATH "./Analyze/SAUpDRAMBW.txt"
#define SA_DOWN_DATA_SIZE_FILE_PATH    "./Analyze/SADownDataSize.txt"
#define SA_DOWN_RELU_SIZE_FILE_PATH    "./Analyze/SADownReluSize.txt"
#define SA_CYCLE_FILE_PATH             "./Analyze/SACycle.txt"
#define SA_EQUAL_SPEED_FILE_PATH       "./Analyze/SAEqualSpeed.txt"

#define RU_LEFT_DATA_SIZE_INROW "./Analyze/RULeftDataSizeInRow.txt"
#define RU_LEFT_DATA_SIZE_GLOB  "./Analyze/RULeftDataSizeGlob.txt"
#define RU_INROW_REUSE          "./Analyze/RUInRowReuse.txt"
#define RU_GLOB_REUSE           "./Analyze/RUGlobReuse.txt"
#define RU_UP_LOAD_DATA_SIZE    "./Analyze/RUUpLoadDataSize.txt"
#define RU_SMU_BANDWIDTH        "./Analyze/RU_SMU_BW.txt"
#define RU_DRAM_BANDWIDTH_INROW "./Analyze/RU_DRAM_BW_InRow.txt"
#define RU_DRAM_BANDWIDTH_GLOB  "./Analyze/RU_DRAM_BW_Glob.txt"
#define RU_CYCLE_FILE_PATH      "./Analyze/RUCycle.txt"

#define DFU_DRAM_VOLUME_FILE_PATH     "./Analyze/DFUVolume.txt"
#define DFU_DRAM_BAND_WIDTH_FILE_PATH "./Analyze/DFUBandWidth.txt"

//#define PRINT_INTERMEDIA_INFO

//#define ANALYZE

#define PRINT_PROCESS
#define PRINT_RESULT

/// use a data with lower bit-width i.e. 10 bit
/// to represent the original 16 bit data to avoid
/// overflow
//#define SIMULATE_16_BIT

//#define GENERATE_DATA

#define XUCHENG_PROTOCOL

#define XUCHENG_MISTAKE

#define TRANS_DATA

//#define ROUND_IN

//#define PRINT_TO_FILE

//#define PRINT_DEBUG_INFO

//#define PRINT_MAP_RESULT

//#define PRINT_COMPONENT

//#define PRINT_TO_STD

//#define GENERATE_CODE

#define AVOID_OVERFLOW

#ifdef GENERATE_DATA
    #define  KERNEL_FILE_PATH "./data/kernel.txt"
    #define PATTERN_FILE_PATH "./data/pattern.txt"
    #define FEATURE_FILE_PATH "./data/feature.txt"
#else
    #ifdef XUCHENG_PROTOCOL
        #define KERNEL_FILE_PATH "./weights"
        #define     IF_FILE_PATH "./input_features"
        #define     OF_FILE_PATH "./output_features"
        #define   BIAS_FILE_PATH "./bias"
    #endif // XUCHENG_PROTOCOL
#endif // GENERATE_DATA

#endif // COMPILER_HPP
