from enum import Enum, auto

class ControlLink(Enum):
    RC = auto() # via onboard RC servo pins
    SERIAL = auto() # via serial connection

class ControlState(Enum):
    MANUAL = auto()
    AUTONOMOUS = auto()

class ActiveState(Enum):
    ACTIVE = auto() # Movement enabled
    STOPPED = auto() # Movement disabled by software
    KILLED = auto() # Movement disabled by hardware

