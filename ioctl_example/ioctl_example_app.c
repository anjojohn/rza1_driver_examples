#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>

typedef unsigned char __u8;

/* MUST MATCH EXACTLY WHAT IS IN THE DRIVER */
enum {
	IOCTL_CMD1 = _IOR(0xC1,0x20,uint32_t),
	IOCTL_CMD2 = _IOW(0xC1,0x21,uint32_t),
	IOCTL_CMD3 = _IOWR(0xC1,0x22,__u8[12])
};

int main(int argc, char *argv[])
{
	int fd;
	int i,j;
	uint32_t tmp32;
	char message[12];
	char cmd;
	char cmd_str[20];
	int ret;

	fd = open("/dev/drvsmpl", O_RDWR);
	if( fd == -1) {
		printf("Can't open device node\n");
		exit(1);
	}

	printf(	"1 = read 32-bit value (hex)\n"
		"2 = write 32-bit value (hex)\n"
		"3 = reverse string (12 char max)\n"
		"e = exit\n"
		);

	while (1) {

		printf("> ");	/* prompt */

		/* Get user input */
		fgets(cmd_str,sizeof(cmd_str),stdin);

		/* Deal with backspaces */
		i=j=1;
		while(cmd_str[i] != 0) {
			if( cmd_str[i] == '\b')
				j--;
			else
				cmd_str[j++] = cmd_str[i];
			i++;
		}
		cmd_str[j] = 0;

		cmd = cmd_str[0];

		if( cmd == '1' ) {
			ret = ioctl(fd, IOCTL_CMD1, &tmp32);
			printf("Value = 0x%X  (ret=%d)\n",tmp32,ret);
		}
		else if( cmd == '2' ) {
			i = sscanf(cmd_str,"%c %x",&cmd, &tmp32);
			if ( i < 2 ) {
				printf("USAGE: 2 {hex-number}\n");
				continue;
			}
			ret = ioctl(fd, IOCTL_CMD2, &tmp32);
			printf("Written (value = 0x%X)(ret=%d)\n",tmp32,ret);
		}
		else if( cmd == '3' ) {
			if (j < 4) {
				printf("USAGE: 3 {12 character string}\n");
				continue;
			}
			memset(message,' ',12);	// pre-fill with spaces

			i = 2; // skip over command and space
			while(cmd_str[i] != '\n') {
				if( i > (12+2) )
					break;	// 12 characters max
				message[i-2] = cmd_str[i];
				i++;
			}

			ret = ioctl(fd, IOCTL_CMD3, message);
			printf("Returned \"");
			for(i=0;i<12;i++)
				printf("%c",message[i]);
			printf("\" (ret=%d)\n", ret);				
		}
		else if( cmd == 'e' ) {
			break;
		}
	}

	close(fd);
}
