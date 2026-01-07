#include "image.h"

#include <stdint.h>
#include <stddef.h>

#ifdef PICO_SDK
#include <stdio.h>
#include <string.h>
#else
#include "stdio_local.h"
#include "string_local.h"
#endif
#include "byteswap.h"

/* ASCII version of "RSC\0x5" defined in Linux kernel */
#define LINUX_RISCV_IMAGE_MAGIC 0x05435352
#define FDT_MAGIC 0xd00dfeed

struct linux_image_h {
	uint32_t	code0;		/* Executable code */
	uint32_t	code1;		/* Executable code */
	uint64_t	text_offset;	/* Image load offset */
	uint64_t	image_size;	/* Effective Image size */
	uint64_t	flags;		/* kernel flags (little endian) */
	uint32_t	version;	/* version of the header */
	uint32_t	res1;		/* reserved */
	uint64_t	res2;		/* reserved */
	uint64_t	res3;		/* reserved */
	uint32_t	magic;		/* Magic number */
	uint32_t	res4;		/* reserved */
};

size_t get_kernel_size(const void *data) {
	const uint32_t magic = LINUX_RISCV_IMAGE_MAGIC;
	const struct linux_image_h *lhdr = data;
	uint64_t image_size;

	/* memcmp and memcpy used to avoid unaligned accesses */
	if (memcmp(&lhdr->magic, &magic, sizeof(lhdr->magic))) {
		puts("Bad Linux RISCV Image magic!\n");
		return 0;
	}

	memcpy(&image_size, &lhdr->image_size, sizeof(image_size));
	if (image_size == 0)
		puts("Image lacks image_size field, error!\n");

	return image_size;
}

size_t get_devicetree_size(const void *data) {
	const uint32_t *data_ptr = (const uint32_t *)data;
	const uint32_t magic = htobe32(FDT_MAGIC);
	uint32_t fdt_size;

	/* memcmp and memcpy used to avoid unaligned accesses */
	if (memcmp(&data_ptr[0], &magic, sizeof(data_ptr[0]))) {
		puts("Bad Device Tree magic!\n");
		return 0;
	}

	memcpy(&fdt_size, &data_ptr[1], sizeof(data_ptr[1]));
	if (fdt_size == 0)
		puts("Device Tree size is 0, error!\n");

	return be32toh(fdt_size);
}
