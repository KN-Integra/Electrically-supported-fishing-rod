import asyncio
import logging

from kivy.uix.floatlayout import FloatLayout
from mock_bluetoothclient import BluetoothClient, Template

class TemplateButton(FloatLayout):
    counter = 0

    def __init__(self, BLEClient: BluetoothClient, template: Template, **kwargs):
        super(TemplateButton, self).__init__(**kwargs)
        self.register_event_type("on_button_touch")
        self.register_event_type("on_template_touch")
        self.BLEClient = BLEClient
        self.template = template
        self.generate_id()
        self.ids.templ_name.text = template.name
        self.ids.templ_speed.text = f"target speed: {template.speed}"

    def generate_id(self):
        self.id = f"template_button_{TemplateButton.counter}"
        TemplateButton.counter += 1

    def on_button_touch(self, layout_id):
        if self.id == layout_id:
            self.template_delete()

    def on_template_touch(self, layout_id):
        if self.id == layout_id:
            self.template_activate()

    def template_activate(self, *args):
        logging.info(f"setting active templ {self.template.name}, layout_id- {self.id}")
        asyncio.create_task(self.BLEClient.set_active_template(self.template))

    def template_delete(self, *args):
        logging.info(f"deleting {self.id}")
        asyncio.create_task(self.BLEClient.delete_template(self.template))
