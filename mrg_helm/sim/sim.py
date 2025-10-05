import typer
import signal

from mrg_helm.sim.helm import SimHelm

app = typer.Typer(
    help='Run mrg_helm Simulation'
)

@app.command('sim')
def sim():
    mh = SimHelm()
    mh.start()

    try:
        signal.pause()
    except KeyboardInterrupt:
        print('Interrupt')
    finally:
        mh.close()