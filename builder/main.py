import json
from kivy.lang import Builder
from kivymd.app import MDApp

class MainApp(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.screen = Builder.load_file('./layout.kv')

    def build(self):
        self.theme_cls.theme_style = "Dark"
        self.title = "TMA-1 Configuration Tool"
        return self.screen

    def on_start(self, **kwargs):
        with open("build_config.json", "r") as file:
            build_config = json.load(file)

            self.screen.ids.telemetry.state = 'down' if build_config["STM32"]["output"]["telemetry"] else 'normal'
            self.screen.ids.serial.state    = 'down' if build_config["STM32"]["output"]["serial"] else 'normal'

            self.screen.ids.can.state           = 'down' if build_config["STM32"]["sensor"]["can"] else 'normal'
            self.screen.ids.digital.state       = 'down' if build_config["STM32"]["sensor"]["digital"] else 'normal'
            self.screen.ids.analog.state        = 'down' if build_config["STM32"]["sensor"]["analog"] else 'normal'
            self.screen.ids.pulse.state         = 'down' if build_config["STM32"]["sensor"]["pulse"] else 'normal'
            self.screen.ids.accelerometer.state = 'down' if build_config["STM32"]["sensor"]["accelerometer"] else 'normal'
            self.screen.ids.gps.state           = 'down' if build_config["STM32"]["sensor"]["gps"] else 'normal'

            self.screen.ids.debug_mode.state    = 'down' if build_config["STM32"]["debug"]["debug_mode"] else 'normal'
            self.screen.ids.release_build.state = 'down' if build_config["STM32"]["debug"]["release_build"] else 'normal'

    def on_checkbox_active(self, checkbox, value):
        if checkbox in self.screen.ids.values():
            id = list(self.screen.ids.keys())[list(self.screen.ids.values()).index(checkbox)]

            build_config = ''
            with open("build_config.json", "r+") as file:
                build_config = json.load(file)

                if id in build_config["STM32"]["output"]:
                    build_config["STM32"]["output"][id] = value

                elif id in build_config["STM32"]["sensor"]:
                    build_config["STM32"]["sensor"][id] = value

                elif id in build_config["STM32"]["debug"]:
                    build_config["STM32"]["debug"][id] = value

                file.seek(0)
                json.dump(build_config, file, indent=2)
                file.truncate()

if __name__ == "__main__":
    MainApp().run()
