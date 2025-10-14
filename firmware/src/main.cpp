#include <Arduino.h>

#include <rc_input.h>
#include <pins.h>

#include "motors.h"
#include "globals.h"

#include "pb_encode.h"
#include "pb_decode.h"
#include "config.pb.h"
#include "command.pb.h"
#include "status.pb.h"

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

  if(!ok) {
    Serial.println("Error encoding version message!");
  } else {
    Serial.println("Sent firmware version protobuf!");
  }
}

static inline uint32_t compute_status() {
  uint32_t state = 0;

  if(g_rc_kil) {
    state = 0;
  } else {
    int ctr_state = rcInput.get_ctr_state();
    if(ctr_state == RCInput::ControlState::autonomous) {
      state = 2;
    } else if(ctr_state == RCInput::ControlState::remote_control) {
      state = 1;
    }
    else {
      state = 0;
    }
  }
  return state;
}

static void send_status(){
  Status msg = Status_init_default;
  msg.control_state = compute_status();

  pb_ostream_t out = {serial_write,nullptr,SIZE_MAX,0,nullptr};

  if(!pb_encode_delimited(&out,Status_fields,&msg)){
    Serial.println("Error encoding Status!");
  } else {
    Serial.println("Status Sent!");
  }
}

// Decode effort
static bool decode_effort(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  size_t *idx = (size_t*)(*arg);
  
  uint64_t u = 0;
  if(!pb_decode_varint(stream,&u)) {
    return false;
  }

  int32_t value = (int32_t)u;
  if(*idx < 2) {
    g_efforts[*idx] = value;
    (*idx)++;
  }
  g_ros_peff = g_efforts[0];
  g_ros_seff = g_efforts[1];
  return true;
}

static bool serial_read(uint32_t timeout_ms = 100) {
  uint32_t start = millis();

  // Read the varint, the length of the message
  uint32_t length = 0;
  uint32_t shift = 0;
  while(true) {
    if((millis() - start) > timeout_ms) {
      return false;
    }
    if(Serial.available()) {
      uint8_t byte = Serial.read();
      length |= (uint32_t)(byte & 0x7F) << shift;
      if(!(byte & 0x80)) {
        break;
      }
      shift += 7;
    }
  }

  // Read the actual message
  uint8_t payload[length];
  size_t got = 0;
  start = millis();
  while(got < length && (millis() - start) < timeout_ms) {
    if(Serial.available()) {
      payload[got++] = Serial.read();
    }
  }
  if(got < length) {
    return false;
  }

  // Decode the message
  Command cmd = Command_init_zero;
  size_t idx = 0;
  cmd.efforts.funcs.decode = &decode_effort;
  cmd.efforts.arg = &idx;

  pb_istream_t stream = pb_istream_from_buffer(payload,length);
  if(!pb_decode(&stream,Command_fields,&cmd)) {
    Serial.println("Decode Failed");
    return false;
  }

  Serial.printf("Received efforts: %ld, %ld\n", (long)g_efforts[0], (long)g_efforts[1]);
  return true;
}

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
      port_throttle = throttle_convert((float)g_ros_peff);
      stbd_throttle = throttle_convert((float)g_ros_seff);
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

  // Do we want to set a specific time that a status sent?
  send_status();

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

  if(Serial.available()) {
    serial_read();
    Serial.printf("Port effort = %d, Stbd effort = %d\n", g_ros_peff, g_ros_seff);
  }
 
  rcInput.read();
  g_rc_srg = rcInput.get_srg();
  g_rc_swy = rcInput.get_swy();
  g_rc_yaw = rcInput.get_yaw();

  exec_mode(rcInput.get_ctr_state(), false);
  
  set_motor_throttles();
}