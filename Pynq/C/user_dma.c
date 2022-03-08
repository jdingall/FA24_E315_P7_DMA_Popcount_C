////////////////////////////////////////////////////////////
//    
//    Example popcount with FPGA and DMA support
//
//    Build for Indiana University's E315 class
//
//    @author:  Andrew Lukefahr <lukefahr@iu.edu>
//    @date:  20200331
//    @date:  20190320
//
////////////////////////////////////////////////////////////
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef NODEBUG
#define dprintf(...) do {\
			fprintf(stderr, "DEBUG: %s:%d: ", __FILE__, __LINE__); \
			fprintf(stderr, __VA_ARGS__); \
			} while (0)
#else
#define dprintf(...) 
#endif

#define eprintf(...) do { \
			fprintf (stderr, "ERROR: %s:%d: ", __FILE__, __LINE__); \
			fprintf (stderr, __VA_ARGS__); \
			exit(EXIT_FAILURE); \
			} while (0)

// size found in /sys/class/uio/uio0/maps/map0/size
const uint32_t UIO_SIZE = 0x00010000;

// offsets found in Vivado's Block Design's Address Map
const uint32_t DMA_OFFSET = 0x00001000; 
// found in Vivado's Block Design's DMA module
const uint32_t DMA_MAX_SIZE = 256 * 32;

// Documented in: 
// AXI DMA v7.1LogiCORE IP Product Guide
// https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf
volatile uint32_t * MM2S_DMACR;
volatile uint32_t * MM2S_DMASR;
volatile uint32_t * MM2S_SA;
volatile uint32_t * MM2S_LENGTH;

static inline
void setup_dma( const uint8_t * const BASE_VADDR)
{
	dprintf("Setting UIO mapping with VBASE=%p\n", BASE_VADDR);

	MM2S_DMACR	= (uint32_t*) (BASE_VADDR + DMA_OFFSET + 0x0 	);
	MM2S_DMASR 	= (uint32_t*) (BASE_VADDR + DMA_OFFSET + 0x4 	);
	MM2S_SA 	= (uint32_t*) (BASE_VADDR + DMA_OFFSET + 0x18	);
	MM2S_LENGTH	= (uint32_t*) (BASE_VADDR + DMA_OFFSET + 0x28	);
}



// NOTE: call AFTER setup_dma
static
void dma_tx( const uint32_t * const SRC_PADDR, const uint32_t SRC_SIZE )
{
	
	// Following "Programming Sequence - Direct Register Mode (Simple DMA)" of
	// https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf
	
	dprintf("Starting DMA transfer: %d bytes from %p\n", SRC_SIZE, SRC_PADDR);
    
    //TODO: Steps 1,3, and 4
	
	//Step 1:  Start MM2S Channel RS bit
	
    //spin until halt bit deasserts
	
	//Step 2 (Interrupts) skipped
	
	//Step 3:  Set Source Address

	//Step 4:  Set Number of Bytes to transfer
	// From Docs:  Writing a non-zero value to this register starts the MM2S transfer.

	//Wait for completion

	dprintf("DMA transfer complete\n");

}	

uint8_t * get_uio_vaddr()
{
    // open+mmap UIO	
	dprintf("Mapping UIO0\n");
	int uio_fd = open("/dev/uio0", O_RDWR|O_SYNC);
	if (uio_fd < 0) eprintf("Error opening /dev/uio0. \n");
	uint8_t * uio= (uint8_t *) mmap(0x0, UIO_SIZE, PROT_READ|PROT_WRITE, 
					MAP_SHARED, uio_fd, 0x0);
	if (uio == NULL) eprintf("Error mmap-ing /dev/uio0.\n");
	close(uio_fd);
    
    return uio;
}

uint8_t * get_udma_vaddr( uint32_t udma_size)
{
	//open+mmap UDMABUF
	dprintf("Mapping UDMABUF0\n");
	int udma_fd = open("/dev/udmabuf0", O_RDWR|O_SYNC);
	if (udma_fd < 0) eprintf("Error opening /dev/udmabuf0. \n");
	uint8_t * udma_buf = (uint8_t *) mmap(0x0, udma_size, PROT_READ|PROT_WRITE, 
					MAP_SHARED, udma_fd, 0x0);
	if (udma_buf == NULL) eprintf("Error mmap-ing /dev/udmabuf0.\n");
	close(udma_fd);

    return udma_buf;
}    

uint8_t * get_udma_paddr()
{
    //look up physical address of udmabuf0
	dprintf("Loading UDMABUF0 Physical Address\n");
	int paddr_fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", 
                O_RDONLY);
	if (paddr_fd < 0) eprintf("Error opening phys_addr.\n");
	char pbuf[100];
	ssize_t bytes = read(paddr_fd, &pbuf, 100);
	if (bytes == 0) eprintf("Error reading phys_addr\n");
	uint8_t * udma_paddr = (uint8_t*) strtol(pbuf, NULL, 16);  //hex
	dprintf("udmabuf0 Phys Addr: %p\n", udma_paddr);
	close(paddr_fd);

    return udma_paddr;
}

uint32_t get_udma_size()
{
    //look up size of udmabuf0
	dprintf("Loading UDMABUF0 Size\n");
	int size_fd = open("/sys/class/u-dma-buf/udmabuf0/size", O_RDONLY);
	if (size_fd < 0) eprintf("Error opening size.\n");
	//reuse pbuf and bytes
	char pbuf[100];
	ssize_t bytes = read(size_fd, &pbuf, 100);
	if (bytes == 0) eprintf("Error reading size\n");
	uint32_t udma_size = strtol(pbuf, NULL, 10);  //decimal
	dprintf("udmabuf0 Size: %d\n", udma_size);
	close(size_fd);
    
    return udma_size;
}

int main(int argc, char **argv)
{
	
	// check for input file
	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_filename\n", argv[0]);
		exit(1);
	}

    uint32_t max_udma_size = get_udma_size();
    //use whichever is the smallest, the DMA buffer or the DMA Transfer Burst Length
    uint32_t udma_size = (max_udma_size > DMA_MAX_SIZE ?  DMA_MAX_SIZE : max_udma_size);

    uint8_t * udma_buf_paddr = get_udma_paddr();
    uint8_t * udma_buf_vaddr = get_udma_vaddr(udma_size);

    uint8_t * uio_vaddr = get_uio_vaddr();

	dprintf("Setting UIO mapping\n");
    uint32_t * reset_reg = (uint32_t*) uio_vaddr;
    uint32_t * count_reg = (uint32_t*) (uio_vaddr + 0x4);
	setup_dma( uio_vaddr);

	dprintf("Opening input file: %s\n", argv[1]);	
    //open the input file 
	int input_fd = open(argv[1], O_RDONLY);
	if (input_fd < 0) {
		fprintf(stderr, "open() %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}

	dprintf("Resetting Count\n");
	*reset_reg = 1;

	dprintf("Reading input file\n");
    uint8_t * data_ptr;
	ssize_t buf_cnt;
    
    //TODO: read file in chunks of udma_size and send the chunks to popcount module using dma_tx function

	
    
    
    
	// Read the bit count result
	printf("Counted %u ones\n", *count_reg);

	dprintf("Cleaning up\n"); 
	close(input_fd);
	munmap(uio_vaddr, UIO_SIZE);
	munmap(udma_buf_vaddr, udma_size);
	return 0;
}

