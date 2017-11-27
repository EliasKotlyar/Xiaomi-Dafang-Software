#ifndef __TX_ISP_VIDEOBUF_H__
#define __TX_ISP_VIDEOBUF_H__

#include <media/videobuf2-core.h>
#include <media/videobuf2-memops.h>
#include <linux/dma-mapping.h>
#include <mach/tx_isp.h>

/*#define TX_ISP_FRAME_CHANNEL_BUFFER_MAX         (26 * 1024 * 1024)*/

// If the system mem is smaller, reserved 4M mem.
#define TX_ISP_FRAME_CHANNEL_BUFFER_MAX         (4 * 1024 * 1024)

extern const struct vb2_mem_ops frame_channel_vb2_memops;

struct vb2_mmap_conf {
	struct device			*dev;
	void				*vaddr;
	dma_addr_t			paddr;
	unsigned long			size;
	unsigned long			used;
};


struct vb2_dc_conf {
	struct device		*dev;

	/* MMAP related */
	int mmap_enable;
	struct vb2_mmap_conf	mmap;

	/* USERPTR related */
	int tlb_enable;
	void 			*tlb;

	struct mutex mlock;
};

struct vb2_dc_buf {
	struct vb2_dc_conf		*conf;
	void				*vaddr;
	dma_addr_t			paddr;
//	enum dma_data_direction		dma_dir;
	unsigned long			size;

	/* MMAP related */
	struct vb2_vmarea_handler	handler;
	atomic_t			refcount;

	/* USERPTR related */
	struct vm_area_struct		*vma;
};

void *frame_buffer_manager_create(struct device *dev);
int frame_buffer_manager_cleanup(void *alloc_ctx);

#endif/* __TX_ISP_VIDEOBUF_H__ */
