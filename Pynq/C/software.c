////////////////////////////////////////////////////////////
//    
//    Example popcount using software
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

//adapted from wikipedia
int32_t popcount(uint32_t x)
{
    int32_t count;
    for (count=0; x; count++)
        x &= x - 1;
    return count;
}

int main(int argc, char **argv)
{

	// check for input file
	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_filename\n", argv[0]);
		exit(1);
	}


    //open the input file 
	int input_fd = open(argv[1], O_RDONLY);
	if (input_fd < 0) {
		fprintf(stderr, "open() %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
  
    //reset popcount
    uint32_t count = 0; 

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
                count += popcount(*(uint32_t*) data_ptr);
				data_ptr += 4;
				buf_cnt -= 4;
			}
			// Feed any remaining bytes 1 byte at a time
			while (buf_cnt > 0) {
                uint8_t value8 = (uint8_t) *data_ptr;
				count += popcount((uint32_t) value8);
				data_ptr++;
				buf_cnt--;
			}
		}
	}
	// Read the bit count result
	printf("Counted %u ones\n", count);

    //close input file
	close(input_fd);
    return 0;
}



