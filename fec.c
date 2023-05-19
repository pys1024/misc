#include "por_api.h"
#include "fec.h"

static FECRate mRate;
static FECData mData;

static inphi_status_t snapshot_fec(uint32_t die)
{
  int i;
  inphi_status_t status = INPHI_OK;
  por_fec_pcs_stats_t fec_stats;
  por_link_status_t link_status;
  uint64_t num_sym_errors_total = 0;
  int16_t mult = 1;
  INPHI_BOOL is_ready = INPHI_FALSE;

  status |= por_link_status_query(die, &link_status);
  if (INPHI_OK != status) {
    INPHI_CRIT("por_link_status_query failed");
    return status;
  }
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    is_ready = por_channel_is_link_ready(die, i, POR_INTF_EG_FEC);
    mData.enabledLinks[i] = is_ready;
    if (is_ready) {
      mData.enabled = INPHI_TRUE;
    }
    mData.lockedLinks[i] = link_status.lrx_fw_lock[i];
  }

  status |= por_snapshot_intf(die, POR_INTF_IG_FEC);
  if (INPHI_OK != status) {
    INPHI_CRIT("por_snapshot_intf failed");
    return status;
  }

  status |= por_fec_pcs_stats_query(die, POR_INTF_IG_FEC, &fec_stats);
  status |= por_fec_pcs_corr_cw_hist_query(die, POR_INTF_IG_FEC, &fec_stats);
  status |= por_fec_pcs_stats_print(die, &fec_stats);
  status |= por_fec_pcs_corr_cw_hist_print(die, &fec_stats);
  if (INPHI_OK != status) {
    INPHI_CRIT("por_fec_pcs_stats_query failed");
    return status;
  }
  if(POR_FPI_FP_FEC_MODES_CFG__COR_SEL__READ(die)) {
    mult = 544;
  } else {
    mult = 528;
  }


  mData.FEC_Corrected_Ones_Interval = fec_stats.num_ones_to_zeroes;
  mData.FEC_Corrected_Zeros_Interval = fec_stats.num_zeroes_to_ones;
  mData.FEC_ErrorCount_Interval = 0;
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    mData.FEC_Symbol_ErrorCount_Interval[i] = fec_stats.num_symbol_errors[i];
    mData.FEC_ErrorCount_Interval += fec_stats.num_symbol_errors[i];
    /* mData.FEC_Skew[i] = fec_stats.skew[i]; */
  }
  mData.FEC_CorrectedBitCount_Interval = fec_stats.num_bit_errors;

  mData.FEC_Symbol_ErrorRate_Interval = fec_stats.sym_error_rate;
  mData.FEC_CorrectedBitRate_Interval = fec_stats.bit_error_rate;
  mData.FEC_Frame_ErrorRate_Interval = fec_stats.frame_error_rate;

  mData.FEC_CW_UncorrectedCount_Interval = fec_stats.codewords_uncorrected;
  mData.FEC_CW_CorrectedCount_Interval = fec_stats.codewords_corrected;
  mData.FEC_CW_ProcessedCount_Interval = fec_stats.codewords_processed;
  /* mData.FEC_CW_UncorrectedErrorRate_Interval = fec_stats.; */

  // accumulated data
  mData.AccumulatedFEC_Corrected_Ones += fec_stats.num_ones_to_zeroes;
  mData.AccumulatedFEC_Corrected_Zeros += fec_stats.num_zeroes_to_ones;
  mData.AccumulatedFEC_ErrorCount += mData.FEC_ErrorCount_Interval;
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    mData.AccumulatedFEC_Symbol_ErrorCount[i] += fec_stats.num_symbol_errors[i];
    num_sym_errors_total += mData.AccumulatedFEC_Symbol_ErrorCount[i];
  }
  mData.AccumulatedFEC_CorrectedBitCount += fec_stats.num_bit_errors;

  mData.AccumulatedFEC_CW_UncorrectedCount += fec_stats.codewords_uncorrected;
  mData.AccumulatedFEC_CW_CorrectedCount += fec_stats.codewords_corrected;
  mData.AccumulatedFEC_CW_ProcessedCount += fec_stats.codewords_processed;
  if (mData.AccumulatedFEC_CW_ProcessedCount != 0) {
    mData.AveragedFEC_Symbol_ErrorRate = num_sym_errors_total / mult / mData.AccumulatedFEC_CW_ProcessedCount;
    mData.AveragedFEC_CorrectedBitRate = mData.AccumulatedFEC_CorrectedBitCount / (mult * 10) / mData.AccumulatedFEC_CW_ProcessedCount;
    mData.AveragedFEC_Frame_ErrorRate = mData.AccumulatedFEC_CW_UncorrectedCount / mData.AccumulatedFEC_CW_ProcessedCount;
    mData.TotalBitCount = mult * 10 * mData.AccumulatedFEC_CW_ProcessedCount;
  } else {
    mData.AveragedFEC_Symbol_ErrorRate = -1;
    mData.AveragedFEC_CorrectedBitRate = -1;
    mData.AveragedFEC_Frame_ErrorRate = -1;
    mData.TotalBitCount = -1;
  }

  return status;
}

