# Raspberry Pi Pico 2 Buildroot

How to build:

```bash
git clone https://github.com/Mr-Bossman/pi-pico2-linux

cd pi-pico2-linux

git submodule update --init

cd buildroot

make BR2_EXTERNAL=$PWD/../ raspberrypi-pico2_defconfig

make

sudo dd status=progress oflag=sync bs=4k if=output/images/sdcard.img of=/dev/sdX; sync
```
