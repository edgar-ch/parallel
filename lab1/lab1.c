#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define RAM_USAGE 16

struct adler32_args {
	uint8_t *data;
	ssize_t data_len;
	uint32_t chksum;
};

size_t ram_size();
size_t proc_cnt();
void *adler32(void *);

const int MOD_ADLER = 65521;

int main()
{
	int i, fd;
	size_t read_buf_size;
	ssize_t bytes_read, bytes_to_thread;
	int thr_cnt = proc_cnt();
	pthread_t calc_threads[thr_cnt];
	struct adler32_args thread_data[thr_cnt];
	uint8_t *buf = NULL;
	
	read_buf_size = ram_size() / RAM_USAGE;
	buf = (uint8_t *) malloc(read_buf_size * sizeof(uint8_t));
	
	fd = open("test.file", O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	
	while (1) {
		bytes_read = read(fd, (void *) buf, read_buf_size);
		if (bytes_read > 0) {
			bytes_to_thread = bytes_read / thr_cnt;
			for (i = 0; i < thr_cnt; i++) {
				thread_data[i].data = buf + bytes_to_thread * i;
				thread_data[i].data_len = bytes_to_thread;
			}
			thread_data[i - 1].data_len += bytes_read % thr_cnt;
			// TODO: start threads here and sum results
		} else if (bytes_read == 0) {
			break;
		} else {
			perror("read");
			exit(EXIT_FAILURE);
		}
	}
	
	printf("Mem size: %ld\n", ram_size());
	printf("Read buf size: %ld\n", read_buf_size);
	printf("Proc: %ld\n", proc_cnt());

	exit(EXIT_SUCCESS);
}

size_t ram_size()
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);

	return pages * page_size;
}

size_t proc_cnt()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

void *adler32(void *args)
{
	struct adler32_args *data_str = (struct adler32_args *) args;
	
	uint32_t a = 1, b = 0;
	uint8_t *ptr;
	ssize_t len = data_str->data_len;
	
	for (ptr = data_str->data; ptr - data_str->data < len; ptr++) {
		a = (a + *ptr) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}
	data_str->chksum = (b << 16) | a;
	
	return 0;
}
