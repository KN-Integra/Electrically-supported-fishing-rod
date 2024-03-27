import asyncio
import logging

from kivy.clock import Clock
from kivy.app import App
from kivy.uix.screenmanager import Screen
from bluetoothclient import BluetoothClient, Template
from kivy.modules import inspector
from kivy.core.window import Window
from custom_widgets.components import TemplateButton


Window.size = (1080, 2340)


class CmdScreen(Screen):
    def __init__(self, **kw):
        super().__init__(**kw)
        self.app = App.get_running_app()
        self.BLEClient: BluetoothClient = self.app.BLEClient

    def on_enter(self, *args):
        logging.info("Entered cmd screen")
        self.templates_update()
        return super().on_enter(*args)

    def on_leave(self, *args):
        logging.info("exiting command screen")
        self.clear_templates_grid()
        return super().on_enter(*args)

    def speed_get(self):
        speed_task = asyncio.create_task(self.BLEClient.cmd_speed_get())
        speed_task.add_done_callback(self.speed_get_callback)

    def speed_get_callback(self, task):
        self.ids.speed_label.text = "current speed: " + str(task.result())
        self.ids.speed_label.width = self.width - 60

    def templates_update(self):
        templates_task = asyncio.create_task(self.BLEClient.get_templ_list())
        templates_task.add_done_callback(self.templates_update_callback)

    def templates_update_callback(self, task):
        # templates_view = self.ids.templates_view
        inspector.create_inspector(Window, self)
        Clock.schedule_once(self.update_templates_grid)

    def template_add(self):
        name = self.ids.new_template_name.text
        speed = self.ids.new_template_speed.text
        templ = Template(name=name, speed=int(speed) if len(speed) > 0 else 0)
        logging.info(templ)
        template_add_task = asyncio.create_task(self.BLEClient.create_template(templ))
        template_add_task.add_done_callback(self.template_add_callback)

    def template_add_callback(self, task):
        # add_button = self.ids.new_template
        self.ids.new_template_name.text = ""
        self.update_templates_grid()
        # add_button.ids.new_template_speed.

    def update_templates_grid(self, *args):
        logging.info("clearaing widget")
        templates_grid = self.ids.templates_grid
        templates_grid.clear_widgets()
        logging.info("widgets should be cleared")
        templates_grid.bind(minimum_height=templates_grid.setter("height"))
        templates = self.BLEClient.template_list
        for template in templates:
            logging.info(template)
            template_button = TemplateButton(template)
            templates_grid.add_widget(template_button)

    def speed_set(self):
        print("setting speed")
        speed = self.ids.speed_input.text
        speed_val = int(speed) if speed else 0
        speed_set_task = asyncio.create_task(self.BLEClient.cmd_speed_set(speed_val))
        speed_set_task.add_done_callback(self.speed_set_callback)

    def speed_set_callback(self, task):
        pass

    def get_driver_version(self):
        print("getting driver version")
        version_task = asyncio.create_task(self.BLEClient.cmd_driver())
        version_task.add_done_callback(self.get_driver_version_callback)

    def get_driver_version_callback(self, task):
        self.ids.driver_label.text = "Driver version: " + str(task.result())

    def enter_bootloader(self):
        print("entering bootloader")
        asyncio.create_task(self.BLEClient.cmd_boot()).add_done_callback(
            self.disconnect_callback
        )

    def disconnect(self):
        print("disconnecting")
        asyncio.create_task(self.BLEClient.cmd_disconnect()).add_done_callback(
            self.disconnect_callback
        )

    def disconnect_callback(self, task):
        self.manager.current = "connect"

    #   helpers
    def clear_templates_grid(self):
        templates = self.ids.templates_grid
        templates.clear_widgets()
        # I cannot find the .clear() method in the documentation, but apparently it does work
        # I found it in the O'reilly book about kivy
        # also cannot find the kivy.properties.ObservableList class, which is the type of templates
