# ROS Mode

import threading
import time


import typer

ROS_IMPORT_SUCCESS = True
ROS_IMPORT_ERROR = None
try:
    import rclpy
    from rclpy.node import Node
    from std_msgs.msg import Int16, Float32MultiArray
    from geometry_msgs.msg import Twist, TwistStamped
except ImportError as e:
    ROS_IMPORT_SUCCESS = False
    ROS_IMPORT_ERROR = e

from mrg_helm.driver.serial import HelmDriver
from mrg_helm.driver import Topic, TargetPort



if ROS_IMPORT_SUCCESS:
    class RosHelmDriver(Node):
        def __init__(self, 
            topic = 'motor_commands',
            command_style = 'direct',
            port = '/tmp/mrg-helm',
            stamped = True
        ):
            super().__init__('mrg_helm')
            self.driver = HelmDriver(port=port)

            if command_style == 'direct':
                self._msg_type = Float32MultiArray
                cmd_cb = self._direct_cmd_cb

            self.cmd_sub = self.create_subscription(
                self._msg_type,
                topic,
                cmd_cb,
                10
            )

            self.status_pub = self.create_publisher(Int16, 'mrg_helm_status', 10)

            self.efforts = [0, 0]

            self.driver.connect()
            if not self.driver._connected:
                print('DRIVER NOT CONNECTED')
                typer.Exit()
            self.timer = self.create_timer(0.1, self._timer_cb)

        def _direct_cmd_cb(self, msg):
            efforts = msg.data
            self.efforts = [int(x * 100) for x in efforts]

        def _timer_cb(self):
            self.driver.command(self.efforts)
            control_state = self.driver.control_state
            status_msg = Int16()
            status_msg.data = int(control_state)
            self.status_pub.publish(status_msg)

        

    def ros(topic: Topic = 'motor_commands',
            port: TargetPort = '/tmp/mrg-helm',
            stamped: bool = True
    ):
        """ROS 2 Mode"""
        rclpy.init()

        ros_helm_driver = RosHelmDriver(topic=topic, port=port, stamped=stamped)
        
        rclpy.spin(ros_helm_driver)
        ros_helm_driver.destroy_node()
        rclpy.shutdown()

else:
    def ros(topic: Topic = 'motor_commands',
            port: TargetPort = '/tmp/mrg-helm',
            stamped: bool = True
    ):
        """ROS 2 Mode"""
        typer.echo(f'ROS 2 libraries not found, have you sourced the ROS 2 workspace?')
        typer.echo(f'{ROS_IMPORT_ERROR}')
        typer.echo(f'Try running `source /opt/ros/[version]/setup.bash`')