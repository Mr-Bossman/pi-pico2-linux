FROM debian:13

RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates patch git \
	make binutils gcc file wget cpio unzip rsync bc bzip2 g++ picotool && \
	rm -rf /var/lib/apt/lists/* && \
	apt-get clean

ARG BRANCH="master"
RUN git clone --recurse-submodules --single-branch --depth 1 -b ${BRANCH} \
	https://github.com/Mr-Bossman/pi-pico2-linux

WORKDIR pi-pico2-linux

ARG THREADS=1
RUN make -C buildroot BR2_EXTERNAL=$PWD/ raspberrypi-pico2_defconfig && \
	make -C buildroot -j${THREADS}

CMD picotool load -fu buildroot/output/images/flash-image.uf2

# To build binary:
# docker build --build-arg THREADS=1 -t pi-pico2-linux .
# To flash the pico2:
# docker run pi-pico2-linux
# To copy built files in/out of the container:
# docker run -v $(pwd):/root/ -it --entrypoint /bin/bash pi-pico2-linux
