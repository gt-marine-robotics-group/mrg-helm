import typer

from mrg_helm.driver.serial import HelmDriver
from mrg_helm.driver.ros import ros

cli = typer.Typer(
    help='Run mrg_helm Driver'
)


@cli.command('ros2')
def run_ros_driver():
    ros()
