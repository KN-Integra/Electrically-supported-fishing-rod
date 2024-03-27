import asyncio
from kivy.app import App
from kivy.uix.screenmanager import Screen


DEVICE_NAME = "Motor Controller"


class ConnectScreen(Screen):
    def __init__(self, **kw):
        super().__init__(**kw)
        self.app = App.get_running_app()

    def on_enter(self, *args):
        print("Entered connect screen")
        return super().on_enter(*args)

    def scan_for_devices(self):
        print("scanning")
        scan_task = asyncio.create_task(self.app.BLEClient.cmd_scan())
        scan_task.add_done_callback(self.scan_callback)

    def scan_callback(self, x):
        devices = x.result()
        all_devices = "".join(
            [str(device.address) + " " + str(device.name) + "\n" for device in devices]
        )
        self.ids.scan_result.text = str(all_devices)
        for device in devices:
            if device.name == DEVICE_NAME:
                self.ids.input_address.text = str(device.address)

    def connect_auto(self):
        print("autconnecting")
        connection_task = asyncio.create_task(self.app.BLEClient.cmd_autoconnect())
        connection_task.add_done_callback(self.connect_callback)

    def connect_with_mac(self):
        print("connecting using mac address")
        mac_addr = self.ids.input_address.text
        connection_task = asyncio.create_task(self.app.BLEClient.cmd_connect(mac_addr))
        connection_task.add_done_callback(self.connect_callback)

    def connect_callback(self, x):
        connection = x.result()
        if connection:
            self.ids.connection_label.text = ""
            self.manager.current = "cmd"
        else:
            self.ids.connection_label.text = "Failed to conenct"
