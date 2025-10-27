#include <Arduino.h>
#include <PacketSerial.h>

#include <rc_input.h>
#include <pins.h>

#include "motors.h"
#include "globals.h"

#include "pb_encode.h"
#include "pb_decode.h"
#include "config.pb.h"
#include "command.pb.h"
#include "status.pb.h"

PacketSerial pktserial;

RCInput rcInput(g_servo5, g_servo2, g_servo3, g_servo4, g_servo1);

static size_t bytesRead = 0;

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
  msg.version.arg = (void*)"1.0.0"; // firmware version (for now) <= FIX THIS

  pb_ostream_t out = {serial_write, nullptr, SIZE_MAX, 0, nullptr};
  bool ok = pb_encode_delimited(&out, Config_fields, &msg);

  if(!ok) {
    // Serial.println("Error encoding version message!");
  } else {
    // Serial.println("Sent firmware version protobuf!");
  }
}

static inline uint32_t compute_status() {
  uint32_t state = 0;

  if(g_rc_kil || hardware_estop) {
    state = 0;
  } else {
    int ctr_state = rcInput.get_ctr_state();
    if(ctr_state == RCInput::ControlState::autonomous) {
      state = 2;
    } else if(ctr_state == RCInput::ControlState::remote_control) {
      state = 1;
    } else if (ctr_state == RCInput::ControlState::calibration) {
      state = 3;
    } else {
      state = 0;
    }
  }
  return state;
}

static void send_status(){
  Status msg = Status_init_default;
  // msg.control_state = compute_status();
  msg.control_state = 9;

  pb_ostream_t out = {serial_write,nullptr,SIZE_MAX,0,nullptr};

  if(!pb_encode_delimited(&out,Status_fields,&msg)){
    // Serial.println("Error encoding Status!");
  } else {
    // Serial.println("Status Sent!");
  }
}

// Decode effort
// static bool decode_effort(pb_istream_t *stream, const pb_field_t *field, void **arg) {
//   size_t *idx = (size_t*)(*arg);
  
//   uint64_t u = 0;
//   if(!pb_decode_varint(stream,&u)) {
//     return false;
//   }

//   int32_t value = (int32_t)u;
//   if(*idx < 2) {
//     g_efforts[*idx] = value;
//     (*idx)++;
//   }
//   g_ros_peff = g_efforts[0];
//   g_ros_seff = g_efforts[1];
//   return true;
// }

static bool serial_read(uint32_t timeout_ms = 100) {
  uint32_t start = millis();

  Command data = Command_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(g_buffer, sizeof(g_buffer));

  if (pb_decode(&stream, Command_fields, &data)) {
    g_ros_peff = data.port;
    g_ros_seff = data.stbd;
    return true;
  } else {
    // Serial.println("Error!");
    return false;
  }
  // Serial.println((String)data.port);
}

static void read_hardware_estop() {
  int pin_value = digitalRead(SERVO_6);

  if (pin_value == HIGH) {
    hardware_estop = true;
  } else {
    hardware_estop = false;
  }
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
        // Serial.println("AUTONOMOUS - ARMING");
      }
      port_throttle = throttle_convert((float)g_ros_peff);
      stbd_throttle = throttle_convert((float)g_ros_seff);
      // Serial.println("AUTONOMOUS");
      // Serial.println(port_throttle);
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
      digitalWrite(GREEN_LED, HIGH); // temporary
    }
  }
}

uint8_t buffer[128];

void onPacket(const uint8_t* buffer, size_t size) {
  Command cmd = Command_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(buffer, size);
  bool status = pb_decode(&stream, Command_fields, &cmd);

  // Serial.printf("Port effort = %d, Stbd effort = %d\n", g_ros_peff, g_ros_seff);
  if (!status) {
    g_ros_peff = cmd.port;
    g_ros_seff = cmd.stbd;
  } else {
    // Serial.println("Error!");
  }
  Serial.println("=======================");
  if(g_ready) {
    // send_status();
    Serial.println("========= STATUS ===");
  } else {
    // send_firmware_version();
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
  pinMode(SERVO_6,INPUT_PULLUP);

  // Serial.begin(115200);
  pktserial.begin(115200);
  pktserial.setPacketHandler(&onPacket);

  // 

  // Do we want to set a specific time that a status sent?
  // read_hardware_estop();
  // send_status();

  g_servo1.attach();
  g_servo2.attach();
  g_servo3.attach();
  g_servo4.attach();
  g_servo5.attach();

  delay(2000);



  // Turn off red to indicate microros transports
  digitalWrite(GREEN_LED, LOW);  

  // rcInput.calibrate();

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
  read_hardware_estop();
  pktserial.update();

  static size_t bytesRead = 0;

  // if(Serial.available()) {
  //   // g_buffer = Serial.read();
  //   update_buffer();
  //   g_ready = serial_read();
  //   Serial.printf("Port effort = %d, Stbd effort = %d\n", g_ros_peff, g_ros_seff);
  //   if(g_ready) {
  //     send_status();
  //   } else {
  //     send_firmware_version();
  //   }
  // }

  // rcInput.read();
  // g_rc_srg = rcInput.get_srg();
  // g_rc_swy = rcInput.get_swy();
  // g_rc_yaw = rcInput.get_yaw();
  // exec_mode(rcInput.get_ctr_state(), false);
  exec_mode(RCInput::ControlState::autonomous, false);
  set_motor_throttles();
  delay(50);  
}