import typer

from mrg_helm.driver.serial import HelmDriver

app = typer.Typer(
    help='Run mrg_helm Driver'
)


@app.command('send')
def send():
    hd = HelmDriver()
    hd.connect()