inphi_status_t IPY100G_SetFECEnable(uint32_t die, INPHI_BOOL enable, FECRate fecRate)
{
  inphi_status_t status = INPHI_OK;
  por_rules_t rules;
  e_por_protocol_mode protocol;

  mRate = fecRate;
  switch (fecRate) {
  case FEC_400G_KP4:   // 53.125 Gbps
    protocol = POR_MODE_400G_KP8_TO_KP4;
    break;
  case FEC_200G_KP2:   // 53.125 Gbps
    protocol = POR_MODE_200G_KP4_TO_KP2;
    break;
  case FEC_200G_KP4:   // 26.5625 Gbps
    protocol = POR_MODE_200G_KP4_TO_KP4;
    break;
  case FEC_100G_KP1:   // 53.125 Gbps
    protocol = POR_MODE_100G_KP2_TO_KP1;
    break;
  case FEC_50G_KP1:    // 26.5625 Gbps
    protocol = POR_MODE_50G_KP1_TO_KP1;
    break;
  case FEC_100G_KP2:   // 26.5625 Gbps
    protocol = POR_MODE_100G_KP4_TO_KP2;
    break;
  case FEC_100G_KR1:   // 51.5625 Gbps
    protocol = POR_MODE_100G_KR2_TO_KR1;
    break;
  case FEC_50G_KR1:    // 25.78125 Gbps
    protocol = POR_MODE_50G_KR1_TO_KR1;
    break;
  default:
    return INPHI_ERROR;
  }

  // soft reset the ASIC
  por_soft_reset(die);

  // setup the default rules for the desired application
  por_rules_set_default(die, POR_MODE_LINE_PCS, protocol,
                        POR_FEC_TP_GEN, &rules);
  // show debug statements from the API
  rules.show_debug_info = INPHI_TRUE;

  // initialize the device
  status |= por_init(die, &rules);
  if (INPHI_OK != status) {
    INPHI_CRIT("por_init failed");
    return status;
  }

  // put the device into operational state
  status |= por_enter_operational_state(die, &rules);
  if (INPHI_OK != status) {
    if (rules.show_debug_info) {
      por_mcu_debug_log_query_dump(die);
    }
    INPHI_CRIT("por_enter_operational_state failed");
    return status;
  }

  // wait for all line RX links to lock
  status |= por_wait_for_link_ready(die, 1000*2000);
  if (INPHI_OK != status) {
    por_link_status_query_dump(die);
    INPHI_CRIT("por_wait_for_link_ready failed");
    return status;
  }

  // show the link status summary
  status |= por_link_status_query_dump(die);
  if (INPHI_OK != status) {
    INPHI_CRIT("por_link_status_query_dump failed");
    return status;
  }

  return status;
}

inphi_status_t IPY100G_ClearFECData(void)
{
  INPHI_MEMSET(&mData, 0, sizeof(FECData));
  return INPHI_OK;
}

inphi_status_t IPY100G_GetFECData(uint32_t die, FECData *fecData)
{
  if (NULL != fecData && INPHI_OK == snapshot_fec(die)) {
    memcpy(fecData, &mData, sizeof(FECData));
    return INPHI_OK;
  }
  return INPHI_ERROR;
}

