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

        self._device = serial.Serial(str(port), 115200, timeout=0.05)
        self._device_info = {}

        self._buffer = None
        self._connected = False

        self.control_state = None

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
                    self._connected = True
                    print("Connected!")
                except DecodeError:
                    print(f'Received {self._buffer} but could not parse.')

    def command(self, commands):
        msg = Command()
        # msg.efforts.extend(commands)
        if len(commands) < 2:
            msg.port = 0
            msg.stbd = 0
        else:
            msg.port = commands[0]
            msg.stbd = commands[1]
        data = msg.SerializeToString()
        print('HELLO ' + str(data))
        self._send(data)

        try:
            self._read()
            msg = Status()
            msg.ParseFromString(self._buffer)
            self.control_state = msg.control_state
        except:
            print(f'Received {self._buffer} but could not parse.')