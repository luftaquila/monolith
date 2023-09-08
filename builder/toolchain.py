import os
import sys
import py7zr
import shutil
import requests

path_toolchain = './toolchain'
path_toolchain_abs = os.path.abspath('./toolchain')

path_cache = './toolchain/.cache'

path_openocd = './toolchain/openocd'
path_make = './toolchain/make'
path_arm = './toolchain/arm'

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
        if os.path.exists(f'{path_cache}/make_executable.zip') and os.path.exists(f'{path_cache}/make_dependency.zip'):
            print("  cached file detected. skipping download...")

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
        return 0

    except Exception as e:
        print("\nERROR: installation of GNU make for Windows failed.")
        return -1

##### GNU make for Windows configure process END #####


##### ARM GNU toolchain configure process START #####
def config_arm_toolchain():
    print("installing ARM GNU toolchain...")

    try:
        if os.path.exists(f'{path_cache}/arm_toolchain.zip'):
            print("  cached file detected. skipping download...")

        else:
            download('arm_toolchain.zip', path_cache, 'https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip?rev=8f4a92e2ec2040f89912f372a55d8cf3&hash=8A9EAF77EF1957B779C59EADDBF2DAC118170BBF')

        print("  extracting ARM GNU toolchain...")

        if os.path.exists(path_arm):
            shutil.rmtree(path_arm)

        shutil.unpack_archive(f'{path_cache}/arm_toolchain.zip', f'{path_cache}/arm_toolchain')
        os.rename(f'{path_cache}/arm_toolchain/gcc-arm-none-eabi-10.3-2021.10', path_arm)
            
        print("ARM GNU toolchain installed!")
        return 0

    except Exception as e:
        print("\nERROR: installation of ARM GNU toolchain failed.")
        return -1

##### ARM GNU toolchain configure process END #####


##### OpenOCD for Windows configure process START #####
def config_openocd():
    print("installing OpenOCD for Windows...")

    try:
        if os.path.exists(f'{path_cache}/openocd.7z'):
            print("  cached file detected. skipping download...")

        else:
            download('openocd.7z', path_cache, 'https://sysprogs.com/getfile/2060/openocd-20230712.7z')

        print("  extracting OpenOCD for Windows...")

        if os.path.exists(path_openocd):
            shutil.rmtree(path_openocd)

        with py7zr.SevenZipFile(f'{path_cache}/openocd.7z', 'r') as archive:
            archive.extractall(path=path_toolchain)
            os.rename(f'{path_toolchain}/OpenOCD-20230712-0.12.0', f'{path_toolchain}/openocd')
            
        print("OpenOCD for Windows installed!")
        return 0

    except Exception as e:
        print("\nERROR: installation of OpenOCD for Windows failed.")
        return -1

##### OpenOCD for Windows configure process END #####

# toolchain installation process
def config():
    os.makedirs(path_cache, exist_ok=True)
    
    ret = 0

    ret |= config_arm_toolchain()
    ret |= config_make()
    ret |= config_openocd()

    return ret

# check toolchain installation
def validate():
    if not os.path.exists(path_arm) or \
       not os.path.exists(path_make) or \
       not os.path.exists(path_openocd):
        print('WARN: missing toolchain detected! Installing them first...')

        if config() != 0:
            print("\nERROR: toolchain installation failed. terminating.")
            return -1

        print('INFO: toolchain installation completed!\n')

    # configure toolchain PATH
    os.environ["PATH"] = os.getenv("PATH") + \
                            f'{path_toolchain_abs}\\arm\\bin;' + \
                            f'{path_toolchain_abs}\\make\\bin;' + \
                            f'{path_toolchain_abs}\\openocd\\bin;'

    return 0

# clean toolchain directories
def clean():
    if os.path.exists(path_toolchain):
        shutil.rmtree(path_toolchain)

# clean cached files
def clean_cache():
    if os.path.exists(path_cache):
        shutil.rmtree(path_cache)


if __name__ == "__main__":
    config()