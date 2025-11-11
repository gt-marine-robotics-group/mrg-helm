from pathlib import Path
import serial

from cobs import cobs
from cobs.cobs import DecodeError as CobsDecodeError

from google.protobuf.message import DecodeError as ProtobufDecodeError

from mrg_helm.pb.robosub_pb2 import (
    MotorCommand,
    IndicatorLightCommand,
    PrestoState,
    SensorBState,
    Envelope,
    BOARD_JETSON_ID,
)


class HelmDriver:
    """Driver for Helm Interface"""
    def __init__(self, 
                 port: Path = Path('/tmp/mrg-helm'),
                 hz: int = 10,
                 baud: int = 115200,
                 rx_timeout : float = 0.05,
    ) -> None:

        self._period = 1.0 / hz

        self._device = serial.Serial(str(port), baud, timeout=rx_timeout)
        self._device_info = {}

        self._buffer: bytes | None = None
        self._connected: bool = self._device.is_open

    def _read(self) -> bytes | None:
        """Read data"""
        frame = self._device.read_until(b'\x00')

        if not frame:
            return None
        frame = frame[:-1]  # Remove 0x00

        try:
            data = cobs.decode(frame)
            return data
        except CobsDecodeError:
            return None

    def _send(self, data: bytes) -> None:
        """Write data"""
        encoded = cobs.encode(data)
        self._device.write(encoded + b'\x00')

    def _send_envelope(self, envelope: Envelope) -> None:
        """Serialize and send an Envelope message"""    
        self._send(envelope.SerializeToString())

    def send_motor_command(self, motors8: list[float]) -> None:
        if len(motors8) != 8:
            raise ValueError("MotorCommand requires exactly 8 values")
        
        env = Envelope()
        env.header.src = BOARD_JETSON_ID

        msg: MotorCommand = env.motor_cmd
        (
            msg.motor_1, 
            msg.motor_2, 
            msg.motor_3, 
            msg.motor_4,
            msg.motor_5, 
            msg.motor_6, 
            msg.motor_7, 
            msg.motor_8
        ) = motors8

        self._send_envelope(env)
        
    def send_indicator_led_command(self, r: float, g: float, b: float) -> None:

        env = Envelope()
        env.header.src = BOARD_JETSON_ID

        msg: IndicatorLightCommand = env.indicator_cmd
        msg.r = r
        msg.g = g
        msg.b = b

        self._send_envelope(env)

    def _read_envelope_once(self) -> Envelope | None:
        """Try to read one Envelope from the wire; return msg or None"""

        data = self._read()
        if not data:
            return None
        
        env = Envelope()
        try:
            env.ParseFromString(data)
            return env
        except ProtobufDecodeError:
            return None

    def read_presto_state_once(self) -> PrestoState | None:
        """Try to read one PrestoState from the wire; return msg or None."""

        env = self._read_envelope_once()
        if not env:
            return None

        if env.WhichOneof("payload") == "presto_state":  # How we can detect what kind of data is coming in
            return env.presto_state
        return None

    def read_sensorb_state_once(self) -> SensorBState | None:
        """Read one SensorBState from the wire, if present."""
        env = self._read_envelope_once()
        if not env:
            return None

        if env.WhichOneof("payload") == "sensorb_state":
            return env.sensorb_state
        return None

    def close(self) -> None:
        try:
            self._device.close()
        except Exception:
            pass
