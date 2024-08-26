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

## Designed to work with [SparkFun Pro Micro - RP2350](https://www.sparkfun.com/products/24870)
