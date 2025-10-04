from pathlib import Path
from subprocess import run
import sys

import typer

<<<<<<< HEAD:mrg_helm/utils/build.py
from mrg_helm.utils.protogen import build_protobuf
from mrg_helm.utils.pio import pio_build
=======
from autohelm.utils.protogen import build_protobuf
from autohelm.utils import pio
>>>>>>> refs/remotes/origin/v2.y:autohelm/utils/build.py

app = typer.Typer(
    help='Build firmware',
    no_args_is_help=True
)

@app.command('build')
def build():
    """Build firmware"""
    typer.echo(typer.style(
        '[mrg_helm] Building Protobuf Messages',
        fg=typer.colors.BLUE, bold=True))
    build_protobuf()
    typer.echo(typer.style(
        '[mrg_helm] Building PlatformIO Project',
         fg=typer.colors.BLUE, bold=True))
    pio.build()

@app.command('list')
def list():
    """List microcontrollers"""
    pio.list()

@app.command('flash')
def flash():
    """Flash"""
    pio.flash()