#pragma once
#include <stdint.h>

#define VIRTIO_BLK_SIZE 512 /**< Block size in bytes */

/**
 * @name Virtio Block command type
 *
 * @{*/
#define VIRTIO_BLK_T_IN 0 /**< Disk Write request */
#define VIRTIO_BLK_T_OUT 1 /**< Disk Read request */
/** @}*/

#define VIRTIO_BLK_PLIC_ID 33 /** Obtained via the dumpdtb and pci mapping */

/**
 * @brief Init the virtio blk driver.
 *
 * @return 0 on SUCCESS, error code < 0 on FAILURE.
 */
int virtio_blk_init(void);

/**
 * @brief Emit a request to the block device. 
 * @note This is a blocking function waiting for device ack.
 *
 * @param type Type of disk request.
 * @param sector The sector offset in 512 bytes block.
 * @param data A pointer to the used buffer.
 * @param size The size of the buffer.
 * @return The written len on SUCCESS, error code < 0 on FAILURE.
 */
int virtio_blk_request(uint8_t type, uint64_t sector, void *data,
		       uint32_t size);

/**
 * @brief Handles the irq of the device.
 */
void virtio_irq_handler(void);
