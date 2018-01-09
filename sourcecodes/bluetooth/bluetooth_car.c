#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/*
	Intelligent Vehicle part of Bluetooth:
	When the car stop, it is client-level program;
	when the car run, it is server-level program;
	Role may be exchanged in the while loop 
*/

int main(int argc, char **argv) {
	int pFile;
	int rfcomm;
	char buf[1024], buff[1024];
	
	//check if the kernel module is implemented in "/dev" directory
	pFile = open("/dev/IntelligentVehicle", O_RDWR);
	if (pFile < 0) {
		printf (stderr, "IntelligentVehicle module isn't loaded\n");
		return 1;
	}
	//check if the bluetooth is implemented in "/dev" directory
	rfcomm = open("/dev/rfcomm0", O_RDWR);
	if (rfcomm < 0) {
		printf (stderr, "rfcomm0 module isn't loaded\n");
		return 1;
	}
	
	//Once these file is ready, the program should go into the while loop 
	//While loop will run forever 
	while(1) {
		//At first, the car plays a role of client and waiting 
		//for reading the instruction from server
		if(read(rfcomm, buff, sizeof(buff)) > 0) {
			write(pFile, buff, sizeof(buff));
		}
		//when it encounter to an obstacle, it will read the kernel module nod
   		//and send it to the controller.
		read(pFile, buf, sizeof(buf));
		printf("%c", &buf[0]);
		if (buf[0] == 'O') {
			write(rfcomm, buf, sizeof(buf));
		}

	}
	close(pFile);
	close(rfcomm);
	return 0;
}
