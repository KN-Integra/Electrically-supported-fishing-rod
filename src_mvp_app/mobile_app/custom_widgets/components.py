import asyncio
import logging

from kivymd.app import MDApp
from kivymd.uix.floatlayout import MDFloatLayout
from kivymd.uix.button import MDFillRoundFlatButton, MDIconButton
from mock_bluetoothclient import BluetoothClient, Template


class TemplateButton(MDFloatLayout):
    counter = 0

    def __init__(self, template: Template, **kwargs):
        super(TemplateButton, self).__init__(**kwargs)
        # self.register_event_type("on_button_touch")
        # self.register_event_type("on_template_touch")
        self.BLEClient: BluetoothClient = MDApp.get_running_app().BLEClient
        self.template = template
        self.generate_id()
        self.ids.templ_name.text = template.name
        self.ids.templ_speed.text = f"target speed: {template.speed}"

    def generate_id(self):
        self.id = f"template_button_{TemplateButton.counter}"
        TemplateButton.counter += 1


class TemplateButtonDelete(MDIconButton):
    def __init__(self, **kwargs):
        super(TemplateButtonDelete, self).__init__(**kwargs)
        self.register_event_type("on_btn_click")

    def on_btn_click(self, templ_id: int, *args):
        logging.info(f"deleting {self.parent.id} {templ_id}")
        # TODO: this parent.parent is very ugly, it should be done better
        self.parent.parent.remove_widget(self.parent)
        asyncio.create_task(self.parent.BLEClient.delete_template(self.parent.template))


class TemplateButtonActivate(MDFillRoundFlatButton):
    def __init__(self, **kwargs):
        super(TemplateButtonActivate, self).__init__(**kwargs)
        self.register_event_type("on_button_touch")

    def on_button_touch(self, template_id: int):
        logging.info("button clicked")
        if self.parent.id == template_id:
            self.template_activate()

    def template_activate(self, *args):
        logging.info(
            f"setting active templ {self.parent.template.name}, layout_id- {self.parent.id}"
        )
        asyncio.create_task(
            self.parent.BLEClient.set_active_template(self.parent.template)
        )
