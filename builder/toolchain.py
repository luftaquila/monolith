import os
import sys
import py7zr
import shutil
import requests
import subprocess

path = {
    'toolchain': './toolchain',
    'toolchain_abs' : os.path.abspath('./toolchain'),
    'cache': './toolchain/.cache',

    'openocd': './toolchain/openocd',
    'make': './toolchain/make',
    'arm': './toolchain/arm',

    'arduino': './toolchain/arduino',
}

# spawn a subprocess
def spawn(argv):
    p = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    while p.poll() == None:
        print(p.stdout.readline(), end='')

    return p.poll()

# format download size
def sizeof_fmt(num, suffix="B"):
    for unit in ("", "K", "M", "G", "T", "P", "E", "Z"):
        if abs(num) < 1024.0:
            return f"{num:3.1f} {unit}{suffix}"
        num /= 1024.0
    return f"{num:.1f} Y{suffix}"

# download file from web url
def download(name, path, url):
    with open(f'{path}/{name}', 'wb') as file:
        response = requests.get(url, allow_redirects=True, stream=True)
        size = response.headers.get('content-length')

        if size is None:
            print(f'  downloading {name}...')
            file.write(response.content)

        else:
            size = int(size)
            print(f'  downloading {name}... ({sizeof_fmt(int(size))})')
            progress = 0
            for data in response.iter_content(chunk_size=4096):
                progress += len(data)
                file.write(data)
                done = int(50 * progress / size)

                sys.stdout.write(f"\r  [{'=' * done}{' ' * (50 - done)}] {int(progress / size * 100)}% ")
                sys.stdout.flush()
            print()


##### GNU make for Windows configure process START #####
def config_make():
    print("installing GNU make for Windows...")

    try:
        if os.path.exists(path["make"]):
            print("  previous installation found. skipping installation...")

        else:
            if os.path.exists(f'{path["cache"]}/make_executable.zip') and os.path.exists(f'{path["cache"]}/make_dependency.zip'):
                print("  cached file detected. skipping download...")

            else:
                download('make_executable.zip', path["cache"], 'https://gnuwin32.sourceforge.net/downlinks/make-bin-zip.php')
                download('make_dependency.zip', path["cache"], 'https://gnuwin32.sourceforge.net/downlinks/make-dep-zip.php')

            print("  extracting GNU make for Windows...")

            os.makedirs(path["make"])

            shutil.unpack_archive(f'{path["cache"]}/make_dependency.zip', f'{path["cache"]}/make_dependency')
            os.rename(f'{path["cache"]}/make_dependency/bin', f'{path["make"]}/bin')

            shutil.unpack_archive(f'{path["cache"]}/make_executable.zip', f'{path["cache"]}/make_executable')
            os.rename(f'{path["cache"]}/make_executable/bin/make.exe', f'{path["make"]}/bin/make.exe')

            print("GNU make for Windows installed!")

        return 0

    except Exception as e:
        print(e)
        print("\nERROR: installation of GNU make for Windows failed.")
        return -1

##### GNU make for Windows configure process END #####


##### ARM GNU toolchain configure process START #####
def config_arm_toolchain():
    print("installing ARM GNU toolchain...")

    try:
        if os.path.exists(path["arm"]):
            print("  previous installation found. skipping installation...")

        else:
            if os.path.exists(f'{path["cache"]}/arm_toolchain.zip'):
                print("  cached file detected. skipping download...")

            else:
                download('arm_toolchain.zip', path["cache"], 'https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-mingw-w64-i686-arm-none-eabi.zip?rev=e6948d78806d4815912a858a6f6a85f6&hash=B20A83F31B9938D5EF819B14924A67E3')

            print("  extracting ARM GNU toolchain...")

            shutil.unpack_archive(f'{path["cache"]}/arm_toolchain.zip', f'{path["cache"]}/arm_toolchain')
            os.rename(f'{path["cache"]}/arm_toolchain/arm-gnu-toolchain-12.3.rel1-mingw-w64-i686-arm-none-eabi', path["arm"])

            print("ARM GNU toolchain installed!")

        return 0

    except Exception as e:
        print(e)
        print("\nERROR: installation of ARM GNU toolchain failed.")
        return -1

##### ARM GNU toolchain configure process END #####


