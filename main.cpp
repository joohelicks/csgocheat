#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include "definitions.h"

void write_memory(pid_t pid, void* dst, void* src, size_t size)
{
    /*
    pid  = target process id
    dst  = address to write to on the target process
    src  = address to read from on the caller process
    size = size of the buffer that will be read
    */
    struct iovec iosrc;
    struct iovec iodst;
    iosrc.iov_base = src;
    iosrc.iov_len  = size;
    iodst.iov_base = dst;
    iodst.iov_len  = size;

    process_vm_writev(pid, &iosrc, 1, &iodst, 1, 0);
}



void read_memory(pid_t pid, void* src, void* dst, size_t size)
{
    /*
    pid  = target process id
    src  = address to read from on the target process
    dst  = address to write to on the caller process
    size = size of the buffer that will be read
    */

    struct iovec iosrc;
    struct iovec iodst;
    iodst.iov_base = dst;
    iodst.iov_len  = size;
    iosrc.iov_base = src;
    iosrc.iov_len  = size;

    if (process_vm_readv(pid, &iodst, 1, &iosrc, 1, 0) == -1)
		std::cout << "Error: " << strerror(errno) << std::endl;
}





int main(int argc, char **argv)
{

	if (argc < 2) {
		std::cout << "usage: " << argv[0] << " <pid>" << std::endl;
		return 0;
	}

	uint32_t pid = atoi(argv[1]);

	if (pid == 0) {
		std::cout << "Please give proper PID" << std::endl;
		return 0;
	}

	uintptr_t playeraddr = 0;

	while (playeraddr == 0) {
		sleep(1);
		read_memory(pid, (void*)(client_clientaddr+dwLocalPlayer), &playeraddr, sizeof(playeraddr));
	}

	int zero = 0;
	int flashdur = 0;
	uintptr_t playerptr = *(&playeraddr);


	while (true) {
		read_memory(pid, (void*)(playerptr + m_flFlashDuration), &flashdur, sizeof(flashdur));

		if (flashdur > 0)
			write_memory(pid, (void*)(playerptr + m_flFlashDuration), &zero, sizeof(zero));

	}

    return 0;
}


