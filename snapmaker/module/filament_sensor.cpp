// /**
//  * Marlin 3D Printer Firmware
//  */
#include "HAL.h"
#include "../../Marlin/src/pins/pins.h"
#include "../../Marlin/src/core/serial.h"
#include "../../Marlin/src/module/planner.h"
#include "filament_sensor.h"
#include "system.h"
#include "motion_control.h"

FilamentSensor filament_sensor;

void FilamentSensor::init() {
  FILAMENT_LOOP(i) {
    check_step_count[i] = FILAMENT_BASE_LEN * planner.settings.axis_steps_per_mm[E_AXIS_N(i)];
  }
}

void FilamentSensor::e0_step(uint8_t step) {
  if (step) {
    e_step_count[0]++;
  } else {
    e_step_count[0]--;
  }
}

void FilamentSensor::e1_step(uint8_t step) {
  if (step) {
    e_step_count[1]++;
  } else {
    e_step_count[1]--;
  }
}

void FilamentSensor::next_sample(uint8_t e) {
  e_step_count[e] = 0;
  start_adc[e] = filament[e].get();
}

void FilamentSensor::check() {
  FILAMENT_LOOP(i) {
    if (!is_enable(i)) {
      continue;
    }
    if (start_adc[i] == 0) {
      // The value is assigned once at startup
      next_sample(i);
      continue;
    }
    if ((e_step_count[i] >= check_step_count[i]) || (e_step_count[i] <= -check_step_count[i])) {
      int32_t diff = (filament[i].get() - start_adc[i]);
      diff = diff > 0 ? diff : -diff;
      if (diff < FILAMENT_THRESHOLD) {
        triggered[i] = true;
      } else {
        triggered[i] = false;
      }
      next_sample(i);
    }
  }
}

void FilamentSensor::debug() {
  FILAMENT_LOOP(i) {
    SERIAL_ECHOLNPAIR("s", i, " val:", filament[i].get());
    SERIAL_ECHOLNPAIR("s", i, " enable:", is_enable(i));
    SERIAL_ECHOLNPAIR("s", i, " state:", is_trigger(i));
  }
}

void FilamentSensor::test_adc(uint8_t e, float step_mm, uint32_t count) {
  uint16_t max = 0x0, min=0xffff;
  uint32_t acc = 0, time=0;
  if (e >= FILAMENT_SENSOR_COUNT) {
    return;
  }
  uint16_t last_adc = filament[e].get();
  SERIAL_ECHOLNPAIR("tast filament sensor ", e);
  for (uint32_t i = 0; i < count; i++) {
    motion_control.extrude_e(step_mm, 15 * 60);
    planner.synchronize();
    time = millis();
    while ((time + 8) > millis());
    uint16_t adc = filament[e].get();
    int32_t diff = adc - last_adc;
    if (diff > 500 || diff < -500) {
      continue;
    }
    last_adc = adc;
    SERIAL_ECHOLNPAIR("diff:", diff);
    SERIAL_ECHOLNPAIR("rawadc:", adc);
    if (diff < min) {
      min = diff;
    }
    if (diff > max) {
      max = diff;
    }
    acc += diff;
  }
  SERIAL_ECHOLNPAIR("max:", max, ", min:", min, ", avr:", acc / count);
}