##### OpenOCD for Windows configure process START #####
def config_openocd():
    print("installing OpenOCD for Windows...")

    try:
        if os.path.exists(path["openocd"]):
            print("  previous installation found. skipping installation...")

        else:
            if os.path.exists(f'{path["cache"]}/openocd.7z'):
                print("  cached file detected. skipping download...")

            else:
                download('openocd.7z', path["cache"], 'https://sysprogs.com/getfile/2060/openocd-20230712.7z')

            print("  extracting OpenOCD for Windows...")

            with py7zr.SevenZipFile(f'{path["cache"]}/openocd.7z', 'r') as archive:
                archive.extractall(path=path["toolchain"])
                os.rename(f'{path["toolchain"]}/OpenOCD-20230712-0.12.0', f'{path["toolchain"]}/openocd')

            print("OpenOCD for Windows installed!")

        return 0

    except Exception as e:
        print(e)
        print("\nERROR: installation of OpenOCD for Windows failed.")
        return -1

##### OpenOCD for Windows configure process END #####


##### Arduino-CLI configure process START #####
def config_arduino():
    print("installing arduino-cli...")

    try:
        if os.path.exists(path["arduino"]):
            print("  previous installation found. skipping installation...")

        else:
            if os.path.exists(f'{path["cache"]}/arduino-cli.zip'):
                print("  cached file detected. skipping download...")

            else:
                download('arduino-cli.zip', path["cache"], 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip')
            print("  extracting arduino-cli...")

            shutil.unpack_archive(f'{path["cache"]}/arduino-cli.zip', f'{path["cache"]}/arduino')
            os.rename(f'{path["cache"]}/arduino', path["arduino"])

            print("  configuring arduino-cli...")

            os.environ["PATH"] = os.getenv("PATH") + f';{path["toolchain_abs"]}\\arduino;'

            spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'core', 'update-index'])
            spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'core', 'install', 'esp32:esp32@2.0.17'])
            spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'lib', 'install', 'ArduinoJson@7.1.0'])
            spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'lib', 'install', 'RingBuffer@1.0.5'])
            spawn(['arduino-cli', '--config-file', './config/arduino-cli.yaml', 'lib', 'install', 'WebSockets@2.4.1'])

            print("arduino-cli configured!")

        return 0

    except Exception as e:
        print(e)
        print("\nERROR: installation of arduino-cli failed.")
        return -1

##### Arduino-CLI configure process END #####


# toolchain installation process
def config(target):
    os.makedirs(path["cache"], exist_ok=True)

    ret = 0

    if target == 'stm':
        ret |= config_arm_toolchain()
        ret |= config_make()
        ret |= config_openocd()

    elif target == 'esp':
        ret |= config_arduino()

    else:
        return -1

    return ret

# check toolchain installation
def validate(target):
    if target == 'stm':
        if not os.path.exists(path["arm"]) or \
           not os.path.exists(path["make"]) or \
           not os.path.exists(path["openocd"]):
            print('WARN: missing toolchain detected! Installing them first...')

            if config(target) != 0:
                print("\nERROR: toolchain installation failed. terminating.")
                return -1

        print('INFO: toolchain validation completed!\n')

        # configure toolchain PATH
        os.environ["PATH"] = os.getenv("PATH") + ';' + \
                                f'{path["toolchain_abs"]}\\arm\\bin;' + \
                                f'{path["toolchain_abs"]}\\make\\bin;' + \
                                f'{path["toolchain_abs"]}\\openocd\\bin'


    elif target == 'esp':
        if not os.path.exists(path["arduino"]):
            print('WARN: missing toolchain detected! Installing them first...')

        if config(target) != 0:
            print("\nERROR: toolchain installation failed. terminating.")
            return -1

        print('INFO: toolchain validation completed!\n')

        # configure toolchain PATH
        os.environ["PATH"] = os.getenv("PATH") + ';' + f'{path["toolchain_abs"]}\\arduino'

    else:
        print("\nERROR: wrong target. terminating.")
        return -1

    return 0

# clean toolchain directories
def clean():
    if os.path.exists(path["toolchain"]):
        shutil.rmtree(path["toolchain"])

# clean cached files
def clean_cache():
    if os.path.exists(path["cache"]):
        shutil.rmtree(path["cache"])


if __name__ == "__main__":
    config()
