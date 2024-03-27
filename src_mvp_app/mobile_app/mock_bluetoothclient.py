import logging
import random
from bleak import BleakScanner, BleakClient, BLEDevice
from dataclasses import dataclass


CMD_BOOT_BYTES = b"\x00"
CMD_SPEED_SET_BYTES = b"\x01"
CMD_ADD_TMPL = b"\x02"
CMD_DEL_TMPL = b"\x03"  # + string name
CMD_ACT_TMPL = b"\x04"  # + string name

DEVICE_NAME = "WyndkaTemplates"
DEVICE_COUNT = 5


@dataclass
class Template:
    name: str
    speed: int

    def to_bytes(self):
        return (
            self.name.encode(encoding="utf-8")
            + (12 - len(self.name)) * b"\0"
            + int.to_bytes(self.speed, length=4, byteorder="big")
        )


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
        self.scanner: BleakScanner = None
        self.wyndka_client: BleakClient = None
        self.wyndka: BLEDevice = None
        # TODO: make class Template instead of this filthy dicts
        self.template_list: TemplateList
        self.template_config: TemplatConfig
        self.curr_templ: Template

    def check_connection(self) -> bool:
        return True

    async def cmd_scan(self) -> list[BLEDevice]:
        return [
            BLEDevice(
                address=f"F0:C2:DF:BE:FE:3{i}",
                name="device{i}",
                details="device{i} details",
                rssi=10 + i,
            )
            for i in range(DEVICE_COUNT)
        ]

    async def cmd_autoconnect(self) -> bool:
        i = random.randint(1, DEVICE_COUNT)
        self.wyndka = (await self.cmd_scan())[i]

        return True

    async def cmd_connect(self, addr: str) -> bool:
        """_summary_
        Connects to a random mock device
        Args:
            addr (str): device address, does not have any impact on the function

        Returns:
            bool: always True
        """
        return await self.cmd_autoconnect()

    async def get_templ_list(self) -> None:
        logging.info("Getting speed config data")
        self.template_config = TemplatConfig(
            length=12, page_no=1, template_count=3, text_len=20
        )
        self.template_list = TemplateList(
            [
                Template("Szczupak", 32000),
                Template("Karp", 42000),
                Template("makrela", 20000),
                Template("morszczuk", 12000),
            ]
        )

    async def get_active_templ(self, *args) -> Template:
        logging.info("Getting current template")
        return self.curr_templ

    async def set_active_template(self, templ: Template) -> Template:
        logging.info("setting active template")
        self.curr_templ = templ
        return self.curr_templ

    async def create_template(self, templ: Template):
        logging.info("creating new template")
        self.template_list.items.append(templ)

    async def delete_template(self, templ: Template):
        logging.info("creating new template")
        self.template_list.items.remove(templ)

    async def cmd_speed_get(self) -> int:
        return self.curr_templ.speed

    async def cmd_driver(self) -> str:
        return "0.1"

    async def cmd_speed_set(self, speed_val: int) -> None:
        print("To set custom speed please change the template or create a new one")

    async def cmd_boot(self) -> None:
        print("entering bootloader")
        self.wyndka_client = None

    async def cmd_disconnect(self) -> bool:
        print("disconnecting...")
        self.wyndka_client = None
        print("disconnected")
        return True
