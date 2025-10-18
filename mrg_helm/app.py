# Main app

import typer

from mrg_helm.utils.build import app as build_app
from mrg_helm.utils.udev import app as udev_app
from mrg_helm.utils.version import get_short_version

from mrg_helm.driver.app import cli as driver_cli

from mrg_helm.sim.sim import app as sim_app

cli = typer.Typer(no_args_is_help=True)

@cli.callback(invoke_without_command=True)
def main(
    version: bool = typer.Option(False, "--version", help="Show the version")
):
    if version:
        print(f"Version: {get_short_version()}")
        raise typer.Exit()

cli.add_typer(build_app,
              name='firmware')

cli.add_typer(udev_app, 
              name='dev')

cli.add_typer(driver_cli, 
              name='run')

cli.add_typer(sim_app,
              name='sim')
