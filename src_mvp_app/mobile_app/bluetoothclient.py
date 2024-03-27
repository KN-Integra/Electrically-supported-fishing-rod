import logging
from bleak import BleakScanner, BleakClient, BLEDevice
from dataclasses import dataclass

CMD_BOOT_BYTES = b"\x00"
CMD_SPEED_SET_BYTES = b"\x01"
CMD_ADD_TMPL = b"\x02"
CMD_DEL_TMPL = b"\x03"  # + string name
CMD_ACT_TMPL = b"\x04"  # + string name

WRITE_CMD_CHARACTERISTIC_UUID = "6e006610-37de-44d4-be45-d1f1fd9385fd"
READ_CHARACTERISTIC_TEMPL_LIST_UUID = "6e006620-37de-44d4-be45-d1f1fd9385fd"
READ_CHARACTERISTIC_TEMPL_ACT_UUID = "6e006621-37de-44d4-be45-d1f1fd9385fd"
READ_SPEED_CHARACTERISTIC_UUID = "6e006622-37de-44d4-be45-d1f1fd9385fd"
READ_HW_VERSION_CHARACTERISTIC_UUID = "6e006623-37de-44d4-be45-d1f1fd9385fd"

DEVICE_NAME = "Wyndka"


@dataclass
class Template:
    name: str
    speed: int

    # TODO: make it responsive with different name sizes
    def to_bytes(self):
        return (
            self.name[:11].encode(encoding="utf-8")
            + (12 - len(self.name[:11])) * b"\0"
            + int.to_bytes(self.speed, length=4, byteorder="big")
        )

    def name_to_bytes(self):
        return self.name[:11].encode(encoding="utf-8") + b"\0"


@dataclass
class TemplateList:
    items: list[Template]


@dataclass
class TemplatConfig:
    length: int
    page_no: int
    template_count: int
    text_len: int