inphi_status_t IPY100G_GetDevRate(FECRate *rate)
{
  *rate = mRate;
  return INPHI_OK;
}

void IPY100G_PrintFECData(FECData *fecData)
{
  int i;
  FECData *fec = fecData;
  INPHI_NOTE("Link Enabled            : %s\n", fec->enabled ? "True" : "False");
  INPHI_NOTE("Enabled Links           :");
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    INPHI_NOTE(" %d", fec->enabledLinks[i]);
  }
  INPHI_NOTE("\n");
  INPHI_NOTE("Locked Links            :");
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    INPHI_NOTE(" %d", fec->lockedLinks[i]);
  }
  INPHI_NOTE("\n");
  /* INPHI_NOTE("FEC Skew                :"); */
  /* for (i = 0; i < FECMAXNUMLINKS; i++) { */
  /*   INPHI_NOTE(" %5u", fec->FEC_Skew[i]); */
  /* } */
  /* INPHI_NOTE("\n"); */
  INPHI_NOTE("Corrected Ones          : %u\n", fec->FEC_Corrected_Ones_Interval);
  INPHI_NOTE("Corrected Zeros         : %u\n", fec->FEC_Corrected_Zeros_Interval);
  INPHI_NOTE("Total Symbol Error      : %u\n", fec->FEC_ErrorCount_Interval);
  INPHI_NOTE("Symbol Error Count      :");
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    INPHI_NOTE(" %5u", fec->FEC_Symbol_ErrorCount_Interval[i]);
  }
  INPHI_NOTE("\n");
  INPHI_NOTE("Corrected Bit Count     : %u\n", fec->FEC_CorrectedBitCount_Interval);
  INPHI_NOTE("FEC SER                 : %e\n", fec->FEC_Symbol_ErrorRate_Interval);
  INPHI_NOTE("FEC BER                 : %e\n", fec->FEC_CorrectedBitRate_Interval);
  INPHI_NOTE("FEC FER                 : %e\n", fec->FEC_Frame_ErrorRate_Interval);
  INPHI_NOTE("CW Uncorrected Count    : %u\n", fec->FEC_CW_UncorrectedCount_Interval);
  INPHI_NOTE("CW Corrected Count      : %u\n", fec->FEC_CW_CorrectedCount_Interval);
  INPHI_NOTE("CW Processed Count      : %u\n", fec->FEC_CW_ProcessedCount_Interval);
  INPHI_NOTE("AC Corrected Ones       : %lu\n", fec->AccumulatedFEC_Corrected_Ones);
  INPHI_NOTE("AC Corrected Zeros      : %lu\n", fec->AccumulatedFEC_Corrected_Zeros);
  INPHI_NOTE("AC Total Symbol Error   : %lu\n", fec->AccumulatedFEC_ErrorCount);
  INPHI_NOTE("AC Symbol Error Count   :");
  for (i = 0; i < FECMAXNUMLINKS; i++) {
    INPHI_NOTE(" %5lu", fec->AccumulatedFEC_Symbol_ErrorCount[i]);
  }
  INPHI_NOTE("\n");
  INPHI_NOTE("AC Corrected Bit Count  : %lu\n", fec->AccumulatedFEC_CorrectedBitCount);
  INPHI_NOTE("Averaged SER            : %e\n", fec->AveragedFEC_Symbol_ErrorRate);
  INPHI_NOTE("Averaged BER            : %e\n", fec->AveragedFEC_CorrectedBitRate);
  INPHI_NOTE("Averaged FER            : %e\n", fec->AveragedFEC_Frame_ErrorRate);
  INPHI_NOTE("AC CW Uncorrected Count : %lu\n", fec->AccumulatedFEC_CW_UncorrectedCount);
  INPHI_NOTE("AC CW Corrected Count   : %lu\n", fec->AccumulatedFEC_CW_CorrectedCount);
  INPHI_NOTE("AC CW Processed Count   : %lu\n", fec->AccumulatedFEC_CW_ProcessedCount);
  INPHI_NOTE("AC Total Bit Count      : %lu\n", fec->TotalBitCount);
}
