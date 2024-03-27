import kivy
import asyncio

from bluetoothclient import BluetoothClient

from kivymd.app import MDApp

# from kivy.app import App
from kivy.core.text import LabelBase
from kivy.uix.screenmanager import ScreenManager
from kivy.lang import Builder

from screens.cmd import CmdScreen
from screens.connect import ConnectScreen

kivy.require("2.2.0")

Builder.load_file("screens/connectscreen.kv")
Builder.load_file("screens/cmdscreen.kv")
Builder.load_file("custom_widgets/roundedwidgets.kv")
Builder.load_file("custom_widgets/components.kv")

LabelBase.register(
    name="lilita-one", fn_regular="fonts/Lilita_One/LilitaOne-Regular.ttf"
)


class FishingRodApp(MDApp):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.BLEClient = BluetoothClient()

    def build(self):
        screens = [ConnectScreen(name="connect"), CmdScreen(name="cmd")]

        screen_manager = ScreenManager()

        # HACK: to set dark theme
        self.theme_cls.theme_style = "Dark"

        for screen in screens:
            screen_manager.add_widget(screen)

        return screen_manager

    def on_start(self):
        pass

    def on_stop(self):
        pass


async def main():
    app = FishingRodApp()
    await asyncio.gather(app.async_run("asyncio"))


if __name__ == "__main__":
    asyncio.run(main())
