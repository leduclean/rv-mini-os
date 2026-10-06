/**
 * @file
 * @brief [TODO:description]
 */

#include <stddef.h>
#include <stdint.h>

#include <lib/bit.h>

#include <asm/board.h>
#include <asm/cpu.h>

#include <drivers/pci.h>
#include <drivers/virtio_blk.h>

#include <kernel/pages.h>

#include "virtio.h"

#define VIRTIO_VENDOR_ID 0x1AF4
#define VIRTIO_BLK_DEV_ID 0x1042
#define VIRTIO_CAP_VENDOR 0x09

#define VIRTIO_MAX_QUEUE_SIZE 16
#define VIRTIO_QUEUE_SELECT 0

#define VIRTIO_BLK_SIZE 512 //< Block size in bytes

/**
 * @name Request device status
 *
 * @{*/
#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2
/** @}*/

struct virtio_blk_req_hdr {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector; /**< Offset multiplied by 512 for R/W. 0 for flush */
};

struct virtio_blk_req {
	struct virtio_blk_req_hdr hdr;
	uint8_t status;
};

/**
 * @brief Segment specified in data for Discard and Write Zeroes request.
 */
struct virtio_blk_discard_write_zeroes {
	uint64_t sector; /**< Starting offset. */
	uint32_t num_sectors; //< Number
	struct {
		uint32_t unmap : 1; /**< 0 for discard commands */
		uint32_t reserved : 31;
	} flags;
};

/**
 * @name Device dependant Feature Bit (select 0)
 *
 * @{*/
#define VIRTIO_BLK_F_SIZE_MAX \
	(1 << 1) /* Maximum size of any single segment is in size_max. */
#define VIRTIO_BLK_F_SEG_MAX \
	(1 << 2) /* Maximum number of segments in a request is in seg_max. */
#define VIRTIO_BLK_F_FLUSH (1 << 9) /* Cache flush command support. */
/** @}*/

struct virtio_blk_driver {
	struct pci_device dev;
	struct virtq vq;
	uint16_t free_head_idx;
	volatile uint16_t *notify_addr;
};

static struct virtio_blk_driver driver;

struct virtio_pci_cap *_get_virtio_cap(const struct pci_device *dev,
				       uint8_t cfg_type)
{
	struct pci_cap *cap = NULL;
	while ((cap = pci_find_cap(dev, cap, VIRTIO_CAP_VENDOR))) {
		struct virtio_pci_cap *vcap = (struct virtio_pci_cap *)cap;
		if (vcap->cfg_type == cfg_type) {
			return vcap;
		}
	}

	return NULL;
}

static int _set_notify_addr(uint16_t notify_off)
{
	int res;

	volatile struct virtio_pci_notify_cap *notify_cap =
		(struct virtio_pci_notify_cap *)_get_virtio_cap(
			&driver.dev, VIRTIO_PCI_CAP_NOTIFY_CFG);
	if (!notify_cap) {
		return -1;
	}

	uintptr_t bar_addr;
	res = pci_bar_addr(&driver.dev, notify_cap->cap.bar, &bar_addr);
	if (res < 0) {
		return res;
	}

	driver.notify_addr =
		(volatile uint16_t *)(bar_addr + notify_cap->cap.offset +
				      notify_cap->notify_off_multiplier *
					      notify_off);

	return 0;
}

/**
 * @brief Makes the internal free list.
 *
 * @warning Must be called after @p _setup_queues
 */
static void _init_virtio_desc(void)
{
	struct virtq_desc *head = driver.vq.desc;
	uint16_t queue_size = driver.vq.num;
	for (int i = 0; i < queue_size - 1; i++) {
		head[i].next = i + 1;
	}

	driver.free_head_idx = 0;
}

static int _setup_queues(volatile struct virtio_pci_common_cfg *cfg)
{
	cfg->queue_select = VIRTIO_QUEUE_SELECT;

	if (cfg->queue_size > VIRTIO_MAX_QUEUE_SIZE) {
		cfg->queue_size = VIRTIO_MAX_QUEUE_SIZE;
	}

	int n = cfg->queue_size;
	if (n == 0) {
		return -1;
	}
	driver.vq.num = n;

	uint64_t addr = (uint64_t)page_alloc();
	if (!addr) {
		return -1;
	}

	addr = ALIGN_UP(addr, VIRTQ_DESC_ALIGN);
	cfg->queue_desc = addr;
	driver.vq.desc = (struct virtq_desc *)(addr);
	addr += sizeof(struct virtq_desc) * n;

	addr = ALIGN_UP(addr, VIRTQ_AVAIL_ALIGN);
	cfg->queue_driver = (uint64_t)(addr);
	driver.vq.avail = (struct virtq_avail *)(addr);
	addr += sizeof(struct virtq_avail) * n;

	addr = ALIGN_UP(addr, VIRTQ_USED_ALIGN);
	cfg->queue_device = (uint64_t)(addr);
	driver.vq.used = (struct virtq_used *)(addr);

	cfg->queue_enable = 1;
	return 0;
}

static int _setup_features(volatile struct virtio_pci_common_cfg *cfg)
{
	cfg->device_status = 0;

	// Spin until read returns 0 before reinitializing
	while (cfg->device_status != 0)
		;
	cfg->device_status |= VIRTIO_STATUS_ACK;
	cfg->device_status |= VIRTIO_STATUS_DRIVER;

	cfg->device_feature_select = 0;
	uint32_t dev_feature = cfg->device_feature;

	cfg->driver_feature_select = 0;
	cfg->driver_feature = dev_feature &
			      (VIRTIO_BLK_F_FLUSH | VIRTIO_BLK_F_SEG_MAX |
			       VIRTIO_BLK_F_SIZE_MAX);

	cfg->device_feature_select = 1;
	cfg->driver_feature_select = 1;
	cfg->driver_feature = cfg->device_feature & VIRTIO_F_VERSION_1;

	cfg->device_status |= VIRTIO_STATUS_FEATURES_OK;

	if (!(cfg->device_status & VIRTIO_STATUS_FEATURES_OK)) {
		return -1;
	}

	return 0;
}

