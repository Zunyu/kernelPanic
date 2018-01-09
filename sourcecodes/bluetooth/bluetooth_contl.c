#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/*
	Controller part of Bluetooth:
	When the car stop, it is a server-level program;
	when the car run, it is a client-level program;
	Role may be exchanged in the while loop 
*/


int main(int argc, char **argv) {

	int pFile;
	FILE * obstacle;
	int pp;
	int flag = 0; //this flag is a sign to specify the role the controller plays
		      //0 represents server; 1 repren
	char buf[1024], buff[1024];
	printf("senddata\n");
	
	
	//check if the bluetooth is implemented in "/dev" directory
	pp = open("/dev/rfcomm0", O_RDWR);
	if (pp < 0) {
		fprintf (stderr, "rfcomm0 module isn't loaded\n");
		return 1;
	} else {
		printf("open success\n");
	}
	
	//Once these file is ready, the program should go into the while loop
	//While loop will run forever 
	while(1) {
		sleep(1);//read files per second.		
		printf("begin to work\n");
		//the controller plays a role of server when flag == 0
		if(flag == 0) {	
			//read the data from posdata.txt if there exist posdata 
			//send the data to client which is intelligent vehicle	
			pFile = open("/home/posdata.txt", O_RDWR);
			if (pFile >= 0) {
				memset(buff, 0, sizeof(buff));
				read(pFile, buff, sizeof(buff));
				printf("senddata[%s]\n", buff);
				write(pp, buff, sizeof(buff));
				flag = 1;
				printf("flag = %d\n", flag);
				close(pFile);

			}
    		}
		close(pp);
		//the controller plays a role of client when flag == 1
		pp = open("/dev/rfcomm0", O_RDWR);
		if (flag == 1) {
			//read the signal from the server which is intelligent vehicle 
			//send the signal to LCD and the LCD screen will show "Encounter Obstacle"			
			if (read(pp, buf, sizeof(buf)) > 0){
				obstacle = fopen("/home/Location.txt", "w+");
				printf("%s\n", buf);
				fwrite(buf, sizeof(buf), 1, obstacle);
				fclose(obstacle);
				flag = 0;
				printf("flag = %d\n", flag);
			}
		}
	}
	return 0;
}
