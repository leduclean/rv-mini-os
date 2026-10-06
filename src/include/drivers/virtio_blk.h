#pragma once
#include <stdint.h>

/**
 * @brief [TODO:description]
 *
 * @return [TODO:return]
 */
int virtio_blk_init(void);

//TODO: doc
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_T_DISCARD 11
#define VIRTIO_BLK_T_WRITE_ZEROES 13

int virtio_blk_request(uint8_t type, uint64_t sector, void *data,
		       uint32_t size);
