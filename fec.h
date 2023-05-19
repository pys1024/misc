#ifndef __FEC_H__
#define __FEC_H__

#ifdef __cplusplus
/* don't mangle api function names (gen) */
extern "C" {
#endif

#include <stdint.h>

#define FECMAXNUMLINKS 4
#define SERMAXNUMSYMBOLS 31

#ifndef INPHI_BOOL
#define INPHI_BOOL uint8_t
#define INPHI_TRUE 1
#define INPHI_FALSE 0
#endif

typedef enum {
  FEC_400G_KP4,   // 53.125 Gbps
  FEC_200G_KP2,   // 53.125 Gbps
  FEC_200G_KP4,   // 26.5625 Gbps
  FEC_100G_KP1,   // 53.125 Gbps
  FEC_50G_KP1,    // 26.5625 Gbps
  FEC_100G_KP2,   // 26.5625 Gbps
  FEC_100G_KR1,   // 51.5625 Gbps
  FEC_50G_KR1,    // 25.78125 Gbps
} FECRate;

typedef struct {
  /**
   * @brief 当前协议, 有效的 Symbols 数
   *  KP         31
   *  KR         15
   */
  uint32_t nSymbols;
  /**
   * @brief 符号错误数
   *  间隔数据
   */
  uint32_t InstantSER[SERMAXNUMSYMBOLS];
  /**
   * @brief 符号错误数
   *  总数据
   */
  uint64_t AccumulatedSER[SERMAXNUMSYMBOLS];
} SERData;

typedef struct {
  /**
   * @brief FEC 启动状态
   *  0           关闭
   *  1           开启
   */
  INPHI_BOOL enabled;
  /**
   * @brief FEC 启动状态
   *  0           关闭
   *  1           开启
   */
  INPHI_BOOL enabledLinks[FECMAXNUMLINKS];  //Links enabled indicator
  /**
   * @brief FEC 锁定状态
   *  0           未锁定
   *  1           锁定
   */
  INPHI_BOOL lockedLinks[FECMAXNUMLINKS];   // Links lock indicator
  /**
   * @brief 运行时间, 单位为 sec
   */
  double Time;

  // uint64_t BitCount;         // Bit Count data LSB/MSB
  // uint32_t FEC_Skew;

  // 间隔数据: 上一次获取到本次获取之间新产生的数据
  uint32_t FEC_Corrected_Ones_Interval;
  uint32_t FEC_Corrected_Zeros_Interval;
  uint32_t FEC_ErrorCount_Interval; // sum of symbol error of all links
  uint32_t FEC_Symbol_ErrorCount_Interval[FECMAXNUMLINKS];
  uint32_t FEC_CorrectedBitCount_Interval;
  double FEC_Symbol_ErrorRate_Interval; // SER
  double FEC_CorrectedBitRate_Interval; // BER
  double FEC_Frame_ErrorRate_Interval; // FER
  uint32_t FEC_CW_UncorrectedCount_Interval;
  uint32_t FEC_CW_CorrectedCount_Interval;
  uint32_t FEC_CW_ProcessedCount_Interval;
  /* double FEC_CW_UncorrectedErrorRate_Interval; */

  // 总数据: 从开始累积到现在的数据
  uint64_t AccumulatedFEC_Corrected_Ones;
  uint64_t AccumulatedFEC_Corrected_Zeros;
  uint64_t AccumulatedFEC_ErrorCount; // sum of symbol error of all links
  uint64_t AccumulatedFEC_Symbol_ErrorCount[FECMAXNUMLINKS];
  uint64_t AccumulatedFEC_CorrectedBitCount;
  double AveragedFEC_Symbol_ErrorRate;
  double AveragedFEC_CorrectedBitRate;
  double AveragedFEC_Frame_ErrorRate;
  uint64_t AccumulatedFEC_CW_UncorrectedCount;
  uint64_t AccumulatedFEC_CW_CorrectedCount;
  uint64_t AccumulatedFEC_CW_ProcessedCount;
  /* double AccumulatedFEC_CW_UncorrectedErrorRate; */
  /* SERData SER; */
  uint64_t TotalBitCount;            // Total Bit Count Processed
} FECData;

/**
 * @brief 设置 FEC 开启或关闭
 * @param enable     开关
 * @param fecRate    fec 速率
 * @return inphi_status_t
 */
inphi_status_t IPY100G_SetFECEnable(uint32_t die, INPHI_BOOL enable, FECRate fecRate);
/**
 * @brief 获取 FEC 数据
 * @param fecData
 * @return inphi_status_t
 */
inphi_status_t IPY100G_GetFECData(uint32_t die, FECData *fecData);
/**
 * @brief 清空 FEC 数据
 * @return inphi_status_t
 */
inphi_status_t IPY100G_ClearFECData(void);
/**
 * @brief 获取当前设备速率
 * @param rate
 * @return inphi_status_t
 */
inphi_status_t IPY100G_GetDevRate(FECRate *rate);
/**
 * @brief 打印 FEC 数据
 * @param fecData
 * @return inphi_status_t
 */
void IPY100G_PrintFECData(FECData *fecData);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // define __FEC_H__
