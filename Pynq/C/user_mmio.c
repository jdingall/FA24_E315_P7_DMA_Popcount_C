////////////////////////////////////////////////////////////
//    
//    Example popcount with FPGA and MMIO support
//    Requires to access /dev/uio0
//
//    Build for Indiana University's E315 class
//
//    @author:  Andrew Lukefahr <lukefahr@iu.edu>
//    @date:  20200331
//
////////////////////////////////////////////////////////////
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


int main(int argc, char **argv)
{

    int dev_mem_fd = -1;
    void * vaddr_base;

    // check for input file
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input_filename\n", argv[0]);
        exit(1);
    }

    //Mapping user-space I/O
    dev_mem_fd = open("/dev/uio0", O_RDWR|O_SYNC);
    if (dev_mem_fd < 0) { perror("open() /dev/uio0"); return 1; }

    // Map 1KB of physical memory starting at 0x40000000
    // to 1KB of virtual memory starting at vaddr_base
    vaddr_base = mmap(0, 1024, PROT_READ|PROT_WRITE,
            MAP_SHARED, dev_mem_fd, 0x0); // not 0x40000000
    if (vaddr_base == MAP_FAILED) { perror("mmap()"); return 1; }
    
    uint32_t * reset_reg = (uint32_t*) vaddr_base;
    uint32_t * count_reg = (uint32_t*) (vaddr_base + 0x4);

    //open the input file 
    int input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        fprintf(stderr, "open() %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }
  
    //reset popcount
    *reset_reg = 1;

    //read file into buffer and pass to popcount
    uint32_t data_buf_size = 0x10000; //64KB
    uint8_t * data_buf = alloca(sizeof(uint8_t) * data_buf_size);
    uint8_t * data_ptr;
    ssize_t buf_cnt;

    while (1) {
        //read into buffer
        buf_cnt = read(input_fd, data_buf, data_buf_size);
        if (buf_cnt < 0) {
            perror("read()");
            exit(1);
        } else if (buf_cnt == 0) {
            break; //EOF
        } else {
            // Feed the data into the counter in 4-byte chunks
            data_ptr = data_buf;
            while (buf_cnt >= 4) {
                *count_reg = *(uint32_t *)data_ptr;
                data_ptr += 4;
                buf_cnt -= 4;
            }
            // Feed any remaining bytes 1 byte at a time
            while (buf_cnt > 0) {
                *count_reg = *data_ptr;
                data_ptr++;
                buf_cnt--;
            }
        }
    }
    // Read the bit count result
    printf("Counted %u ones\n", *count_reg);

    //close input file
    close(input_fd);

    if (munmap(vaddr_base, 1024) != 0) { perror("munmap()"); }
    if (close(dev_mem_fd) != 0) { perror("close()"); }
    dev_mem_fd = -1;

    return 0;
}



