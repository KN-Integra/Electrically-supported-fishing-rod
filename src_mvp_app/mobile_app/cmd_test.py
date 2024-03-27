import kivy
import asyncio

from bluetoothclient import BluetoothClient

from kivy.app import App
from kivy.core.text import LabelBase
from kivy.uix.screenmanager import ScreenManager
from kivy.lang import Builder

from screens.cmd import CmdScreen
from screens.connect import ConnectScreen

kivy.require("2.2.0")

Builder.load_file("screens/connectscreen.kv")
Builder.load_file("screens/cmdscreen.kv")
Builder.load_file("custom_widgets/roundedwidgets.kv")

LabelBase.register(
    name="lilita-one", fn_regular="fonts/Lilita_One/LilitaOne-Regular.ttf"
)


class FishingRodApp(App):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.BLEClient = BluetoothClient()

    def build(self):
        screens = [CmdScreen(name="cmd")]
        screen_manager = ScreenManager()
        for screen in screens:
            screen_manager.add_widget(screen)

        return screen_manager

    def on_start(self):
        pass

    def on_stop(self):
        pass


async def main(app):
    await asyncio.gather(app.async_run("asyncio"))


if __name__ == "__main__":
    app = FishingRodApp()
    asyncio.run(main(app))
