FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates patch git make binutils gcc g++ file wget cpio unzip rsync bc bzip2 g++ cmake python3 && \
	rm -rf /var/lib/apt/lists/* && \
	apt-get clean

RUN wget https://github.com/raspberrypi/pico-sdk-tools/releases/download/v2.0.0-5/riscv-toolchain-14-x86_64-lin.tar.gz && \
	mkdir -p ~/toolchain && \
	tar -vxzf riscv-toolchain-14-x86_64-lin.tar.gz -C ~/toolchain && \
	rm riscv-toolchain-14-x86_64-lin.tar.gz

RUN git clone --recurse-submodules https://github.com/Mr-Bossman/pi-pico2-linux

WORKDIR pi-pico2-linux
RUN make -C buildroot BR2_EXTERNAL=$PWD/ raspberrypi-pico2_defconfig && \
	make -C buildroot && \
	PICO_TOOLCHAIN_PATH=~/toolchain/ PICO_SDK_FETCH_FROM_GIT=1 make -C psram-bootloader

CMD make -C psram-bootloader flash-kernel

# docker build -t pi-pico2-linux .
# docker run -v $(pwd):/root/ -it --entrypoint /bin/bash pi-pico2-linux
# docker run pi-pico2-linux