static int _config_common(void)
{
	int res;
	uintptr_t bar_addr;

	struct virtio_pci_cap *cap_cfg = _get_virtio_cap(
		&driver.dev, VIRTIO_PCI_CAP_COMMON_CFG);
	if (!cap_cfg) {
		return -1;
	}

	res = pci_bar_addr(&driver.dev, cap_cfg->bar, &bar_addr);
	if (res < 0) {
		return res;
	}

	volatile struct virtio_pci_common_cfg *cfg =
		(struct virtio_pci_common_cfg *)(bar_addr + cap_cfg->offset);

	res = _setup_features(cfg);
	if (res < 0) {
		return res;
	}

	res = _setup_queues(cfg);
	if (res < 0) {
		return res;
	}

	_init_virtio_desc();

	res = _set_notify_addr(cfg->queue_notify_off);
	if (res < 0) {
		return res;
	}

	cfg->device_status |= VIRTIO_STATUS_DRIVER_OK;

	if (cfg->device_status & VIRTIO_STATUS_FAILED) {
		panic("BOOT: Failed to start the block driver");
	}

	return 0;
}

static int _config_pci(void)
{
	struct pci_device dev;
	int res;

	if (!pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_BLK_DEV_ID, &dev)) {
		return -1;
	}
	// All virtio structures live in the BAR pointed by the caps.
	struct virtio_pci_cap *cap_cfg = _get_virtio_cap(
		&dev, VIRTIO_PCI_CAP_COMMON_CFG);
	if (!cap_cfg) {
		return -1;
	}

	res = pci_set_bar(&dev, cap_cfg->bar, VIRTIO_BLK_BASE_ADDRESS);
	if (res < 0) {
		return res;
	}
	pci_enable_device(&dev);
	pci_set_master(&dev);
	driver.dev = dev;

	return 0;
}

int virtio_blk_init(void)
{
	int res;
	res = _config_pci();
	if (res < 0) {
		return res;
	}

	res = _config_common();
	if (res < 0) {
		return res;
	}

	return 0;
}

static inline void _notify_device(void)
{
	*driver.notify_addr = VIRTIO_QUEUE_SELECT;
}

static void _submit_req(struct virtio_blk_req *req, uint8_t type,
			uint64_t sector, void *data, uint32_t size)
{
	req->hdr.type = type;
	req->hdr.sector = sector;

	struct virtq_avail *avail = driver.vq.avail;
	struct virtq_desc *desc = driver.vq.desc;
	uint16_t queue_size = driver.vq.num;

	uint16_t idx0 = driver.free_head_idx;
	uint16_t idx1 = desc[idx0].next;
	uint16_t idx2 = desc[idx1].next;
	uint16_t new_free_slot = desc[idx2].next;

	desc[idx0] = (struct virtq_desc){ .addr = (uint64_t)&req->hdr,
					  .len = sizeof(
						  struct virtio_blk_req_hdr),
					  .flags = VIRTQ_DESC_F_NEXT,
					  .next = idx1 };

	uint16_t data_flags = VIRTQ_DESC_F_NEXT;
	if (type == VIRTIO_BLK_T_IN) {
		data_flags |=
			VIRTQ_DESC_F_WRITE; // Device needs to write in our buffer
	}

	desc[idx1] = (struct virtq_desc){ .addr = (uint64_t)data,
					  .len = size,
					  .flags = data_flags,
					  .next = idx2 };

	desc[idx2] = (struct virtq_desc){
		.addr = (uint64_t)&req->status,
		.len = sizeof(req->status),
		.flags = VIRTQ_DESC_F_WRITE, // Device will write in status
		.next = 0
	};

	avail->ring[avail->idx % queue_size] = idx0;

	m2m_wmb();

	avail->idx++;
	driver.free_head_idx = new_free_slot;
}

/**
 * @brief Release a descriptor chain and claim status.
 *
 * @param head_idx Head idx of the chain.
 * @return The status state of the described operation.
 */
static uint8_t _release_desc_chain(uint16_t head_idx)
{
	struct virtq_desc *desc = driver.vq.desc;
	struct virtq_desc *curr = &desc[head_idx];

	while (curr->flags & VIRTQ_DESC_F_NEXT) {
		curr->flags = 0;
		curr = &desc[curr->next];
	};

	struct virtq_desc *status = curr;
	status->next = driver.free_head_idx;
	driver.free_head_idx = head_idx;

	return *(volatile uint8_t *)(status->addr);
}

int virtio_blk_request(uint8_t type, uint64_t sector, void *data, uint32_t size)
{
	struct virtio_blk_req req;
	volatile struct virtq_used *used = driver.vq.used;
	uint16_t last_seen = used->idx;

	_submit_req(&req, type, sector, data, size);
	m2io_wmb();
	_notify_device();

	// Pool
	//FIX: Stub, we will use interrupt
	while (last_seen == used->idx) {
		// Spin lock
	}

	volatile struct virtq_used_elem e =
		used->ring[last_seen % driver.vq.num];

	uint16_t status = _release_desc_chain(e.id);
	if (status == VIRTIO_BLK_S_IOERR) {
		panic("Block IO error");
	}
	if (status == VIRTIO_BLK_S_UNSUPP) {
		panic("Unsupported Virtio Block operations");
	}

	return e.len;
}
