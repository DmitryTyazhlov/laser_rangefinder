#include "vl53l1x.h"
#include "vl53l1x_reg.h"
#include <furi_hal_i2c.h>
#include <furi.h>

#define VL53L1X_I2C_ADDR (0x29 << 1)


static bool vl53l1x_write_reg(uint16_t reg, uint8_t value) {
    uint8_t buf[3];
    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    buf[2] = value;
    furi_hal_i2c_acquire(&furi_hal_i2c_handle_external);
    bool status = furi_hal_i2c_tx(
        &furi_hal_i2c_handle_external, VL53L1X_I2C_ADDR, buf, 3, 10);
    furi_hal_i2c_release(&furi_hal_i2c_handle_external);
    return status;
}

static bool vl53l1x_write_reg_16bit(uint16_t reg, uint16_t value) {
    uint8_t buf[4];
    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    buf[2] = value >> 8;
    buf[3] = value & 0xFF;
    furi_hal_i2c_acquire(&furi_hal_i2c_handle_external);
    bool status = furi_hal_i2c_tx(
        &furi_hal_i2c_handle_external, VL53L1X_I2C_ADDR, buf, 4, 10);
    furi_hal_i2c_release(&furi_hal_i2c_handle_external);
    return status;
}
static uint16_t vl53l1x_read_reg_16bit(uint16_t reg ){
    uint8_t reg_buf[2];
    reg_buf[0] = reg >> 8;
    reg_buf[1] = reg & 0xFF;
    uint8_t data[2] = {0};
    furi_hal_i2c_acquire(&furi_hal_i2c_handle_external);
    if(!furi_hal_i2c_trx(&furi_hal_i2c_handle_external, VL53L1X_I2C_ADDR, reg_buf, 2, data, 2, 200)) {
        return false;
    }
    furi_hal_i2c_release(&furi_hal_i2c_handle_external);
    return (data[0] << 8) | data[1];
}

static void set_distance_mode_long(){
    vl53l1x_write_reg(RANGE_CONFIG__VCSEL_PERIOD_A, 0x0F);
    vl53l1x_write_reg(RANGE_CONFIG__VCSEL_PERIOD_B, 0x0D);
    vl53l1x_write_reg(RANGE_CONFIG__VALID_PHASE_HIGH, 0xB8);
    // dynamic config
    vl53l1x_write_reg(SD_CONFIG__WOI_SD0, 0x0F);
    vl53l1x_write_reg(SD_CONFIG__WOI_SD1, 0x0D);
    vl53l1x_write_reg(SD_CONFIG__INITIAL_PHASE_SD0, 14); // tuning parm default
    vl53l1x_write_reg(SD_CONFIG__INITIAL_PHASE_SD1, 14); // tuning parm default
}

