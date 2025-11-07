from typing_extensions import Annotated

import typer

Topic = Annotated[
    str,
    typer.Option('--topic', '-t', help='Topic to subscribe to.')
]

TargetPort = Annotated[
    str,
    typer.Option('--port', '-p', help='Serial port.')
]