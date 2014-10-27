#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

size_t ram_size();

int main()
{
	printf("Mem size: %ld\n", ram_size());

	exit(EXIT_SUCCESS);
}

size_t ram_size()
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);

	return pages * page_size;
}
