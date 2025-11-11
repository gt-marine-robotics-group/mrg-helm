#include <Arduino.h>
#include "board_config.h"
#include "pinout.h"

#include <PacketSerial.h>
#include "pb_encode.h"
#include "pb_decode.h"
#include "robosub.pb.h"

static PacketSerial pktserial;
static constexpr uint32_t BAUDRATE = 115200;

static constexpr size_t MAX_PROTO = 256;
static uint8_t tx_buf[MAX_PROTO];

static constexpr uint32_t TELEMETRY_PERIOD_MS = 50;
static constexpr uint32_t CMD_TIMEOUT_MS = 250;


// ------- Motor Controls (Presto) ------
#if defined(HAS_MOTORS) && HAS_MOTORS
static uint32_t last_cmd_ms = 0;
static float motor_targets[8] = {0};

// TODO: Implement motor driver functions
static void motors_init() {
  // TODO: configure PWM pins for each motor in pins::...
}

static void motors_set_all(const float motor_vals[8]) {
  // TODO: map [-1.0,1.0] to PWM/ESC outputs
  (void)motor_vals;
}

static void motors_safe_stop() {
  float zeros[8] = {0};
  motors_set_all(zeros);
}

#endif // HAS_MOTORS


#if defined(HAS_E_STOP) && HAS_E_STOP
static bool estop_active() {
  // TODO: read digital input from e-stop pin

  // TODO: Jason "consider using an interrupt to make sure e-stop has priority"
  return false;
}
#endif


#if defined(HAS_AUTONOMY_SWITCH) && HAS_AUTONOMY_SWITCH
static bool autonomy_switch_enabled() {
  // TODO: read autonomy switch pin
  return false;
}
#endif

#if defined(HAS_INDICATOR_LED) && HAS_INDICATOR_LED
static void setup_neopixel() {
  // TODO: Initialize the NeoPixel

}

static void indicator_set_rgb(float r, float g, float b) {
  // TODO: set Neopixel to the given rgb and brightness
  (void)r; (void)g; (void)b;
}
#endif

#if defined(BOARD_PRESTO) && BOARD_PRESTO
static void send_presto_state() {
  Envelope env = Envelope_init_default;
  env.header.src = kBoardId;

  env.which_payload = Envelope_presto_state_tag;
  PrestoState &out = env.payload.presto_state;
  out.header = env.header;  // keep inner header too (handy on Jetson)

  #if defined(HAS_E_STOP) && HAS_E_STOP
    out.e_stop = estop_active();
  #else
    out.e_stop = false;
  #endif

  #if defined(HAS_AUTONOMY_SWITCH) && HAS_AUTONOMY_SWITCH
    out.autonomy_switch = autonomy_switch_enabled();
  #else
    out.autonomy_switch = false;
  #endif

  pb_ostream_t s = pb_ostream_from_buffer(tx_buf, sizeof(tx_buf));
  if (pb_encode(&s, Envelope_fields, &env)) {
    pktserial.send(tx_buf, s.bytes_written);
  }
}
#endif  // BOARD_PRESTO


// ------ Sensor Board ------

#if defined(HAS_POWER_SENSING) && HAS_POWER_SENSING
static void power_sensing_init() {
  // TODO: initialize the power sensing sensors
}
static float read_voltage_v() {
  // TODO: ready your voltage values
  /*
  May be worth renaming these variable to be more specific. I don't have 
  full context, but something like compute_battery_voltage_v would be
  preferred over voltage_v jsut to reduce ambiguity.

  Will also need to update the protobuuf definitions so send Mitchell a message
  if there is a more descriptive name for this or the current values
  */
  return 0.0f;
}
static float read_current_a() {
  // TODO: read current values

  return 0.0f;
}
#endif

#if defined(HAS_PRESSURE_SENSING) && HAS_PRESSURE_SENSING
static float read_pressure_pa() {
  // TODO: read pressure sensor values
  /*
  Leave this to Mitchell and Matthew there is something funky happening that
  we've been debugging
  */
  return 0.0f;
}
#endif


