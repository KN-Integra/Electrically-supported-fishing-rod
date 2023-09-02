import kivy
kivy.require('2.2.1')
import os
import asyncio

from bleak import BleakScanner, BleakClient, BLEDevice
from repl import BluetoothDesktopClient

from kivy.app import App
from kivy.uix.label import Label

CMD_INIT_BYTES = b'\x00'
CMD_BOOT_BYTES = b'\x01'
CMD_SPEED_BYTES = b'\x02'
CMD_DRIVER_BYTES = b'\x03'
CMD_DEBUG_BYTES = b'\x04'
READ_CHARACTERISTIC_UUID = "00002a38-0000-1000-8000-00805f9b34fb"
WRITE_CHARACTRERISTIC_UUID ="00002a39-0000-1000-8000-00805f9b34fb"
script_dir = os.path.dirname(os.path.abspath(__file__))
HELP_PATH = os.path.join(script_dir, 'help.txt')


class FishingRodApp(App):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.BLEClient = BluetoothDesktopClient()

    def build(self):
        pass

    def on_start(self):
        pass

    def on_stop(self):
        pass

    def scan_for_devices(self):
        print("scanning")
        asyncio.run(self.BLEClient.cmd_scan())

    async def connect_auto(self):
        asyncio.run(self.BLEClient.autoconnect())



if __name__ == '__main__':
    FishingRodApp().run()
