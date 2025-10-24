import typer

from mrg_helm.driver.serial import HelmDriver
from mrg_helm.driver.ros import ros
from mrg_helm.driver import Topic, TargetPort

cli = typer.Typer(
    help='Run mrg_helm Driver'
)


@cli.command('ros2')
def run_ros_driver(
    topic: Topic = 'motor_commands',
    port: TargetPort = '/tmp/mrg-helm',
    stamped: bool = True
):
    ros(topic, port, stamped)
