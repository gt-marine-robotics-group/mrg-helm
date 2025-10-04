import typer
import signal

from mrg_helm.sim.mockhelm import MockHelm

app = typer.Typer(
<<<<<<< HEAD:mrg_helm/sim/sim.py
    help='Run mrg_helm Simulation'
=======
    help='Run Autohelm Simulation',
    no_args_is_help=True
>>>>>>> refs/remotes/origin/v2.y:autohelm/sim/sim.py
)

@app.command('mock')
def mock():
    mh = MockHelm()
    mh.start()

    try:
        signal.pause()
    except KeyboardInterrupt:
        mh.close()