#if defined(BOARD_SENSOR) && BOARD_SENSOR
static void send_sensorb_state() {
  Envelope env = Envelope_init_default;
  env.header.src = kBoardId;

  env.which_payload = Envelope_sensorb_state_tag;
  SensorBState &out = env.payload.sensorb_state;
  out.header = env.header;

  #if defined(HAS_POWER_SENSING) && HAS_POWER_SENSING
    out.voltage_v = read_voltage_v();
    out.current_a = read_current_a();
  #else
    out.voltage_v = 0;
    out.current_a = 0;
  #endif

  #if defined(HAS_PRESSURE_SENSING) && HAS_PRESSURE_SENSING
    out.pressure_pa = read_pressure_pa();
  #else
    out.pressure_pa = 0;
  #endif

  pb_ostream_t s = pb_ostream_from_buffer(tx_buf, sizeof(tx_buf));
  if (pb_encode(&s, Envelope_fields, &env)) {
    pktserial.send(tx_buf, s.bytes_written);
  }
}
#endif  // any sensor feature


// ------ Packet Handler ------
static void onPacket(const uint8_t *buffer, size_t size) {
  /*
  Receive a packet from the Jetson
  */

  if (!buffer || size == 0) return;

  Envelope env = Envelope_init_default;
  pb_istream_t in = pb_istream_from_buffer(buffer, size);
  if (!pb_decode(&in, Envelope_fields, &env)) {
    return;
  }

  switch (env.which_payload) {

    case Envelope_motor_cmd_tag:
    #if defined(HAS_MOTORS) && HAS_MOTORS
      {
        const MotorCommand &m = env.payload.motor_cmd;
        motor_targets[0] = m.motor_1;
        motor_targets[1] = m.motor_2;
        motor_targets[2] = m.motor_3;
        motor_targets[3] = m.motor_4;
        motor_targets[4] = m.motor_5;
        motor_targets[5] = m.motor_6;
        motor_targets[6] = m.motor_7;
        motor_targets[7] = m.motor_8;
        last_cmd_ms = millis();
        motors_set_all(motor_targets);
      }
    #endif
      break;

    case Envelope_indicator_cmd_tag:
    #if defined(HAS_INDICATOR_LED) && HAS_INDICATOR_LED
      {
        const IndicatorLightCommand &c = env.payload.indicator_cmd;

        float r = c.r;
        float g = c.g;
        float b = c.b;
        indicator_set_rgb(r, g, b);
      }
    #endif
      break;

    default:
      break;
  }
}


// ------ Setup ------
void setup() {
  pktserial.begin(BAUDRATE);
  pktserial.setPacketHandler(&onPacket);

  #if defined(HAS_MOTORS) && HAS_MOTORS
    motors_init();
  #endif

  #if defined(HAS_INDICATOR_LED) && HAS_INDICATOR_LED
    setup_neopixel();
  #endif 

  #if defined(HAS_POWER_SENSING) && HAS_POWER_SENSING
    power_sensing_init();
  #endif

  delay(50); // Wait for things to configure before starting loop
}


// ------ Main Loop ------
void loop() {
  static uint32_t last_telem_ms = 0;

  pktserial.update();

  const uint32_t now = millis();

  #if defined(HAS_MOTORS) && HAS_MOTORS
    // Safety: timeout kills motors
    if ((now - last_cmd_ms) > CMD_TIMEOUT_MS) {
      motors_safe_stop();
    }
  #endif

  // Telemetry send depending on active features
  if ((now - last_telem_ms) >= TELEMETRY_PERIOD_MS) {
    last_telem_ms = now;

    #if BOARD_PRESTO

      send_presto_state();

    #elif BOARD_SENSOR

      send_sensorb_state();

    #endif
  }
}
