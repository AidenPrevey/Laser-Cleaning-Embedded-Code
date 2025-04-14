from abc import ABC, abstractmethod
from dataclasses import dataclass
import serial
import struct
from typing import Tuple

class Transmitter:
    def __init__(self, port: str, baud_rate: int, write_timeout: float, timeout: float):
        self.serial = serial.Serial(port, baud_rate, write_timeout=write_timeout, timeout=timeout)
    
    def send_msg(self, msg: "Message"):
        sentinel = b"\xA5"
        header = struct.pack("<BI", msg.message_id(), msg.length())
        self.serial.write(sentinel + header + msg.encode())
        
        
@dataclass
class Message(ABC):
    @staticmethod
    @abstractmethod
    def message_id() -> int:
        ...
    
    @abstractmethod
    def length(self) -> int:
        ...
    
    @abstractmethod
    def encode(self) -> bytes:
        ...
        
@dataclass
class ShutdownMessage(Message):
    
    @staticmethod
    def message_id() -> int:
        return 0x00
    
    def length(self) -> int:
        return 4
    
    def encode(self) -> bytes:
        return struct.pack("i", 0)
    
@dataclass
class CommandMessage(Message):
    Command: str
        
    @staticmethod
    def message_id() -> int:
        return 0x01
    
    def length(self) -> int:
        return 8*5
    
    def encode(self) -> bytes:
        data = struct.pack(f"<{self.length()}s", self.Command.encode('utf-8'))
        return data
    
if __name__ == "__main__":
    # Example usage
    transmitter = Transmitter(port="COM9", baud_rate=115200, write_timeout=1, timeout=1)
    
    general_msg = CommandMessage("G0 X1")
    transmitter.send_msg(general_msg)
    
    while True:
        try:
            data = transmitter.serial.read_until("\r")
            if data:
                print("Received:", data)
        except serial.SerialTimeoutException:
            print("Timeout occurred while reading from the serial port.")
        except Exception as e:
            print(f"An error occurred: {e}")
            break
    