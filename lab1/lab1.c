#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define RAM_USAGE 16

struct hash64_args {
	uint8_t *data;
	ssize_t data_len;
	uint64_t chksum_a;
	uint64_t chksum_b;
};

size_t ram_size();
size_t proc_cnt();
void *hash64(void *);

int HASH_INIT = 0;
pthread_mutex_t hash_mutex;

int main(int argc, char **argv)
{
	int ram_usage = RAM_USAGE;
	int i, fd, p_st;
	size_t read_buf_size;
	ssize_t bytes_read, bytes_to_thread;
	int thr_cnt = proc_cnt();
	pthread_t calc_threads[thr_cnt];
	struct hash64_args thread_data[thr_cnt];
	uint8_t *buf = NULL;
	uint64_t chksum_a = 0, chksum_b = 0, chksum;
	void *stat;
	time_t time1, time0;
	char *f_name, c;

	while ((c = getopt(argc, argv, "sf:m:")) != -1) {
		switch (c) {
			case 's':
				thr_cnt = 1;
				break;
			case 'f':
				f_name = optarg;
				break;
			case 'm':
				ram_usage = atoi(optarg);
				if (ram_usage < 2) {
					printf("Will use all avaliable mem, exit.");
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
				if (optopt == 'c')
					printf("Option -%c requires an argument.\n",
					optopt);
				else
					printf("Unknown option `-%c'.\n", optopt);
				exit(EXIT_FAILURE);
			default:
				abort();
		}
	}

	read_buf_size = ram_size() / ram_usage;
	buf = (uint8_t *) malloc(read_buf_size);

	fd = open(f_name, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// init mutex
	pthread_mutex_init(&hash_mutex, NULL);
	time0 = time(NULL);
	// main loop
	while (1) {
		// read data portion
		bytes_read = read(fd, (void *) buf, read_buf_size);
		if (bytes_read > 0) {
			bytes_to_thread = bytes_read / thr_cnt;
			for (i = 0; i < thr_cnt; i++) {
				thread_data[i].data = buf + bytes_to_thread * i;
				thread_data[i].data_len = bytes_to_thread;
			}
			thread_data[i - 1].data_len += bytes_read % thr_cnt;
			// start threads
			for (i = 0; i < thr_cnt; i++) {
				p_st = pthread_create(&calc_threads[i], NULL, hash64, &thread_data[i]);
				if (p_st) {
					printf("Error in pthread_create: %d\n", p_st);
					exit(EXIT_FAILURE);
				}
			}
			// join threads
			for (i = 0; i < thr_cnt; i++) {
				p_st = pthread_join(calc_threads[i], &stat);
				if (p_st) {
					printf("Error in thread join: %d\n", p_st);
					exit(EXIT_FAILURE);
				}
				chksum_a += thread_data[i].chksum_a;
				chksum_b += thread_data[i].chksum_b;
			}
		} else if (bytes_read == 0) {
			break;
		} else {
			perror("read");
			exit(EXIT_FAILURE);
		}
	}
	// Calc final checksum
	chksum = (~chksum_b << 32) ^ chksum_a;
	time1 = time(NULL);

	printf("Elapsed time (in sec): %f\n", difftime(time1, time0));
	printf("MEM size: %ld\n", ram_size());
	printf("Read BUFF size: %ld\n", read_buf_size);
	printf("CPU Cores: %ld\n\n", proc_cnt());
	printf("Checksum: 0x%lx\n", chksum);

	close(fd);
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

void *hash64(void *args)
{
	struct hash64_args *data_str = (struct hash64_args *) args;
	uint64_t a, b;
	uint8_t *ptr;
	ssize_t len = data_str->data_len;

	pthread_mutex_lock(&hash_mutex);
	if (!HASH_INIT) {
		a = 0x00000000, b = 0xFFFFFFFF;
		HASH_INIT = ~HASH_INIT;
	} else {
		a = 0x00000000, b = 0x00000000;
	}
	pthread_mutex_unlock(&hash_mutex);

	for (ptr = data_str->data; ptr - data_str->data < len; ptr++) {
		a += *ptr;
		b += *ptr;
	}
	data_str->chksum_a = a;
	data_str->chksum_b = b;

	return 0;
}
