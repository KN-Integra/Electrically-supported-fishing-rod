import asyncio
import os
from bleak import BleakScanner, BleakClient, BLEDevice, BleakGATTCharacteristic

CMD_INIT_BYTES = b"\x00"
CMD_BOOT_BYTES = b"\x01"
CMD_SPEED_BYTES = b"\x02"
CMD_DRIVER_BYTES = b"\x03"
CMD_DEBUG_BYTES = b"\x04"
READ_CHARACTERISTIC_UUID = "00002a38-0000-1000-8000-00805f9b34fb"
WRITE_CHARACTRERISTIC_UUID = "00002a39-0000-1000-8000-00805f9b34fb"


class BluetoothClient:

    def __init__(self) -> None:
        self.scanner: BleakScanner = BleakScanner()
        self.wyndka_client: BleakClient = None
        self.wyndka: BLEDevice = None

    def check_connection(self) -> bool:
        if self.wyndka_client:
            return self.wyndka_client.is_connected
        else:
            return False

    async def cmd_scan(self) -> any:
        print("Scanning for devices...")
        devices = await self.scanner.discover(timeout=5, return_adv=False)
        return devices

    async def cmd_autoconnect(self) -> bool:
        self.wyndka = await self.scanner.find_device_by_name("Wyndka", 5)
        if self.wyndka:
            await self.cmd_connect(self.wyndka.address)
            print("connection state:", self.check_connection())
            return self.check_connection()
        else:
            print(
                "Failed to find Wyndka, make sure that the device is powered on\n\
                  If it is, then try to run scan, and connect using the proper mac address"
            )
        return False

    async def cmd_connect(self, addr: str) -> bool:
        print("connecting...")
        self.wyndka_client = BleakClient(addr)
        # there is a problem in bleak android implementation:
        # checking connection before any attempt to connect causes an error.
        # this should be uncommented when working with repl
        # -------------------
        # if self.check_connection():
        #     print("device already connected, disconnect first")
        #     return True
        # -------------------
        try:
            await self.wyndka_client.connect()
            return self.check_connection()
        except Exception as e:
            print(e)
            return False

    async def cmd_init(self) -> None:
        print("init")
        try:
            cmd = CMD_INIT_BYTES
            await self.wyndka_client.write_gatt_char(
                WRITE_CHARACTRERISTIC_UUID, cmd, response=False
            )
        except Exception as e:
            print(f"init cmd: {e}")

    async def cmd_speed_get(self) -> int:
        speed = None
        try:
            cmd = CMD_SPEED_BYTES
            await self.wyndka_client.write_gatt_char(
                WRITE_CHARACTRERISTIC_UUID, cmd, response=False
            )
            response = await self.wyndka_client.read_gatt_char(READ_CHARACTERISTIC_UUID)
            speed = int.from_bytes(response[0:4], byteorder="little")
        except Exception as e:
            print(e)
        return speed if speed is not None else -1

    async def cmd_driver(self) -> str:
        cmd = CMD_DRIVER_BYTES
        await self.wyndka_client.write_gatt_char(
            WRITE_CHARACTRERISTIC_UUID, cmd, response=False
        )
        response = await self.wyndka_client.read_gatt_char(READ_CHARACTERISTIC_UUID)
        return str(response[0]) + "." + str(response[1])

    async def cmd_speed_set(self, speed_val: int) -> None:
        print("setting speed...")
        try:
            cmd = bytearray(
                CMD_SPEED_BYTES + speed_val.to_bytes(4, byteorder="big", signed=False)
            )
            await self.wyndka_client.write_gatt_char(
                WRITE_CHARACTRERISTIC_UUID, cmd, response=False
            )
        except Exception as e:
            print(e)
        print("Done")

    async def cmd_boot(self) -> None:
        print("entering bootloader")
        try:
            cmd = CMD_BOOT_BYTES
            await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd)
            # send the boot once again to confirm that the device is disconnected
            await self.wyndka_client.write_gatt_char(WRITE_CHARACTRERISTIC_UUID, cmd)
        except Exception as e:
            # ignore error as connection will be broken after switching to bootloader
            pass
        self.wyndka_client = None

    async def cmd_disconnect(self) -> bool:
        print("disconnecting...")
        try:
            await self.wyndka_client.disconnect()
            self.wyndka_client = None
            print("disconnected")
            return True
        except Exception as e:
            print(e)
            return False
