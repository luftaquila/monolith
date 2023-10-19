import re
import sys
import json
import datetime

import serial
import serial.tools.list_ports

from kivy.lang import Builder

from kivymd.app import MDApp
from kivymd.uix.dialog import MDDialog
from kivymd.uix.button import MDFlatButton
from kivymd.uix.menu import MDDropdownMenu
from kivymd.uix.list import OneLineAvatarIconListItem

import builder

class MainApp(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.screen = Builder.load_file('./config/layout.kv')

        self.can_baudrate_menu = MDDropdownMenu(
            caller = self.screen.ids.can_baudrate,
            items = [
                { "viewclass": "OneLineListItem", "text": "125 kbit/s", "on_release": lambda x="125 kbit/s":self.on_can_dropdown(x)},
                { "viewclass": "OneLineListItem", "text": "250 kbit/s", "on_release": lambda x="250 kbit/s":self.on_can_dropdown(x)},
                { "viewclass": "OneLineListItem", "text": "500 kbit/s", "on_release": lambda x="500 kbit/s":self.on_can_dropdown(x)},
                { "viewclass": "OneLineListItem", "text": "1 Mbit/s", "on_release": lambda x="1 Mbit/s":self.on_can_dropdown(x)},
            ],
            width_mult = 2,
            max_height = 200
        )


    def build(self):
        self.theme_cls.theme_style = "Dark"
        self.title = "TMA-1 Configuration Tool"
        return self.screen


    # read build_config.json and init checkboxes on startup
    def on_start(self, **kwargs):
        with open("./config/build_config.json", "r") as file:
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

            self.screen.ids.network_ssid.text   = build_config['ESP32']['network']['ssid']
            self.screen.ids.network_password.text   = build_config['ESP32']['network']['password']
            self.screen.ids.channel_name.text   = build_config['ESP32']['channel']['name']
            self.screen.ids.channel_key.text   = build_config['ESP32']['channel']['key']

            self.screen.ids.server_name.text   = build_config['ESP32']['server']['name']

            self.screen.ids.comport.text = build_config['ESP32']['port']

            self.screen.ids.can_baudrate.set_item(build_config["STM32"]["can"]["baudrate"])


    # update build_config.json on dropdown event
    def on_can_dropdown(self, item):
        builder.clean_stm32()
        
        self.screen.ids.can_baudrate.set_item(item)
        self.can_baudrate_menu.dismiss()

        with open("./config/build_config.json", "r+") as file:
            build_config = json.load(file)

            build_config["STM32"]["can"]["baudrate"] = item

            file.seek(0)
            json.dump(build_config, file, indent=2)
            file.truncate()

            print("INFO: build_config.json writed successfully")


    # update build_config.json on checkbox event
    def on_checkbox_active(self, checkbox, value):
        builder.clean_stm32()

        if checkbox in self.screen.ids.values():
            id = list(self.screen.ids.keys())[list(self.screen.ids.values()).index(checkbox)]

            with open("./config/build_config.json", "r+") as file:
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

            print("INFO: build_config.json writed successfully")


    # update build_config.json on checkbox event
    def on_text_change(self, textfield, value):
        if textfield in self.screen.ids.values():
            id = list(self.screen.ids.keys())[list(self.screen.ids.values()).index(textfield)]

            with open("./config/build_config.json", "r+") as file:
                build_config = json.load(file)

                if id == 'network_ssid':
                    build_config["ESP32"]["network"]["ssid"] = value.strip()

                elif id == 'network_password':
                    build_config["ESP32"]["network"]["password"] = value.strip()

                elif id == 'channel_name':
                    build_config["ESP32"]["channel"]["name"] = value.strip()
                    self.screen.ids.channel_name.text = value.strip()

                elif id == 'channel_key':
                    build_config["ESP32"]["channel"]["key"] = value.strip()
                    self.screen.ids.channel_key.text = value.strip()

                elif id == 'server_name':
                    build_config["ESP32"]["server"]["name"] = value.strip()
                    self.screen.ids.server_name.text = value.strip()

                elif id == 'comport':
                    build_config["ESP32"]["port"] = value.strip()
                    self.screen.ids.comport.text = value.strip()

                file.seek(0)
                json.dump(build_config, file, indent=2)
                file.truncate()


    def flash_stm32(self):
        ret = builder.stm32()

        if ret == 0:
            self.dialog = MDDialog(text=f'[font=consola.ttf][color=00ff00]&bl;OK&br;[/color] [color=ffffff]flashing successfully finished.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
            self.dialog.open()
        else:
            self.dialog = MDDialog(text=f'[font=consola.ttf][color=ff0000]&bl;FAIL&br;[/color] [color=ffffff]flashing failed. please check the console output.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
            self.dialog.open()


    def flash_esp32(self):
        ret = builder.esp32()

        if ret == 0:
            self.dialog = MDDialog(text=f'[font=consola.ttf][color=00ff00]&bl;OK&br;[/color] [color=ffffff]flashing successfully finished.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
            self.dialog.open()
        else:
            self.dialog = MDDialog(text=f'[font=consola.ttf][color=ff0000]&bl;FAIL&br;[/color] [color=ffffff]flashing failed. please check the console output.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
            self.dialog.open()


    def sync_rtc(self):
        ports = sorted([port for port in serial.tools.list_ports.comports()], key=lambda s: int(re.search(r'\d+', s.name).group()))
        ports = list(map(lambda x: OneLineAvatarIconListItem(text=f'[font=Malgun.ttf]{x.description}[/font]', id=x.name, on_release=self.select_port), ports))

        self.dialog = MDDialog(title='[font=consola.ttf]Select UART COM port[/font]', type='confirmation', items=ports, buttons=[MDFlatButton(text='Cancel', on_release=self.close_dialog)])
        self.dialog.open()


    def close_dialog(self, inst):
        self.dialog.dismiss()


    def select_port(self, inst):
        self.dialog.dismiss()

        now = datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S").encode()
        checksum = (sum(now) & 0xff).to_bytes()
        now += checksum

        print(f'\nINFO: synchronizing system time to TMA-1 at {inst.id}: {now}\n')

        try:
            target = serial.Serial(port=inst.id, baudrate=115200, timeout=1)
            target.write(now)
            res = target.read(size=3).decode()
            if res == 'ACK':
                print(f'INFO: TMA-1 RTC unit successfully synced with host system time')
                self.dialog = MDDialog(text=f'[font=consola.ttf][color=00ff00]&bl;OK&br;[/color] [color=ffffff]TMA-1 RTC unit successfully synced.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
                self.dialog.open()
            elif len(res) == 0:
                print(f'ERROR: RTC sync ACK timeout reached')
                self.dialog = MDDialog(text=f'[font=consola.ttf][color=ff0000]&bl;FAIL&br;[/color] [color=ffffff]RTC sync timed out.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
                self.dialog.open()
            else:
                print(f'ERROR: corrupt RTC sync ACK')
                self.dialog = MDDialog(text=f'[font=consola.ttf][color=ff0000]&bl;FAIL&br;[/color] [color=ffffff]RTC sync ACK from TMA-1 is corrrupt.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
                self.dialog.open()

            target.close()
        except serial.serialutil.SerialException as e:
            print(f'{e}\nERROR: serial communication failed. please check COM port number or re-plug the adapter.')


    def clean(self):
        builder.clean()
        self.dialog = MDDialog(text=f'[font=consola.ttf][color=00ff00]&bl;OK&br;[/color] [color=ffffff]INFO: build directory cleaned.[/color][/font]', buttons=[MDFlatButton(text='OK', on_release=self.close_dialog)])
        self.dialog.open()

if __name__ == "__main__":
    MainApp().run()
