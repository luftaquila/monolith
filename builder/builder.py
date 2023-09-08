import os
import json
import shutil
import subprocess

import toolchain

# read build_config.json
build_config = ''
with open("build_config.json", "r") as file:
    build_config = json.load(file)

# spawn a subprocess
def spawn(argv):
    p = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    while p.poll() == None:
        print(p.stdout.readline(), end='')

##### STM32 build and flash process START #####
def build_stm32():
    # step into STM32CubeMX project folder
    os.chdir('../device/TMA-1')

    # set build environment variables
    os.environ["DEBUG"] = build_config["STM32"]["DEBUG"]
    os.environ["OPT"] = build_config["STM32"]["OPT"]

    # build
    spawn(['make'])

    # step back to builder
    os.chdir('../../builder')

    print("build completed! copying binary...")

    # move target executable
    if os.path.exists('../device/TMA-1/build/TMA-1.elf'):
        os.makedirs('./build', exist_ok=True)
        shutil.copyfile('../device/TMA-1/build/TMA-1.elf', './build/TMA-1.elf')

    else:
        print("ERROR: No built binary found!!!")

    print("binary copied to build/TMA-1.elf")

def flash_stm32():
    if os.path.exists('../device/TMA-1/TMA-1.cfg'):
        shutil.copyfile('../device/TMA-1/TMA-1.cfg', './TMA-1.cfg')

    else:
        print("ERROR: No OpenOCD config file found!!!")

    spawn(['openocd', '-f', './config/TMA-1.cfg'])
    return
##### STM32 build and flash process END #####

##### ESP32 build and flash process START #####
def build_esp32():
    return

def flash_esp32():
    return
##### ESP32 build and flash process END #####

# Monolith TMA-1 software build and flash process
def build():
    toolchain.validate()

    build_stm32()
    flash_stm32()

    build_esp32()
    flash_esp32()

# clean build directories
def clean():
    # build directory
    if os.path.exists('./build'):
        shutil.rmtree('./build')

    # STM32CubeMX build directory
    if os.path.exists('../device/TMA-1/build'):
        shutil.rmtree('../device/TMA-1/build')

if __name__ == "__main__":
    build()