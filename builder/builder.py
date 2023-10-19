import os
import sys
import json
import shutil
import subprocess
import serial.tools.list_ports

import toolchain

# spawn a subprocess
def spawn(argv):
    p = subprocess.Popen(argv, encoding='utf-8', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    while p.poll() == None:
        try:
            print(p.stdout.readline(), end='')
        except UnicodeDecodeError:
            pass

    return p.poll()


##### STM32 build and flash process START #####
def build_stm32():
    print("INFO: building TMA-1 STM32 binary...")

    # set build environment variables
    with open("./config/build_config.json", "r") as file:
        build_config = json.load(file)

        os.environ["ENABLE_TELEMETRY"] = '1' if build_config["STM32"]["output"]["telemetry"] else '0'
        os.environ["ENABLE_SERIAL"]    = '1' if build_config["STM32"]["output"]["serial"] else '0'

        os.environ["ENABLE_MONITOR_CAN"]           = '1' if build_config["STM32"]["sensor"]["can"] else '0'
        os.environ["ENABLE_MONITOR_DIGITAL"]       = '1' if build_config["STM32"]["sensor"]["digital"] else '0'
        os.environ["ENABLE_MONITOR_ANALOG"]        = '1' if build_config["STM32"]["sensor"]["analog"] else '0'
        os.environ["ENABLE_MONITOR_PULSE"]         = '1' if build_config["STM32"]["sensor"]["pulse"] else '0'
        os.environ["ENABLE_MONITOR_ACCELEROMETER"] = '1' if build_config["STM32"]["sensor"]["accelerometer"] else '0'
        os.environ["ENABLE_MONITOR_GPS"]           = '1' if build_config["STM32"]["sensor"]["gps"] else '0'

        os.environ["DEBUG_MODE"] = '1' if build_config["STM32"]["debug"]["debug_mode"] else '0'

        os.environ["DEBUG"] = '0' if build_config["STM32"]["debug"]["release_build"] else '1'
        os.environ["OPT"] = '-O2' if build_config["STM32"]["debug"]["release_build"] else '-Og'

        if build_config["STM32"]["can"]["baudrate"] == '125 kbit/s':
            os.environ["CAN_PRESC"] = "21"
            os.environ["CAN_TSEG1"] = "CAN_BS1_13TQ"

        elif build_config["STM32"]["can"]["baudrate"] == '250 kbit/s':
            os.environ["CAN_PRESC"] = "12"
            os.environ["CAN_TSEG1"] = "CAN_BS1_11TQ"

        elif build_config["STM32"]["can"]["baudrate"] == '500 kbit/s':
            os.environ["CAN_PRESC"] = "6"
            os.environ["CAN_TSEG1"] = "CAN_BS1_11TQ"

        elif build_config["STM32"]["can"]["baudrate"] == '1 Mbit/s':
            os.environ["CAN_PRESC"] = "3"
            os.environ["CAN_TSEG1"] = "CAN_BS1_11TQ"

    # step into STM32CubeMX project folder
    os.chdir('../device/TMA-1')

    # build
    ret = spawn(['make'])

    # step back to builder
    os.chdir('../../builder')

    if ret != 0:
        print("\nERROR: build job failed. terminating.")
        return -1

    # move target executable
    if os.path.exists('../device/TMA-1/build/TMA-1.elf'):
        os.makedirs('./build', exist_ok=True)
        shutil.copyfile('../device/TMA-1/build/TMA-1.elf', './build/TMA-1.elf')
        print("INFO: build of TMA-1 STM32 binary completed! binary copied to build/TMA-1.elf")
        return 0

    else:
        print("\nERROR: No binary found for STM32. terminating.")
        return -1


def flash_stm32():
    print("INFO: flashing TMA-1 STM32 binary...")

    retry = 0
    ret = spawn(['openocd', '-f', './config/TMA-1.cfg'])

    while ret != 0 and retry < 2:
        retry += 1
        print(f"ERROR: flashing failed. retry count: {retry}")
        ret = spawn(['openocd', '-f', './config/TMA-1.cfg'])

    if ret != 0:
        print("ERROR: max retry count reached. terminating.")
        return -1

    print('INFO: TMA-1 STM32 binary successfully flashed. please recyle the power.')
    return 0
##### STM32 build and flash process END #####


##### ESP32 build and flash process START #####
def build_esp32():
    print("INFO: building TMA-1 ESP32 binary...")

    # set build environment variables
    with open("./config/build_config.json", "r") as file:
        build_config = json.load(file)

    ret = spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'compile', '-e', '--fqbn', 'esp32:esp32:esp32', '-v', '--build-property', f'compiler.cpp.extra_flags="-DNETWORK_SSID="{build_config["ESP32"]["network"]["ssid"]}"" -DNETWORK_PASSWORD="{build_config["ESP32"]["network"]["password"]}" -DCHANNEL_NAME="{build_config["ESP32"]["channel"]["name"]}" -DCHANNEL_KEY="{build_config["ESP32"]["channel"]["key"]}" -DSERVER_NAME="{build_config["ESP32"]["server"]["name"]}"', '../device/telemetry/telemetry.ino'])

    if ret != 0:
        print("\nERROR: build job failed. terminating.")
        return -1

    # move target executable
    if os.path.exists('../device/telemetry/build/esp32.esp32.esp32/telemetry.ino.elf'):
        os.makedirs('./build', exist_ok=True)
        shutil.copyfile('../device/telemetry/build/esp32.esp32.esp32/telemetry.ino.elf', './build/telemetry.elf')
        shutil.copyfile('../device/telemetry/build/esp32.esp32.esp32/telemetry.ino.bin', './build/telemetry.bin')
        shutil.copyfile('../device/telemetry/build/esp32.esp32.esp32/telemetry.ino.map', './build/telemetry.map')
        shutil.copyfile('../device/telemetry/build/esp32.esp32.esp32/telemetry.ino.bootloader.bin', './build/telemetry.bootloader.bin')
        shutil.copyfile('../device/telemetry/build/esp32.esp32.esp32/telemetry.ino.partitions.bin', './build/telemetry.partitions.bin')
        print("INFO: build of TMA-1 ESP32 binary completed! binary copied to build/telemetry.*")
        return 0

    print("INFO: build of TMA-1 ESP32 binary completed!")
    return 0

