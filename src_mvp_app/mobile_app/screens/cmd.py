import asyncio
from kivy.app import App
from kivy.uix.screenmanager import Screen


class CmdScreen(Screen):
    def __init__(self, **kw):
        super().__init__(**kw)
        self.app = App.get_running_app()

    def on_enter(self, *args):
        print("Entered cmd screen")
        return super().on_enter(*args)

    def initialize_device(self):
        asyncio.create_task(self.app.BLEClient.cmd_init())

    def speed_get(self):
        speed_task = asyncio.create_task(self.app.BLEClient.cmd_speed_get())
        speed_task.add_done_callback(self.speed_get_callback)

    def speed_get_callback(self, task):
        self.ids.speed_label.text = "current speed: " + str(task.result())

    def speed_set(self):
        print("setting speed")
        speed = self.ids.speed_input.text
        speed_val = int(speed) if speed else 0
        speed_set_task = asyncio.create_task(self.app.BLEClient.cmd_speed_set(speed_val))
        speed_set_task.add_done_callback(self.speed_set_callback)

    def speed_set_callback(self, task):
        pass

    def get_driver_version(self):
        print("getting driver version")
        version_task = asyncio.create_task(self.app.BLEClient.cmd_driver())
        version_task.add_done_callback(self.get_driver_version_callback)

    def get_driver_version_callback(self, task):
        self.ids.driver_label.text = "Driver version: " + str(task.result())

    def enter_bootloader(self):
        print("entering bootloader")
        asyncio.create_task(self.app.BLEClient.cmd_boot())\
            .add_done_callback(self.disconnect_callback)

    def disconnect(self):
        print("disconnecting")
        asyncio.create_task(self.app.BLEClient.cmd_disconnect())\
            .add_done_callback(self.disconnect_callback)

    def disconnect_callback(self, task):
        self.manager.current = 'connect'
