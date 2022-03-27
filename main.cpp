#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <fstream>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "definitions.h"

#define ONGROUND (1 << 0)

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

int read_memory(pid_t pid, void* src, void* dst, size_t size) {
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

    if (process_vm_readv(pid, &iodst, 1, &iosrc, 1, 0) == -1) {
		sleep(1);
		return -1;
	}
	return 0;
}

uintptr_t parsemaps(uint32_t pid) {
	if (pid == 0) {
		return -1;
	}


	std::string so_address;
	std::string path = "/proc/" + std::to_string(pid) + "/maps";
	std::ifstream infile(path);
	
	if (infile.is_open()) {
		std::string line;
		while (std::getline(infile, line)) { 
			if (line.find("client_client.so") != -1) { // find line with "client_client.so"
				so_address = line.substr(0, line.find("-")); // from the same line, cut out the address of client_client.so
				break;
			}
		}
		infile.close();
	} else {
		return -1;
	}

	return std::stoul(so_address, 0, 16);
}
constexpr uintptr_t dwForceJump = 0x2b86b50;

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cout << "usage: " << argv[0] << " <pid>" << std::endl;
		return 0;
	}

	Display *dpy =XOpenDisplay(0);
	Window root = DefaultRootWindow(dpy);
	XEvent ev;

	int             keycode         = XKeysymToKeycode(dpy,XK_F1);
    Window          grab_window     = root;
    Bool            owner_events    = False;
    int             pointer_mode    = GrabModeAsync;
    int             keyboard_mode   = GrabModeAsync;

	XGrabKey(dpy, keycode, 0, grab_window, owner_events, pointer_mode,
			keyboard_mode);
	XSelectInput(dpy, root, KeyPressMask);

	uint32_t pid = atoi(argv[1]);

	if (pid == 0) {
		std::cout << "Please give proper PID" << std::endl;
		return 0;
	}

	uintptr_t client_clientaddr = parsemaps(pid);
	uintptr_t playeraddr = 0;

	while (playeraddr == 0) {
		sleep(1);
		read_memory(pid, (void*)(client_clientaddr+dwLocalPlayer), &playeraddr, sizeof(playeraddr));
	}


	int zero = 0;
	int flashdur = 0;
	int bunnyhop = 6;
	uintptr_t playerptr = *(&playeraddr);
	int flags=0;
	bool bhop = false;

	while (true) {
		int retval = read_memory(pid, (void*)(playerptr + m_flFlashDuration), &flashdur, sizeof(flashdur));
		if (retval == -1) {
			std::cout << "Error: " << strerror(errno) << std::endl;
			break;
		}

		if (flashdur > 0)
			write_memory(pid, (void*)(playerptr + m_flFlashDuration), &zero, sizeof(zero));

		read_memory(pid, (void*)(playerptr + m_fFlags), &flags, sizeof(flags));
		if (XCheckWindowEvent(dpy, root, KeyPressMask, &ev))
			bhop = !bhop;

		if (flags & (1 << 0) && bhop == true) {
			write_memory(pid, (void*)(client_clientaddr + dwForceJump), &bunnyhop, sizeof(bunnyhop));
			usleep(100);
			write_memory(pid, (void*)(client_clientaddr + dwForceJump), &zero, sizeof(zero));
		}
	}
	XCloseDisplay(dpy);
    return 0;
}
