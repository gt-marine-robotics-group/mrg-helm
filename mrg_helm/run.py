import typer

from mrg_helm.comm.serial import send as ser_send

app = typer.Typer(
    help='Run mrg_helm Driver'
)

@app.command('send')
def send():
    ser_send()