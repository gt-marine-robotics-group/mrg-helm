from enum import Enum, auto

from mrg_helm.common.helm import HelmControlState

class IndicatorState(Enum):
    OFF = auto()
    BLINK = auto()
    SOLID = auto()

class IndicatorType(Enum):
    LIGHT_RED = auto()
    LIGHT_YELLOW = auto()
    LIGHT_GREEN = auto()
    LIGHT_BLUE = auto()

class IndicatorMapping:
    def __init__(self, state, 
                 indicator_type: IndicatorType,
                 indicator_state: Tuple[IndicatorState, ...]):
        self.indicator_type = indicator_type
        self.indicator_state = indicator_state
