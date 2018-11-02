#ifndef ANALYZE_HPP
#define ANALYZE_HPP

#include "net.hpp"
#include "SysSim.hpp"
#include "RUArraySim.hpp"
#include "DFUSim.hpp"

#ifdef __linux__
#include <sys/stat.h>
#endif // _linux_

#ifdef _WIN32
#include <direct.h>
#endif // _WIN32

class Analyze{
private:
    #ifdef ANALYZE
    FILE *fp_SAUpCacheSize,
         *fp_SAUpDataSize,
         *fp_SAUpDataReuse,
         *fp_SAUpDRAM_Bandwidth,
         *fp_SADownDataSize,
         *fp_SADownReluSize,
         *fp_SACycle,
         *fp_SAEqualSpeed;

    FILE *fp_RULeftDataSizeInRow,
         *fp_RULeftDataSizeGlob,
         *fp_RUInRowReuse,
         *fp_RUGlobReuse,
         *fp_RUUpLoadDataSize,
         *fp_RU_SMU_BandWidth,
         *fp_RU_DRAM_BandWidth_InRow,
         *fp_RU_DRAM_BandWidth_Glob,
         *fp_RUCycle;

    FILE *fp_DFU_DRAM_Volume,
         *fp_DFU_DRAM_BandWidth;
    #endif // ANALYZE

    int totalCyc;

    #ifdef ANALYZE
    int  PrintSysAnalReport(const Layer& thisLayer, const Systolic& sys,
                            const LayerDimension& thisLayerInfo,
                            const LayerDimension& lastLayerInfo,
                            int lastLayerCycle,int kH,int kW);
    void PrintDFUAnalReport(const DFU& dfus,int SACyclseUse);
    void PrintRUAAnalReport(const RUArray& rua,int SACyclseUse);
    #endif // ANALYZE

    void GenKernel (int kN,int kH,int kW,int kD,int zeroRatio,int _16bitRatio,const string& prefix = "./");
    void GenFeature(int  h,int  w,int  t,       int zeroRatio,int _16bitRatio,const string& prefix = "./");

    void PrintProcess(const char str[]);

    void analyze(const LayerDimension& lastLayerInfo,
                 const LayerDimension& thisLayerInfo,
                 int kH,int kW,int sH=1,int sW=1,const string& prefix = "./");

    DFU dfus;

    inline void MKDir(const string& path) const{
        #ifdef __linux__
        mkdir(path.c_str(),S_IRWXU);
        #endif // _linux_
        #ifdef _WIN32
        mkdir(path.c_str());
        #endif // _WIN32
        return;
    }
    inline void MKLayerDir(const string& prefix,
                           const string& layerName) const{
        this->MKDir(prefix+"/"+layerName            );
        this->MKDir(prefix+"/"+layerName+"/data"   );
        this->MKDir(prefix+"/"+layerName+"/transed");
        #ifdef REFORMED
        this->MKDir(prefix+"/"+layerName+"/transed/SA_XIn_reformed/");
        #else
        this->MKDir(prefix+"/"+layerName+"/transed/SA_XIn/");
        #endif // REFORMED
        this->MKDir(prefix+"/"+layerName+"/transed/SA_WIn/");
        this->MKDir(prefix+"/"+layerName+"/transed/SA_Out/");
        this->MKDir(prefix+"/"+layerName+"/transed/RU_XIn/");
        #ifndef REFORMED
        this->MKDir(prefix+"/"+layerName+"/transed/RU_LIn/");
        this->MKDir(prefix+"/"+layerName+"/transed/RU_XOut/");
        #endif // REFORMED
        return;
    }

public:
    Analyze(){
        #ifdef ANALYZE
        fp_SAUpCacheSize
            = fopen(SA_UP_CACHE_SIZE_FILE_PATH,"w+");
        fp_SAUpDataSize
            = fopen(SA_UP_DATA_SIZE_FILE_PATH,"w+");
        fp_SAUpDataReuse
            = fopen(SA_UP_DATA_REUSE_FILE_PATH,"w+");
        fp_SAUpDRAM_Bandwidth
            = fopen(SA_UP_DRAM_BANDWIDTH_FILE_PATH,"w+");
        fp_SADownDataSize
            = fopen(SA_DOWN_DATA_SIZE_FILE_PATH,"w+");
        fp_SADownReluSize
            = fopen(SA_DOWN_RELU_SIZE_FILE_PATH,"w+");
        fp_SACycle
            = fopen(SA_CYCLE_FILE_PATH,"w+");
        fp_SAEqualSpeed
            = fopen(SA_EQUAL_SPEED_FILE_PATH,"w+");

        fp_RULeftDataSizeInRow
            = fopen(RU_LEFT_DATA_SIZE_INROW,"w+");
        fp_RULeftDataSizeGlob
            = fopen(RU_LEFT_DATA_SIZE_GLOB,"w+");
        fp_RUInRowReuse
            = fopen(RU_INROW_REUSE,"w+");
        fp_RUGlobReuse
            = fopen(RU_GLOB_REUSE,"w+");
        fp_RUUpLoadDataSize
            = fopen(RU_UP_LOAD_DATA_SIZE,"w+");
        fp_RU_SMU_BandWidth
            = fopen(RU_SMU_BANDWIDTH,"w+");
        fp_RU_DRAM_BandWidth_InRow
            = fopen(RU_DRAM_BANDWIDTH_INROW,"w+");
        fp_RU_DRAM_BandWidth_Glob
            = fopen(RU_DRAM_BANDWIDTH_GLOB,"w+");
        fp_RUCycle
            = fopen(RU_CYCLE_FILE_PATH,"w+");

        fp_DFU_DRAM_Volume
            = fopen(DFU_DRAM_VOLUME_FILE_PATH,"w+");
        fp_DFU_DRAM_BandWidth
            = fopen(DFU_DRAM_BAND_WIDTH_FILE_PATH,"w+");
        #endif // ANALYZE
        return;
    }
    ~Analyze(){
        #ifdef ANALYZE
        fclose(fp_SAUpCacheSize);
        fclose(fp_SAUpDataSize);
        fclose(fp_SAUpDataReuse);
        fclose(fp_SAUpDRAM_Bandwidth);
        fclose(fp_SADownDataSize);
        fclose(fp_SADownReluSize);
        fclose(fp_SACycle);
        fclose(fp_SAEqualSpeed);

        fclose(fp_RULeftDataSizeInRow);
        fclose(fp_RULeftDataSizeGlob);
        fclose(fp_RUInRowReuse);
        fclose(fp_RUGlobReuse);
        fclose(fp_RUUpLoadDataSize);
        fclose(fp_RU_SMU_BandWidth);
        fclose(fp_RU_DRAM_BandWidth_InRow);
        fclose(fp_RU_DRAM_BandWidth_Glob);
        fclose(fp_RUCycle);

        fclose(fp_DFU_DRAM_Volume);
        fclose(fp_DFU_DRAM_BandWidth);
        #endif // ANALYZE
        return;
    }

    void AnalyzeAlexNet();
    void AnalyzeVGG16();
    void AnalyzeVGG19();

    void GenPEArrayTestData(
            LayerDimension& lastLayerInfo,
            LayerDimension& thisLayerInfo,
            int kH,int kW,int sH=1,int sW=1
        );
    void GenRUPEArrayTestData(
            LayerDimension& lastLayerInfo,
            LayerDimension& thisLayerInfo,
            int kH,int kW,int sH=1,int sW=1
        );
};

#endif // ANALYZE_HPP
