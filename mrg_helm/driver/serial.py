from pathlib import Path
import time
import serial

from google.protobuf.message import DecodeError


from mrg_helm.pb.command_pb2 import Command
from mrg_helm.pb.config_pb2 import Config
from mrg_helm.pb.status_pb2 import Status

class HelmDriver:
    """Driver for Helm Interface"""
    def __init__(self, 
                 port=Path('/tmp/mrg-helm'),
                 hz=10,
                 efforts=2):

        self._frequency = 1.0 / hz
        self._efforts = efforts

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
        print(self._buffer)

    def _send(self, data):
        """Write data"""
        self._device.write(data)

    def connect(self):
        """Connect to helm interface"""
        while not self._connected:
            self._read()
            if self._buffer is not None:
                try:
                    msg = Config()
                    msg.ParseFromString(self._buffer)
                    self._device_info['version'] = msg.version
                    self.command([0 for _ in range(self._efforts)])
                    self._read()
                    msg = Status()
                    msg.ParseFromString(self._buffer)
                    if msg.control_state == 1:
                        self._connected = True
                        print("Connected!")
                except DecodeError:
                    print(f'Received {self._buffer} but could not parse.')

        

    def command(self, commands):
        msg = Command()
        msg.efforts.extend(commands)
        data = msg.SerializeToString()
        self._send(data)

