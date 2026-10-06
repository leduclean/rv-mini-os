#pragma once

#include <stddef.h>
#include <stdint.h>

#include <drivers/pci.h>

/**
 * @name Device independent feature bits (feature_select 1)
 * Bit 32 of the 64-bit feature set, i.e. bit 0 of the high word.
 * @{
 */
#define VIRTIO_F_VERSION_1 (1 << 0) /**< Compliant with virtio 1.x */
/** @} */

/**
 * @name virtio_pci_cap.cfg_type values.
 * @{
 */
#define VIRTIO_PCI_CAP_COMMON_CFG 1 /**< Common configuration */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2 /**< Notifications */
#define VIRTIO_PCI_CAP_ISR_CFG 3 /**< ISR status */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4 /**< Device specific configuration */
#define VIRTIO_PCI_CAP_PCI_CFG 5 /**< PCI configuration access */
/** @} */

/**
 * @brief Virtio vendor specific PCI capability.
 */
struct virtio_pci_cap {
	struct pci_cap cap;
	uint8_t cfg_type; /**< Identifies the structure */
	uint8_t bar; /**< BAR where to find it */
	uint8_t padding[3]; /**< Pad to full dword */
	uint32_t offset; /**< Offset within the BAR */
	uint32_t length; /**< Length of the structure, in bytes */
};

/**
 * @brief Notify capability, followed by the multiplier.
 */
struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	uint32_t notify_off_multiplier; /**< Multiplier for queue_notify_off */
};

/**
 * @name virtio_pci_common_cfg.device_status bits
 * @{
 */
#define VIRTIO_STATUS_ACK (1 << 0) /**< Guest has noticed the device */
#define VIRTIO_STATUS_DRIVER (1 << 1) /**< Guest knows how to drive it */
#define VIRTIO_STATUS_DRIVER_OK (1 << 2) /**< Driver is ready */
#define VIRTIO_STATUS_FEATURES_OK (1 << 3) /**< Feature negotiation done */
#define VIRTIO_STATUS_FAILED (1 << 7) /**< Driver gave up on the device */
/** @} */

/** 
 * @brief Common configuration structure, located in a BAR.
 */
struct virtio_pci_common_cfg {
	/* About the whole device. */
	uint32_t device_feature_select; /**< read-write */
	uint32_t device_feature; /**< read-only for driver */
	uint32_t driver_feature_select; /**< read-write */
	uint32_t driver_feature; /**< read-write */
	uint16_t msix_config; /**< read-write */
	uint16_t num_queues; /**< read-only for driver */
	uint8_t device_status; /**< read-write */
	uint8_t config_generation; /**< read-only for driver */

	/* About a specific virtqueue, selected by queue_select. */
	uint16_t queue_select; /**< read-write */
	uint16_t queue_size; /**< read-write */
	uint16_t queue_msix_vector; /**< read-write */
	uint16_t queue_enable; /**< read-write */
	uint16_t queue_notify_off; /**< read-only for driver */
	uint64_t queue_desc; /**< read-write */
	uint64_t queue_driver; /**< read-write */
	uint64_t queue_device; /**< read-write */
};

/**
 * @name virtq_desc.flags values
 * @{
 */
#define VIRTQ_DESC_F_NEXT 1 /**< Buffer continues via the next field */
#define VIRTQ_DESC_F_WRITE 2 /**< Device write-only (else read-only) */
/** @} */

/** @name Virtqueue part alignments (virtio spec §2.7) @{ */
#define VIRTQ_DESC_ALIGN 16
#define VIRTQ_AVAIL_ALIGN 2
#define VIRTQ_USED_ALIGN 4
/** @} */

/** 
 * @brief Descriptor table entry, 16-byte aligned.
 */
struct virtq_desc {
	uint64_t addr; /**< Guest-physical address */
	uint32_t len; /**< Length */
	uint16_t flags; /**< VIRTQ_DESC_F_* */
	uint16_t next; /**< Next descriptor if flags & NEXT */
};

/** virtq_avail.flags: don't interrupt when consuming a buffer. */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

/** 
 * @brief Available ring (driver -> device), 2-byte aligned.
 */
struct virtq_avail {
	uint16_t flags; /**< VIRTQ_AVAIL_F_* */
	uint16_t idx; /**< Where the driver puts the next entry */
	uint16_t ring[]; /**< Idx Heads of descriptor chains */
};

/** 
 * @brief Used ring element.
 */
struct virtq_used_elem {
	uint32_t id; /**< Index of start of used descriptor chain */
	uint32_t len; /**< Bytes written into the device writable part */
};

/** virtq_used.flags: don't notify when adding a buffer. */
#define VIRTQ_USED_F_NO_NOTIFY 1

/** 
 * @brief Used ring (device -> driver), 4-byte aligned. 
 */
struct virtq_used {
	uint16_t flags; /**< VIRTQ_USED_F_* */
	uint16_t idx; /**< Where the device puts the next entry */
	struct virtq_used_elem ring[]; /**< Completed descriptor chains */
};

/**
 * @brief Driver side view of a virtqueue.
 */
struct virtq {
	unsigned int num; /**< Number of entries */
	struct virtq_desc *desc; /**< Table of descriptor */
	struct virtq_avail *avail;
	struct virtq_used *used;
};
