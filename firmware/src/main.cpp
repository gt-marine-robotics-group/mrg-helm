#include <Arduino.h>

#include <rc_input.h>
#include <pins.h>

#include "motors.h"
#include "globals.h"

#include "pb_encode.h"
#include "pb_decode.h"
#include "config.pb.h"

RCInput rcInput(g_servo5, g_servo2, g_servo3, g_servo4, g_servo1);


enum states {
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED
} state;

// Protobuf helper, nanopb expects a function for Config
static bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
  const char *s = (const char*)(*arg);
  if(!pb_encode_tag_for_field(stream,field)) {
    return false;
  }
  return pb_encode_string(stream, (const pb_byte_t*)s,strlen(s)); 
}

// Output System -> Teensy Serial
static bool serial_write(pb_ostream_t *s, const pb_byte_t *buf, size_t cnt) {
  size_t w = 0;
  while(w < cnt) {
    w += Serial.write(buf + w, cnt - w);
  }
  return true;
}

static void send_firmware_version() {
  Config msg = Config_init_default;
  msg.version.funcs.encode = &encode_string;
  msg.version.arg = (void*)"1.0.0"; // firmware version (for now)

  pb_ostream_t out = {serial_write, nullptr, SIZE_MAX, 0, nullptr};
  bool ok = pb_encode_delimited(&out, Config_fields, &msg);

  if (!ok) {
    Serial.println("Error encoding version message!");
  } else {
    Serial.println("Sent firmware version protobuf!");
  }

}

// Translate RC input to 2 motor system
void set_motor_2x() {
  int port = (g_rc_srg + g_rc_yaw);
  int stbd = (g_rc_srg - g_rc_yaw);
  float max_val = max(100, max(abs(port), abs(stbd))) / 100;
  g_rc_peff = port / max_val;
  g_rc_seff = stbd / max_val;
}

void exec_mode(int mode, bool killed) {
  // Vehicle Logic
  if (killed) {
    delay(1);
  } else {
    if (mode == RCInput::ControlState::autonomous) {  // AUTONOMOUS
      if (!g_armed) {
        set_arm(true);
        Serial.println("AUTONOMOUS - ARMING");
      }
      Serial.println("AUTONOMOUS");
      digitalWrite(RED_LED, HIGH);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
    } else if (mode == RCInput::ControlState::calibration) {  // CALIBRATION
      if (g_armed) {
        set_arm(false);
        Serial.println("CALIBRATION - DISARMING");
      }
      rcInput.check_calibration_ready();
      digitalWrite(RED_LED, HIGH);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
    } else if (mode == RCInput::ControlState::remote_control) {  // REMOTE CONTROL
      if (!g_armed) {
        set_arm(true);
        Serial.println("MANUAL - ARMING");
      }
      set_motor_2x();
      port_throttle = throttle_convert((float)g_rc_peff);
      stbd_throttle = throttle_convert((float)g_rc_seff);
      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
    }
  }
}


void setup() {
  // Set all LEDs to be output and on
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);  
  pinMode(YELLOW_LED, OUTPUT);
  digitalWrite(YELLOW_LED, HIGH);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, HIGH);

  Serial.begin(115200);

  send_firmware_version();

  g_servo1.attach();
  g_servo2.attach();
  g_servo3.attach();
  g_servo4.attach();
  g_servo5.attach();

  delay(2000);



  // Turn off red to indicate microros transports
  digitalWrite(GREEN_LED, LOW);  

  rcInput.calibrate();

  SPI.begin();
  pot.begin();

  pot.setValue(0, MCP_POT_MIDDLE_VALUE);
  pot.setValue(1, MCP_POT_MIDDLE_VALUE);
  
  

  delay(500);
  // Turn off yellow to indicate SPI, Pot, RC ready
  Serial.println("======= CALIBRATION COMPLETE - RC READY =======");
  digitalWrite(YELLOW_LED, LOW);

  // Turn off green to indicate ROS entities created
  digitalWrite(RED_LED, LOW);
}

void loop() {
  loop_time = millis();
 
  rcInput.read();
  g_rc_srg = rcInput.get_srg();
  g_rc_swy = rcInput.get_swy();
  g_rc_yaw = rcInput.get_yaw();

  exec_mode(rcInput.get_ctr_state(), false);
  
  set_motor_throttles();
}