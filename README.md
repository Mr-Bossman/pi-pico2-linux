# Raspberry Pi Pico 2 Buildroot

How to build:

```bash
git clone https://github.com/Mr-Bossman/pi-pico2-linux

cd pi-pico2-linux

git submodule update --init

cd buildroot

make BR2_EXTERNAL=$PWD/../ raspberrypi-pico2_defconfig

make

cd ../psram-bootloader

make flash-kernel
```

#### NOTES on SDK
This project uses the sdk and tools installed to `~/.pico-sdk/`.
To use the tools and sdk installed to another location see [this issue](https://github.com/raspberrypi/pico-sdk/pull/1820#issuecomment-2291611448)
or the following.

```bash
export PICO_SDK_PATH=~/example_sdk/sdk/2.0.0/
export PICO_TOOLCHAIN_PATH=~/example_sdk/toolchain/13_2_Rel1
export pioasm_DIR=~/example_sdk/tools/2.0.0/pioasm
export picotool_DIR=~/example_sdk/picotool/2.0.0/picotool

make flash-kernel
```

## Designed to work with [SparkFun Pro Micro - RP2350](https://www.sparkfun.com/products/24870)

### Use on other boards

This only works on the RP2350 RISC-V cores.

If you want to run this on other boards, please change the `PICO_BOARD` variable in `CMakeLists.txt`.
`set(PICO_BOARD sparkfun_promicro_rp2350 CACHE STRING "Board type")`

You will also need to set the psram CS pin with the `SFE_RP2350_XIP_CSI_PIN` macro in `bootloader.c`.
As of now the only psram chip tested is the `APS6404L` and any others may not work.