def flash_esp32():
    print("INFO: flashing TMA-1 ESP32 binary...")

    # set build environment variables
    port = None
    with open("./config/build_config.json", "r") as file:
        build_config = json.load(file)
        port = build_config["ESP32"]["port"]

    # auto-detect esp32 port
    if port == "":
        for p in serial.tools.list_ports.comports():
            if p.vid == 0x10C4 and p.pid == 0xEA60:
                port = p.name
                break

    # auto-detect failure
    if port == "":
        print("ERROR: no ESP32 found! please check connection or specify the COM port in the Configuration Tool.")
        return -1

    ret = spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'upload', '-i', './build/telemetry.elf', '--fqbn', 'esp32:esp32:esp32', '-p', port ])

    if ret != 0:
        print("ERROR: flashing failed. terminating.")
        return -1

    print('INFO: TMA-1 ESP32 binary successfully flashed. please recyle the power.')
    return 0
##### ESP32 build and flash process END #####


##### Monolith TMA-1 STM32 firmware build and flash process START #####
def stm32():
    print("INFO: checking toolchain...")
    if toolchain.validate('stm') != 0:
        print("\nERROR: toolchain is corrupt. please delete builder/toolchain/.cache and retry.")
        return -1
    print('INFO: toolchain validation finished. start build...')

    if build_stm32() != 0:
        print("\nERROR: build of TMA-1 STM32 binary failed. please delete builder/toolchain and retry.")
        return -1

    print('\nINFO: build finished. start flashing...\n')

    if flash_stm32() != 0:
        print("\nERROR: flashing of TMA-1 STM32 binary failed. please check debugger and retry.")
        return -1

    print('\nINFO: TMA-1 STM32 flashing finished.')

    return 0
##### Monolith TMA-1 STM32 firmware build and flash process end #####


##### Monolith TMA-1 ESP32 firmware build and flash process START #####
def esp32():
    print("INFO: checking toolchain...")
    if toolchain.validate('esp') != 0:
        print("\nERROR: toolchain is corrupt. please delete builder/toolchain/.cache and retry.")
        return -1
    print('INFO: toolchain validation finished. start build...')

    if build_esp32() != 0:
        print("\nERROR: build of TMA-1 ESP32 binary failed. please check console output and retry.")
        return -1

    print('\nINFO: build finished. start flashing...\n')

    if flash_esp32() != 0:
        print("\nERROR: flashing of TMA-1 ESP32 binary failed. please check connection and retry.")
        return -1

    print('\nINFO: TMA-1 ESP32 flashing finished.')

    return 0
##### Monolith TMA-1 ESP32 firmware build and flash process end #####

# clean build directories
def clean():
    # build directory
    print("cleaning ./build...")
    if os.path.exists('./build'):
        shutil.rmtree('./build')

    # STM32CubeMX build directory
    print("cleaning ../device/TMA-1/build...")
    if os.path.exists('../device/TMA-1/build'):
        shutil.rmtree('../device/TMA-1/build')

    print("cleaning ../device/telemetry/build...")
    if os.path.exists('../device/telemetry/build'):
        shutil.rmtree('../device/telemetry/build')

def clean_stm32():
    # build directory
    print("cleaning ./build...")
    if os.path.exists('./build'):
        shutil.rmtree('./build')

    # STM32CubeMX build directory
    print("cleaning ../device/TMA-1/build...")
    if os.path.exists('../device/TMA-1/build'):
        shutil.rmtree('../device/TMA-1/build')

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f'ERROR: no command specified')
    elif sys.argv[1] == "stm":
        if len(sys.argv) < 3:
            stm32()
        else:
            if sys.argv[2] == "build":
                if toolchain.validate('stm') == 0:
                    build_stm32()
            elif sys.argv[2] == "flash":
                if toolchain.validate('stm') == 0:
                    flash_stm32()
            else:
                print(f'ERROR: unknown command {sys.argv[2]}')

    elif sys.argv[1] == "esp":
        if len(sys.argv) < 3:
            esp32()
        else:
            if sys.argv[2] == "build":
                if toolchain.validate('esp') == 0:
                    build_esp32()
            elif sys.argv[2] == "flash":
                if toolchain.validate('esp') == 0:
                    flash_esp32()
            else:
                print(f'ERROR: unknown command {sys.argv[2]}')
    elif sys.argv[1] == "clean":
        clean()
    else:
        print(f'ERROR: unknown command {sys.argv[1]}')
