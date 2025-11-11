# ROS Mode
from pathlib import Path
from mrg_helm.driver.serial import HelmDriver
from typing import Optional

import typer

ROS_IMPORT_SUCCESS = True
ROS_IMPORT_ERROR = None
try:
    import rclpy
    from rclpy.node import Node
    from std_msgs.msg import Bool, ColorRGBA, Float32, Float32MultiArray
except ImportError as e:
    ROS_IMPORT_SUCCESS = False
    ROS_IMPORT_ERROR = e


if ROS_IMPORT_SUCCESS:
    class RosHelmDriver(Node):
        def __init__(self,
                     presto_port: str = "/dev/ttyUSB0",  #/tmp/ttyUSB-presto  # TODO: Setup udev rules on Jetson 
                     sensorb_port: str = "/dev/ttyUSB1", #/tmp/ttyUSB-sensorb # @SeanFish are you setting up some alias when you set "/tmp/mrg-helm" 
                     hz: int = 20
        ) -> None:

            super().__init__('mrg_helm')

            self.presto_driver: Optional[HelmDriver] = None
            self.sensorb_driver: Optional[HelmDriver] = None

            try:
                self.presto_driver = HelmDriver(Path(presto_port), hz=hz)
            except Exception as e:
                self.get_logger().warn(
                    f"Could not open presto port {presto_port}: {e}")

            try:
                self.sensorb_driver = HelmDriver(Path(sensorb_port), hz=hz)
            except Exception as e:
                self.get_logger().warn(
                    f"Could not open sensor board port {sensorb_port}: {e}")

            # ------ ROS Subscriptions ------
            self.cmd_motors_sub = self.create_subscription(
                Float32MultiArray,
                "/pontus/thruster_cmds",
                self._motors_callback,
                10
            )

            self.cmd_indicator_led_sub = self.create_subscription(
                ColorRGBA,
                "/pontus/indicator_led_color",
                self._indicator_led_callback,
                10
            )

            self._last_led_color = None

            # ------ ROS Publishers ------
            self.estop_pub = self.create_publisher(
                Bool,
                "/pontus/e_stop",
                10
            )

            self.autonomy_switch_pub = self.create_publisher(
                Bool,
                "/pontus/autonomy_switch",
                10
            )

            self.depth_sensor_pub = self.create_publisher(
                Float32,
                "/pontus/depth_sensor",
                10
            )

            self.voltage_pub = self.create_publisher(
                Float32,
                "/pontus/power/voltage",
                10
            )

            self.current_pub = self.create_publisher(
                Float32,
                "/pontus/power/current",
                10
            )

            # ------ Timers ------
            self.telemetry_timer = self.create_timer(
                1.0 / hz, 
                self._telemetry_timer_callback
            )


        # --------- Callbacks ---------

        def _motors_callback(self, msg: Float32MultiArray) -> None:
            vals = list(msg.data)

            if self.presto_driver and self.presto_driver._connected:
                try:
                    self.presto_driver.send_motor_command(vals)
                except Exception as e:
                    self.get_logger().warn(f"send_motor_command failed: {e}")

        def _indicator_led_callback(self, msg: ColorRGBA) -> None:
            if self._last_led_color == msg:
                # Don't send color if the same as previous message
                return 

            self._last_led_color = msg
            
            if self.presto_driver and self.presto_driver._connected:
                try:
                    self.presto_driver.send_indicator_led_command(
                        msg.r, msg.g, msg.b
                    )
                except Exception as e:
                    self.get_logger().warn(
                        f"send_indicator_led_command failed: {e}"
                    )

        def _telemetry_timer_callback(self) -> None:
            # ------ Presto Board Telemetry ------
            if self.presto_driver and self.presto_driver._connected:
                state = self.presto_driver.read_presto_state_once()
                if state:
                    e_stop_msg = Bool()
                    e_stop_msg.data = bool(state.e_stop)
                    self.estop_pub.publish(e_stop_msg)

                    autonomy_switch_msg = Bool()
                    autonomy_switch_msg.data = bool(state.autonomy_switch)
                    self.autonomy_switch_pub.publish(autonomy_switch_msg)

            # ------ Sensor Board Telemetry ------
            if self.sensorb_driver and self.sensorb_driver._connected:
                state = self.sensorb_driver.read_sensorb_state_once()
                if state:
                    depth_msg = Float32()
                    depth_msg.data = state.pressure_pa
                    self.depth_sensor_pub.publish(depth_msg)

                    voltage_msg = Float32()
                    voltage_msg.data = state.voltage_v
                    self.voltage_pub.publish(voltage_msg)

                    current_msg = Float32()
                    current_msg.data = state.current_a
                    self.current_pub.publish(current_msg)

    def ros(presto_port: str = "/tmp/ttyUSB-presto",
            sensorb_port: str = "/tmp/ttyUSB-sensorb",
            hz: int = 20
            ) -> None:
        """ROS 2 Mode"""
        rclpy.init()

        node = RosHelmDriver(presto_port=presto_port,
                             sensor_port=sensorb_port, 
                             hz=hz)

        try:
            rclpy.spin(node)
        finally:
            node.destroy_node()
            rclpy.shutdown()
else:
    def ros(presto_port: str = "/tmp/ttyUSB-presto",
            sensor_port: str = "/tmp/ttyUSB-sensor",
            hz: int = 20) -> None:
        typer.echo(
            "ROS 2 libraries not found. Have you sourced your ROS 2 setup?")
        typer.echo(f"{ROS_IMPORT_ERROR}")
