import asyncio
import logging
from kivy.app import App
from kivy.uix.screenmanager import Screen
from bluetoothclient import BluetoothClient, Template
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.modules import inspector
from kivy.core.window import Window


class ContentBox(BoxLayout):
    def __init__(self, BLEClient: BluetoothClient, template: Template, **kwargs):
        super(ContentBox, self).__init__(**kwargs)
        self.register_event_type("on_click")
        self.BLEClient = BLEClient
        self.template = template
        self.add_widget(Label(text=template.name, size_hint=(1, None), height=40))
        self.add_widget(Label(text=str(template.speed), size_hint=(1, None), height=40))

    def on_touch_down(self, touch):
        if self.collide_point(*touch.pos):
            self.dispatch("on_click")
            return True
        return super(ContentBox, self).on_touch_down(touch)

    def on_click(self):
        asyncio.create_task(self.BLEClient.set_active_template(self.template))


class TemplateButton(BoxLayout):
    def __init__(self, BLEClient: BluetoothClient, template: Template, **kwargs):
        super(TemplateButton, self).__init__(**kwargs)
        self.BLEClient = BLEClient
        self.template = template
        content_box = ContentBox(BLEClient=BLEClient, template=template)
        del_button = Button(text="del", on_press=self.template_delete)

        self.add_widget(content_box)
        self.add_widget(del_button)

    def template_delete(self, *args):
        logging.info("deleting")
        asyncio.create_task(self.BLEClient.delete_template(self.template))


class CmdScreen(Screen):
    def __init__(self, **kw):
        super().__init__(**kw)
        self.app = App.get_running_app()
        self.BLEClient: BluetoothClient = self.app.BLEClient

    def on_enter(self, *args):
        logging.info("Entered cmd screen")
        self.templates_update()
        return super().on_enter(*args)

    def speed_get(self):
        speed_task = asyncio.create_task(self.BLEClient.cmd_speed_get())
        speed_task.add_done_callback(self.speed_get_callback)

    def speed_get_callback(self, task):
        self.ids.speed_label.text = "current speed: " + str(task.result())

    def templates_update(self):
        templates_task = asyncio.create_task(self.BLEClient.get_templ_list())
        templates_task.add_done_callback(self.templates_update_callback)

    def templates_update_callback(self, task):
        # templates_view = self.ids.templates_view
        inspector.create_inspector(Window, self)
        self.update_templates_grid()

    def template_add(self):
        templ = Template(
            name=self.ids.new_template_name.text,
            speed=int(self.ids.new_template_speed.text),
        )
        logging.info(templ)
        template_add_task = asyncio.create_task(self.BLEClient.create_template(templ))
        template_add_task.add_done_callback(self.template_add_callback)

    def template_add_callback(self, task):
        # add_button = self.ids.new_template
        self.ids.new_template_name.text = ""
        self.update_templates_grid()
        # add_button.ids.new_template_speed.

    def update_templates_grid(self):
        logging.info("cleraing widget")
        templates_grid = self.ids.templates_grid
        templates_grid.clear_widgets()
        logging.info("widgets should be cleared")
        templates_grid.bind(minimum_height=templates_grid.setter("height"))
        templates = self.BLEClient.template_list
        for template in templates:
            logging.info(template)
            template_button = TemplateButton(self.BLEClient, template)
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
