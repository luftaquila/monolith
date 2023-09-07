import os
import sys
import py7zr
import shutil
import requests

path_cache = './toolchain/.cache'
path_toolchain = './toolchain'
path_openocd = './toolchain/openocd'
path_make = './toolchain/make'
path_arm = './toolchain/arm'

def download(name, path, url):
    with open(f'{path}/{name}', 'wb') as file:
        response = requests.get(url, allow_redirects=True, stream=True)
        size = response.headers.get('content-length')

        if size is None:
            print(f'  downloading {name}...')
            file.write(response.content)

        else:
            size = int(size)
            print(f'  downloading {name}... ({int(size / 1024)} KB)')
            progress = 0
            for data in response.iter_content(chunk_size=4096):
                progress += len(data)
                file.write(data)
                done = int(50 * progress / size)

                sys.stdout.write(f"\r  [{'=' * done}{' ' * (50 - done)}] {int(progress / size * 100)}% ")
                sys.stdout.flush()
            print()

def config_make():
    print("installing GNU make for Windows...")

    if os.path.exists(f'{path_cache}/make_executable.zip') and os.path.exists(f'{path_cache}/make_dependency.zip'):
        print("  detected cached file. skipping download...")

    else:
        download('make_executable.zip', path_cache, 'https://gnuwin32.sourceforge.net/downlinks/make-bin-zip.php')
        download('make_dependency.zip', path_cache, 'https://gnuwin32.sourceforge.net/downlinks/make-dep-zip.php')

    print("  extracting GNU make for Windows...")

    if os.path.exists(path_make):
        shutil.rmtree(path_make)
    os.makedirs(path_make)

    shutil.unpack_archive(f'{path_cache}/make_dependency.zip', f'{path_cache}/make_dependency')
    os.rename(f'{path_cache}/make_dependency/bin', f'{path_make}/bin')

    shutil.unpack_archive(f'{path_cache}/make_executable.zip', f'{path_cache}/make_executable')
    os.rename(f'{path_cache}/make_executable/bin/make.exe', f'{path_make}/bin/make.exe')

    print("GNU make for Windows installed!")

def config_arm_toolchain():
    print("installing ARM GNU toolchain...")

    if os.path.exists(f'{path_cache}/arm_toolchain.zip'):
        print("  detected cached file. skipping download...")

    else:
        download('arm_toolchain.zip', path_cache, 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip?rev=8f4a92e2ec2040f89912f372a55d8cf3&hash=8A9EAF77EF1957B779C59EADDBF2DAC118170BBF')

    print("  extracting ARM GNU toolchain...")

    if os.path.exists(path_arm):
        shutil.rmtree(path_arm)

    shutil.unpack_archive(f'{path_cache}/arm_toolchain.zip', f'{path_cache}/arm_toolchain')
    os.rename(f'{path_cache}/arm_toolchain/gcc-arm-none-eabi-10.3-2021.10', path_arm)
        
    print("ARM GNU toolchain installed!")

def config_openocd():
    print("installing OpenOCD for Windows...")

    if os.path.exists(f'{path_cache}/openocd.7z'):
        print("  detected cached file. skipping download...")

    else:
        download('openocd.7z', path_cache, 'https://sysprogs.com/getfile/2060/openocd-20230712.7z')

    print("  extracting OpenOCD for Windows...")

    if os.path.exists(path_openocd):
        shutil.rmtree(path_openocd)

    with py7zr.SevenZipFile(f'{path_cache}/openocd.7z', 'r') as archive:
        archive.extractall(path=path_toolchain)
        os.rename(f'{path_toolchain}/OpenOCD-20230712-0.12.0', f'{path_toolchain}/openocd')
        
    print("OpenOCD for Windows installed!")


def config():
    os.makedirs(path_cache, exist_ok=True)

    config_make()
    config_arm_toolchain()
    config_openocd()

if __name__ == "__main__":
    config()