import os
import shutil
import json
import subprocess

import toolchain

toolchain_path = os.path.abspath('./toolchain')

build_config = ''
with open("build_config.json", "r") as file:
    build_config = json.load(file)

def spawn(argv):
    p = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    while p.poll() == None:
        print(p.stdout.readline(), end='')

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

    print("binary copied to build/TMA-1.elf")


def build_esp32():
    return

def upload_stm32():
    openocd = subprocess.Popen(['openocd', '-f' '../device/TMA-1/TMA-1.cfg'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    gdb = subprocess.Popen(['arm-none-eabi-gdb', '-x', 'gdb.txt'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    
    while True:
        if openocd.poll() == None:
            print(openocd.stdout.readline(), end='')

        if gdb.poll() == None:
            print(gdb.stdout.readline(), end='')

        if openocd.poll() != None and gdb.poll != None:
            break

    return

def upload_esp32():
    return

def build():
    # install toolchain
    if not os.path.exists(toolchain.path_make) or \
       not os.path.exists(toolchain.path_arm) or \
       not os.path.exists(toolchain.path_openocd):
        print('Missing toolchain detected! Installing them first...')
        toolchain.config()
        print('Toolchain installation completed!')

    # configure toolchain PATH
    os.environ["PATH"] = os.getenv("PATH") + f'{toolchain_path}\\arm\\bin;{toolchain_path}\\make\\bin;{toolchain_path}\\openocd\\bin;'

    build_stm32()
    upload_stm32()

    build_esp32()
    upload_esp32()

def clean():
    if os.path.exists('./build'):
        shutil.rmtree('./build')

if __name__ == "__main__":
    build()