# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Sebastian Soczawa
#

import asyncio
import os
from bleak import BleakScanner, BleakClient, BLEDevice, BleakGATTCharacteristic

CMD_INIT_BYTES = b'\x00'
CMD_BOOT_BYTES = b'\x01'
CMD_SPEED_BYTES = b'\x02'
CMD_DRIVER_BYTES = b'\x03'
CMD_DEBUG_BYTES = b'\x04'
READ_CHARACTERISTIC_UUID = "00002a38-0000-1000-8000-00805f9b34fb"
WRITE_CHARACTRERISTIC_UUID ="00002a39-0000-1000-8000-00805f9b34fb"

script_dir = os.path.dirname(os.path.abspath(__file__))
HELP_PATH = os.path.join(script_dir, 'help.txt')

class BluetoothDesktopClient():

    def __init__(self) -> None:
        self.running = False
        self.scanner:BleakScanner = BleakScanner()
        self.wyndka_client:BleakClient = None
        self.wyndka:BLEDevice = None
        try:
            with open(HELP_PATH, 'r') as file:
                self.HELP = file.read()
        except FileNotFoundError:
            print("cannot locate help.txt")

    def check_connection(self):
        if self.wyndka_client:
            return self.wyndka_client.is_connected
        else:
            return False

    async def cmd_scan(self) -> None:
        print("Scanning for devices...")
        devices = await self.scanner.discover(timeout=5, return_adv=False)
        for device in devices:
            print(device.address, device.name)

    async def autoconnect(self) -> None:
        self.wyndka = await self.scanner.find_device_by_name("Wyndka", 5)
        if self.wyndka:
            await self.cmd_connect(self.wyndka.address)
            print("connection state:",self.check_connection())
        else:
            print("Failed to find Wyndka, make sure that the device is powered on\n\
                  If it is, then try to run scan, and connect using the proper mac address")

    async def cmd_connect(self, addr: str) -> bool:
        print("connecting...")
        self.wyndka_client = BleakClient(addr)
        if self.check_connection():
            print("device already connected, disconnect first")
            return True
        try:
            await self.wyndka_client.connect()
            return True
        except Exception as e:
            print(e)
            return False

    def cmd_help(self) -> None:
        print(self.HELP)

    async def cmd_init(self) -> None:
        print("init")
        try:
            cmd = CMD_INIT_BYTES
            await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd, response=False)
        except Exception as e:
            print(f"init cmd: {e}")

    async def cmd_speed_get(self) -> None:
        try:
            cmd = CMD_SPEED_BYTES
            await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd, response=False)
            response = await self.wyndka_client.read_gatt_char(READ_CHARACTERISTIC_UUID)
            print("Current speed: ", int.from_bytes(response[0:4], byteorder='little'))
        except Exception as e:
            print(e)

    async def cmd_driver(self) -> None:
        cmd = CMD_DRIVER_BYTES
        await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd, response=False)
        response = await self.wyndka_client.read_gatt_char(READ_CHARACTERISTIC_UUID)
        print("Current Driver Version: ", response[0], '.', response[1], sep='')

    async def cmd_speed_set(self, speed_val: int):
        print("setting speed...")
        try:
            cmd = bytearray(CMD_SPEED_BYTES + speed_val.to_bytes(4, byteorder='big', signed=False))
            await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd, response=False)
        except Exception as e:
            print(e)
        print("Done")

    async def cmd_boot(self):
        print("boot")
        try:
            cmd = CMD_BOOT_BYTES
            await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd)
        except Exception as e:
            # ignore error as connection will be broken after switching to bootloader
            pass
        self.wyndka_client = None

    async def cmd_disconnect(self):
        print("disconnecting...")
        await self.wyndka_client.disconnect()
        self.wyndka_client = None
        print("disconnected")

    async def start_repl(self):
        self.running = True

        print("Welcome to the Wyndka repl program!\nType 'help' for more information")
        try:
            while self.running:
                try:
                    _in = input("|| wyndka || >> ")
                    cmd = _in.split(' ')
                    if cmd[0] == "scan":
                        await self.cmd_scan()

                    elif cmd[0] == "connect":
                        if cmd[1] == "auto":

                            await self.autoconnect()
                        elif cmd[1] == "check":

                            print(self.check_connection())
                        else:
                            await self.cmd_connect(cmd[1])

                    elif cmd[0] == "help":
                        self.cmd_help()

                    elif cmd[0] == "exit":
                            print("Closing repl.")
                            if self.check_connection():
                                await self.cmd_disconnect()
                            return

                    elif self.check_connection():
                        if cmd[0] == "init":
                            await self.cmd_init()

                        elif cmd[0] == "speed":
                            if cmd[1] == "get":
                                await self.cmd_speed_get()

                            elif cmd[1] == "set":
                                await self.cmd_speed_set(int(cmd[2]))

                        elif cmd[0] == "boot":
                            await self.cmd_boot()

                        elif cmd[0] == "driver":
                            await self.cmd_driver()

                        elif cmd[0] == "disconnect":
                            await self.cmd_disconnect()
                        
                    else:
                        print("Invalid command or state. Please type: 'help' for help")
                except Exception as e:
                    print(f"Error during input: {e}")

        except  KeyboardInterrupt as e:
            print("\nClosing repl.")
            if self.check_connection():
                await self.cmd_disconnect()
            return


if __name__ == "__main__":
    client = BluetoothDesktopClient()

    try:
        asyncio.run(client.start_repl())
    except KeyboardInterrupt as e:
        print("Please exit the program using the exit command next time")
