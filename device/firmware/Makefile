LIBCLANG_PATH := $(HOME)/.rustup/toolchains/esp/xtensa-esp32-elf-clang/esp-18.1.2_20240912/esp-clang/lib
PATH := $(HOME)/.rustup/toolchains/esp/xtensa-esp-elf/esp-14.2.0_20240906/xtensa-esp-elf/bin:$(HOME)/.wokwi/bin:$(PATH)

build:
	cargo build --release

flash:
	cargo flash

sim: build
	wokwi-cli .
