import typer
import signal

from mrg_helm.sim.mockhelm import MockHelm

app = typer.Typer(
    help='Run mrg_helm Simulation'
)

@app.command('mock')
def mock():
    mh = MockHelm()
    mh.start()

    try:
        signal.pause()
    except KeyboardInterrupt:
        mh.close()