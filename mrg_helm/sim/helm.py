from pathlib import Path
import time

import vserial
from mrg_helm.pb.command_pb2 import Command
from mrg_helm.pb.config_pb2 import Config
from mrg_helm.pb.status_pb2 import Status
from mrg_helm.common.helm import ControlState, ControlLink, ActiveState, ControlLinkStatus
from mrg_helm.utils.version import get_short_version

class TemplateHelm:
    """Template for Helm Interface"""
    def __init__(self, name='mrg-helm',
                 directory=Path('/tmp'),
                 hz=10,
                 num_efforts=2):
        self.name = name
        self.port = directory / name
        self.frequency = 1.0 / hz
        self._num_efforts = num_efforts
        self.efforts = None


        self.vsd = vserial.VirtualSerialDevice(
            port = self.port,
            callback=self._read
        )

        self._running = False

        self.link_status = {
            ControlLink.SERIAL: ControlLinkStatus.DISCONNECTED,
            ControlLink.RC: ControlLinkStatus.DISCONNECTED
        }

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

        self._last_serial = time.time()

    def parse(self, data):
        msg = Command()
        print(f"received {data}")
        try:
            msg.ParseFromString(data)
            self.efforts = msg.efforts
            self._last_serial = time.time()
            self.link_status[ControlLink.SERIAL] = ControlLinkStatus.ACTIVE
        except:
            pass

    def setup(self):
        print('Controller Calibration')
        
    def _transmit_config(self):
        """Transmit config"""
        msg = Config()
        msg.version = get_short_version()
        data = msg.SerializeToString()
        self._send(data)

    def _transmit_status(self):
        """Transmit status"""
        msg = Status()
        msg.control_state = 1
        data = msg.SerializeToString()
        self._send(data)

    def loop(self):
        loop_time = time.time()

        if loop_time - self._last_serial > 5:
            if self.link_status[ControlLink.SERIAL] == ControlLinkStatus.DISCONNECTED:
                self._transmit_config()
        else:
            self._transmit_status()
        time.sleep(self.frequency)