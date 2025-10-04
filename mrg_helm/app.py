# Main app

import typer

from mrg_helm.utils.build import app as build_app
from mrg_helm.utils.udev import app as udev_app

from mrg_helm.run import app as run_app

from mrg_helm.sim.sim import app as sim_app

cli = typer.Typer(no_args_is_help=True)

cli.add_typer(build_app,
              name='firmware')

cli.add_typer(udev_app, 
              name='dev')

cli.add_typer(run_app, 
              name='run')

cli.add_typer(sim_app,
              name='sim')
