from pathlib import Path
import time

import vserial
from mrg_helm.pb.command_pb2 import Command
from mrg_helm.pb.init_pb2 import Init
from mrg_helm.common.helm import ControlState, ControlLink, ActiveState
from mrg_helm.utils.version import get_short_version

class TemplateHelm:
    """Template for Helm Interface"""
    def __init__(self, name='mrg-helm',
                 directory=Path('/tmp'),
                 hz=10):
        self.name = name
        self.port = directory / name
        self.frequency = 1.0 / hz

        self.vsd = vserial.VirtualSerialDevice(
            port = self.port,
            callback=self._read
        )

        self._running = False

        self.active_link = ControlLink.SERIAL
        self.control_state = ControlState.MANUAL
        self.active_state = ActiveState.STOPPED

    def _read(self, data):
        """Read data"""
        self.parse(data)

    def _send(self, data):
        """Write data"""
        self.vsd.send(data)

    def start(self):
        """Start simulated helm interface"""
        self.vsd.open()
        self._running = True
        self.setup()

        try:
            while self._running:
                self.loop()
                time.sleep(self.frequency)
        except:
            self.close()
            raise

    def close(self):
        """Close simulated helm interface"""
        self._running = False
        self.vsd.close()

    def parse(self, data):
        """Parse data"""
        raise NotImplementedError

    def setup(self):
        """Simulate setup"""
        raise NotImplementedError

    def loop(self):
        """Simulate loop"""
        raise NotImplementedError


class SimHelm(TemplateHelm):
    """Simulated Helm Interface"""
    def __init__(self, name='mrg-helm', 
                 directory=Path('/tmp'),
                 hz=10):
        super().__init__(name, directory, hz)

    def parse(self, data):
        print(data)
        # msg = Command()
        # msg.ParseFromString(data)
        # print(msg)

    def setup(self):
        print('Controller Calibration')
        print('Initialize connection')
        
    def _initialize(self):
        msg = Init()
        msg.version = get_short_version()
        data = msg.SerializeToString()
        self._send(data)

    def loop(self):
        self._initialize()