bool vl53l1x_init(void){

  vl53l1x_write_reg(SOFT_RESET, 0x00);
  furi_delay_us(100);
  vl53l1x_write_reg(SOFT_RESET, 0x01);

  // give it some time to boot; otherwise the sensor NACKs during the readReg()
  // call below and the Arduino 101 doesn't seem to handle that well
  furi_delay_ms(1);


  // check last_status in case we still get a NACK to try to deal with it correctly
//   while ((readReg(FIRMWARE__SYSTEM_STATUS) & 0x01) == 0 || last_status != 0)
//   {
//     if (checkTimeoutExpired())
//     {
//       did_timeout = true;
//       return false;
//     }
//   }
  // VL53L1_poll_for_boot_completion() end

  // VL53L1_software_reset() end

  // VL53L1_DataInit() begin

  // // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
  // if (io_2v8)
  // {
  //   vl53l1x_write_reg(PAD_I2C_HV__EXTSUP_CONFIG,
  //     readReg(PAD_I2C_HV__EXTSUP_CONFIG) | 0x01);
  // }

  // store oscillator info for later use
//   fast_osc_frequency = vl53l1x_read_reg_16bit(OSC_MEASURED__FAST_OSC__FREQUENCY);
//   osc_calibrate_val = vl53l1x_read_reg_16bit(RESULT__OSC_CALIBRATE_VAL);

  // VL53L1_DataInit() end

  // VL53L1_StaticInit() begin

  // Note that the API does not actually apply the configuration settings below
  // when VL53L1_StaticInit() is called: it keeps a copy of the sensor's
  // register contents in memory and doesn't actually write them until a
  // measurement is started. Writing the configuration here means we don't have
  // to keep it all in memory and avoids a lot of redundant writes later.

  // the API sets the preset mode to LOWPOWER_AUTONOMOUS here:
  // VL53L1_set_preset_mode() begin

  // VL53L1_preset_mode_standard_ranging() begin

  // values labeled "tuning parm default" are from vl53l1_tuning_parm_defaults.h
  // (API uses these in VL53L1_init_tuning_parm_storage_struct())

  // static config
  // API resets PAD_I2C_HV__EXTSUP_CONFIG here, but maybe we don't want to do
  // that? (seems like it would disable 2V8 mode)
  vl53l1x_write_reg_16bit(DSS_CONFIG__TARGET_TOTAL_RATE_MCPS, TARGET_RATE); // should already be this value after reset
  vl53l1x_write_reg(GPIO__TIO_HV_STATUS, 0x02);
  vl53l1x_write_reg(SIGMA_ESTIMATOR__EFFECTIVE_PULSE_WIDTH_NS, 8); // tuning parm default
  vl53l1x_write_reg(SIGMA_ESTIMATOR__EFFECTIVE_AMBIENT_WIDTH_NS, 16); // tuning parm default
  vl53l1x_write_reg(ALGO__CROSSTALK_COMPENSATION_VALID_HEIGHT_MM, 0x01);
  vl53l1x_write_reg(ALGO__RANGE_IGNORE_VALID_HEIGHT_MM, 0xFF);
  vl53l1x_write_reg(ALGO__RANGE_MIN_CLIP, 0); // tuning parm default
  vl53l1x_write_reg(ALGO__CONSISTENCY_CHECK__TOLERANCE, 2); // tuning parm default

  // general config
  vl53l1x_write_reg_16bit(SYSTEM__THRESH_RATE_HIGH, 0x0000);
  vl53l1x_write_reg_16bit(SYSTEM__THRESH_RATE_LOW, 0x0000);
  vl53l1x_write_reg(DSS_CONFIG__APERTURE_ATTENUATION, 0x38);

  // timing config
  // most of these settings will be determined later by distance and timing
  // budget configuration
  vl53l1x_write_reg_16bit(RANGE_CONFIG__SIGMA_THRESH, 360); // tuning parm default
  vl53l1x_write_reg_16bit(RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS, 192); // tuning parm default

  // dynamic config

  vl53l1x_write_reg(SYSTEM__GROUPED_PARAMETER_HOLD_0, 0x01);
  vl53l1x_write_reg(SYSTEM__GROUPED_PARAMETER_HOLD_1, 0x01);
  vl53l1x_write_reg(SD_CONFIG__QUANTIFIER, 2); // tuning parm default

  // VL53L1_preset_mode_standard_ranging() end

  // from VL53L1_preset_mode_timed_ranging_*
  // GPH is 0 after reset, but writing GPH0 and GPH1 above seem to set GPH to 1,
  // and things don't seem to work if we don't set GPH back to 0 (which the API
  // does here).
  vl53l1x_write_reg(SYSTEM__GROUPED_PARAMETER_HOLD, 0x00);
  vl53l1x_write_reg(SYSTEM__SEED_CONFIG, 1); // tuning parm default

  // from VL53L1_config_low_power_auto_mode
  vl53l1x_write_reg(SYSTEM__SEQUENCE_CONFIG, 0x8B); // VHV, PHASECAL, DSS1, RANGE
  vl53l1x_write_reg_16bit(DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, 200 << 8);
  vl53l1x_write_reg(DSS_CONFIG__ROI_MODE_CONTROL, 2); // REQUESTED_EFFFECTIVE_SPADS

  // VL53L1_set_preset_mode() end

  // default to long range, 50 ms timing budget
  // note that this is different than what the API defaults to
  set_distance_mode_long();
//   setMeasurementTimingBudget(50000);

  // VL53L1_StaticInit() end

  // the API triggers this change in VL53L1_init_and_start_range() once a
  // measurement is started; assumes MM1 and MM2 are disabled
  uint16_t tmp = vl53l1x_read_reg_16bit(MM_CONFIG__OUTER_OFFSET_MM) * 4;
  vl53l1x_write_reg_16bit(ALGO__PART_TO_PART_RANGE_OFFSET_MM, tmp);

  return true;
}

bool vl53l1x_read_distance(uint16_t* distance_mm) {
    uint16_t reg = 0x001E; // Пример регистра (Distance)
    uint8_t reg_buf[2];
    reg_buf[0] = reg >> 8;
    reg_buf[1] = reg & 0xFF;
    uint8_t data[2] = {0};
    furi_hal_i2c_acquire(&furi_hal_i2c_handle_external);
    if(!furi_hal_i2c_trx(&furi_hal_i2c_handle_external, VL53L1X_I2C_ADDR, reg_buf, 2, data, 2, 200)) {
        return false;
    }
    furi_hal_i2c_release(&furi_hal_i2c_handle_external);
    *distance_mm = (data[0] << 8) | data[1];
    return true;
}