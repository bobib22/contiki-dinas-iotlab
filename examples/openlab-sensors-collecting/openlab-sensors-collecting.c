#include "contiki.h"
#include <stdio.h>
#include "dev/light-sensor.h"
#include "dev/acc-mag-sensor.h"

PROCESS(sensor_collection, "Sensors collection");
AUTOSTART_PROCESSES(&sensor_collection);

/*
 * Light sensor
 */
static void config_light()
{
  light_sensor.configure(LIGHT_SENSOR_SOURCE, ISL29020_LIGHT__AMBIENT);
  light_sensor.configure(LIGHT_SENSOR_RESOLUTION, ISL29020_RESOLUTION__16bit);
  light_sensor.configure(LIGHT_SENSOR_RANGE, ISL29020_RANGE__1000lux);
  SENSORS_ACTIVATE(light_sensor);
}
static void process_light()
{
  int light_val = light_sensor.value(0);
  float light = ((float)light_val) / LIGHT_SENSOR_VALUE_SCALE;
  printf("light_sensor: %f lux\n", light);
}


/*
 * Accelerometer / magnetometer
 */
static void config_acc()
{
  acc_sensor.configure(ACC_MAG_SENSOR_DATARATE,
      LSM303DLHC_ACC_RATE_1344HZ_N_5376HZ_LP);
  acc_sensor.configure(ACC_MAG_SENSOR_SCALE,
      LSM303DLHC_ACC_SCALE_2G);
  acc_sensor.configure(ACC_MAG_SENSOR_MODE,
      LSM303DLHC_ACC_UPDATE_ON_READ);
  SENSORS_ACTIVATE(acc_sensor);
}

static void process_acc()
{
  int xyz[3];
  static unsigned count = 0;
  if ((++count % 1000) == 0) {
    // print every 1000 values
    xyz[0] = acc_sensor.value(ACC_MAG_SENSOR_X);
    xyz[1] = acc_sensor.value(ACC_MAG_SENSOR_Y);
    xyz[2] = acc_sensor.value(ACC_MAG_SENSOR_Z);

    printf("accelerometer: x %d y %d z %d\n", xyz[0], xyz[1], xyz[2]);
  }
}

static void config_mag()
{
  SENSORS_ACTIVATE(mag_temp_sensor);
}

static void process_mag()
{
  int xyz[3];
  xyz[0] = mag_temp_sensor.value(ACC_MAG_SENSOR_X);
  xyz[1] = mag_temp_sensor.value(ACC_MAG_SENSOR_Y);
  xyz[2] = mag_temp_sensor.value(ACC_MAG_SENSOR_Z);

  printf("magnetometer: x %d y %d z %d\n", xyz[0], xyz[1], xyz[2]);
}

static void process_temp()
{
  printf("temperature: %d °C\n", mag_temp_sensor.value(TEMP_SENSOR) / \
      TEMP_SENSOR_VALUE_SCALE);
}




/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_collection, ev, data)
{
  PROCESS_BEGIN();
  static struct etimer timer;

  config_light();
  config_acc();
  config_mag();

  etimer_set(&timer, CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER) {
      process_light();
      process_temp();

      etimer_reset(&timer);
    } else if (ev == sensors_event && data == &acc_sensor) {
      process_acc();
    } else if (ev == sensors_event && data == &mag_temp_sensor) {
      process_mag();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/