from pathlib import Path
import time
import serial


from cobs import cobs
from cobs.cobs import DecodeError as CobsDecodeError

from google.protobuf.message import DecodeError as ProtobufDecodeError


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

        self.control_state = -999

        # self.active_link = ControlLink.SERIAL
        # self.control_state = ControlState.MANUAL
        # self.active_state = ActiveState.STOPPED

    def _read(self):
        """Read data"""
        self._buffer = self._device.read_until(b'\x00')

    def _send(self, data):
        """Write data"""
        encoded = cobs.encode(data)
        self._device.write(encoded + b'\x00')

    def connect(self):
        """Connect to helm interface"""
        while not self._connected:
            self._read()
            decoded = False
            if self._buffer is not None:
                data, decoded = self._decode()
                try:
                    msg = Config()
                    msg.ParseFromString(data)
                    self._device_info['version'] = msg.version
                    self._connected = True
                    print("Connected!")
                except ProtobufDecodeError:
                    print(f'Connect Received {data} {decoded} but could not parse.')

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
        # print('HELLO ' + str(data))
        # print('[MRG-HELM] SENDING', commands[0], commands[1])
        print('[DRIVER] Sent', data)
        self._send(data)

        decoded = False
        try:
            self._read()
            data, decoded = self._decode()
            msg = Status()
            msg.control_state = -1
            # msg.port = commands[0]
            # msg.stbd = commands[1]
            # sample = msg.SerializeToString()
            # print(f'[DRIVER] Sample: {sample}')
            # print(f'[DRIVER] Incoming: {data}')
            msg.ParseFromString(data)
            self.control_state = msg.control_state
            print(f'[DRIVER] Control state {self.control_state}')
            print(f'[DRIVER] Port: {msg.port} | Stbd: {msg.stbd}')
            self._buffer = None
            self._device.reset_input_buffer()
        except ProtobufDecodeError:
            print(f'[DRIVER] Command Received {data} {decoded} but could not parse.')

    def _decode(self):
        decoded = False
        
        # frames = [f for f in self._buffer.split(b'\x00') if f] 
       
        frame = b''
        if len(self._buffer) > 0:
            frame = self._buffer[0:(len(self._buffer)-1)]
       
        try:
            data = cobs.decode(frame)[1:]
            print(f'[DRIVER] Decoded data: {data}')
            decoded = True
        except CobsDecodeError as e:
            print(e)
            data = frame

        return data, decoded
