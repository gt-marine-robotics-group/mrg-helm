from pathlib import Path
import time
import serial

from google.protobuf.message import DecodeError


from mrg_helm.pb.command_pb2 import Command
from mrg_helm.pb.init_pb2 import Init

class HelmDriver:
    """Driver for Helm Interface"""
    def __init__(self, 
                 port=Path('/tmp/mrg-helm'),
                 hz=10):

        self._frequency = 1.0 / hz

        self._device = serial.Serial(str(port), 115200, timeout=1)
        self._device_info = {}

        self._buffer = None
        self._connected = False

        # self.active_link = ControlLink.SERIAL
        # self.control_state = ControlState.MANUAL
        # self.active_state = ActiveState.STOPPED

    def _read(self):
        """Read data"""
        self._buffer = self._device.readline().strip()

    def _send(self, data):
        """Write data"""
        self._device.write(data)

    def connect(self):
        """Connect to helm interface"""
        while not self._connected:
            self._read()
            print(self._buffer)
            if self._buffer is not None:
                try:
                    msg = Init()
                    msg.ParseFromString(self._buffer)
                    self._device_info['version'] = msg.version
                    self._connected = True
                    print("Connected!")
                except DecodeError:
                    print(DecodeError)

    def command(self, commands):
        pass