class BluetoothClient:
    def __init__(self) -> None:
        self.scanner: BleakScanner = BleakScanner()
        self.wyndka_client: BleakClient = None
        self.wyndka: BLEDevice = None
        self.template_list: TemplateList
        self.template_config: TemplatConfig
        self.curr_templ: Template

    def check_connection(self) -> bool:
        if self.wyndka_client or len(self.template_list.items()):
            return self.wyndka_client.is_connected
        else:
            return False

    async def cmd_scan(self) -> list[BLEDevice]:
        print("Scanning for devices...")
        # TODO: try active scanning by using scanner.start() and periodically print scanner.discoverd_devices
        devices = await self.scanner.discover(timeout=5, return_adv=False)
        return devices

    async def cmd_autoconnect(self) -> bool:
        self.wyndka = await self.scanner.find_device_by_name(DEVICE_NAME, 5)
        if self.wyndka:
            await self.cmd_connect(self.wyndka.address)
            return self.check_connection()
        else:
            print(
                "Failed to find Wyndka, make sure that the device is powered on\n\
                  If it is, then try to run scan, and connect using the proper mac address"
            )
        return False

    async def cmd_connect(self, addr: str) -> bool:
        logging.info("connecting")
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
            await self.get_templ_list()
            # TODO: get active template periodically
            await self.get_active_templ()
            logging.info(self.curr_templ)
            return self.check_connection()
        except Exception as e:
            print(e)
            return False

    def calc_bounds_for(self, type: int, field_no: int, text_len: int) -> int:
        """
        type:\n
            0 - name\n
            1 - speed
        """
        if type == 0:
            start = 4 + (field_no) * (text_len + 4)
            return (start, start + text_len)
        elif type == 1:
            start = (4 + (field_no) * (text_len + 4)) + text_len
            return (start, start + 4)
        else:
            return (0, 0)

    def get_field_data(self, sd: bytearray, field_no: int, text_len: int) -> Template:
        bounds_name = self.calc_bounds_for(0, field_no, text_len)
        bounds_speed = self.calc_bounds_for(1, field_no, text_len)
        return Template(
            name=sd[bounds_name[0] : bounds_name[1]].split(b"\x00")[0].decode("utf-8"),
            speed=int.from_bytes(
                sd[bounds_speed[0] : bounds_speed[1]], byteorder="little"
            ),
        )

    def save_templ_list(self, sd: bytearray) -> None:
        # assuming automatic conversion to int
        count = sd[2]
        text_len = sd[3]
        self.template_config = TemplatConfig(
            length=sd[0], page_no=sd[1], template_count=sd[2], text_len=sd[3]
        )
        self.template_list = [
            self.get_field_data(sd, i, text_len) for i in range(count)
        ]

    async def get_templ_list(self):
        logging.info("Getting speed config data")
        try:
            response = await self.wyndka_client.read_gatt_char(
                READ_CHARACTERISTIC_TEMPL_LIST_UUID
            )
            logging.info("received speed config data")
            self.save_templ_list(response)
            logging.info(self.template_list)
        except Exception as e:
            print(e)

    @staticmethod
    async def format_templ(tmpl: bytearray, text_len: int, speed_len: int):
        return Template(
            name=str((tmpl[0:text_len].split(b"\x00")[0]).decode("utf-8")),
            speed=int.from_bytes(
                tmpl[text_len : (text_len + speed_len)], byteorder="little"
            ),
        )

    async def get_active_templ(self, *args) -> Template:
        logging.info("Getting current template")
        try:
            curr_templ = await self.wyndka_client.read_gatt_char(
                READ_CHARACTERISTIC_TEMPL_ACT_UUID
            )
            self.curr_templ = await BluetoothClient.format_templ(
                curr_templ, self.template_config.text_len, 4
            )
            logging.info(self.curr_templ)
        except Exception as e:
            print(e)
            self.curr_templ = Template(name="no_data", speed=0)

    async def set_active_template(self, templ: Template) -> None:
        logging.info("setting active template")
        cmd = CMD_ACT_TMPL + templ.to_bytes()
        try:
            await self.wyndka_client.write_gatt_char(
                WRITE_CMD_CHARACTERISTIC_UUID, cmd, response=True
            )
        except Exception as e:
            logging.error(e)

    async def create_template(self, templ: Template) -> None:
        logging.info("creating new template")
        cmd = CMD_ADD_TMPL + templ.to_bytes()
        try:
            await self.wyndka_client.write_gatt_char(
                WRITE_CMD_CHARACTERISTIC_UUID, cmd, response=True
            )
        except Exception as e:
            logging.error(e)

    async def delete_template(self, templ: Template) -> None:
        logging.info("Removing template")
        cmd = CMD_DEL_TMPL + templ.name_to_bytes()
        try:
            await self.wyndka_client.write_gatt_char(
                WRITE_CMD_CHARACTERISTIC_UUID, cmd, response=True
            )
        except Exception as e:
            logging.error(e)

    async def cmd_speed_get(self) -> int:
        speed = None
        try:
            response = await self.wyndka_client.read_gatt_char(
                READ_SPEED_CHARACTERISTIC_UUID
            )
            speed = int.from_bytes(response[0:4], byteorder="little")
        except Exception as e:
            print(e)
        return speed if speed is not None else -1

    async def cmd_driver(self) -> str:
        response = await self.wyndka_client.read_gatt_char(
            READ_HW_VERSION_CHARACTERISTIC_UUID
        )
        return str(response[0]) + "." + str(response[1])

    async def cmd_speed_set(self, speed_val: int) -> None:
        print("setting speed...")
        try:
            cmd = bytearray(
                CMD_SPEED_SET_BYTES
                + speed_val.to_bytes(4, byteorder="big", signed=False)
            )
            await self.wyndka_client.write_gatt_char(
                WRITE_CMD_CHARACTERISTIC_UUID, cmd, response=True
            )
        except Exception as e:
            print(e)
        print("Done")

    async def cmd_boot(self) -> None:
        print("entering bootloader")
        try:
            cmd = CMD_BOOT_BYTES
            await self.wyndka_client.write_gatt_char(WRITE_CMD_CHARACTERISTIC_UUID, cmd)
            # send the boot once again to confirm that the device is disconnected
            await self.wyndka_client.write_gatt_char(WRITE_CMD_CHARACTERISTIC_UUID, cmd)
        except Exception:
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
