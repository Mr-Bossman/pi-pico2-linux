FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates patch git make binutils gcc file wget cpio unzip rsync bc bzip2 g++ && \
	rm -rf /var/lib/apt/lists/* && \
	apt-get clean

RUN git clone --recurse-submodules https://github.com/Mr-Bossman/pi-pico2-linux

WORKDIR pi-pico2-linux
RUN make -C buildroot BR2_EXTERNAL=$PWD/ raspberrypi-pico2_defconfig && \
	make -C buildroot

CMD picotool load -fu buildroot/output/images/flash-image.uf2

# docker build -t pi-pico2-linux .
# docker run -v $(pwd):/root/ -it --entrypoint /bin/bash pi-pico2-linux
# docker run pi-pico2-linux
