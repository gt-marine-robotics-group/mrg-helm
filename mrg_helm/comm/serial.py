import serial
from mrg_helm.pb.command_pb2 import Command

def send():
    ser = serial.Serial('/tmp/mockhelm', 115200, timeout=1)
    cmd = Command()
    cmd.efforts.extend([0, 0, 1, 2])

    data = cmd.SerializeToString()
    length = len(data).to_bytes(2, 'big')
    ser.write(data)
    # ser.write(length + data)
