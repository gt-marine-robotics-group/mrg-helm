# ROS Mode

import threading
import time
from typing_extensions import Annotated

import typer

try:
    import rclpy
    from rclpy.node import Node
    from geometry_msgs.msg import Twist, TwistStamped
except ImportError:
    raise


Topic = Annotated[
    str,
    typer.Option('--topic', '-t', help='Topic to publish to.')
]

class RosHelmDriver(Node):

    def __init__(self, topic='twist'):
        super().__init__('override_teleop_twist')
        self.publisher_ = self.create_publisher(Twist, topic, 10)

    def publish(self, values):
        msg = Twist()
        for name, value in values.items():
            if name == 'surge':
                msg.linear.x = value.value
            elif name == 'sway':
                msg.linear.y = value.value
            elif name == 'heave':
                msg.linear.z = value.value
            elif name == 'roll':
                msg.angular.x = value.value
            elif name == 'pitch':
                msg.angular.y = value.value
            elif name == 'yaw':
                msg.angular.z = value.value
        self.publisher_.publish(msg)

app = typer.Typer()

@app.command()
def ros(keybind: Keybind = KeybindConfigs.sixaxis,
        topic: Topic = 'twist'):
    """ROS 2 Mode"""
    rclpy.init()

    twist_pub = TwistPub(topic)
    callback = twist_pub.publish

    t = threading.Thread(target=osk, args=(keybind, callback), daemon=True)
    t.start()

    while True:
        if not t.is_alive():
            break
        time.sleep(1)
    