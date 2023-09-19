from kivy.lang import Builder
from kivymd.app import MDApp
from kivymd.uix.label import MDLabel
from kivymd.uix.screen import MDScreen
from kivymd.uix.button import MDRectangleFlatButton

class MainApp(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.screen = Builder.load_file('./layout.kv')

    def build(self):
        self.theme_cls.theme_style = "Dark"
        self.title = "TMA-1 Configuration Tool"
        return self.screen

    def on_start(self, **kwargs):
        self.screen.ids.enable_telemetry

    def on_checkbox_active(self, checkbox, value):
        if value:
            print('The checkbox', checkbox, 'is active')
        else:
            print('The checkbox', checkbox, 'is inactive')

if __name__ == "__main__":
    MainApp().run()
