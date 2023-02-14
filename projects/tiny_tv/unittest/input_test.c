#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <input_elc.h>
	
int main(int argc, char **argv)
{
	int ret;
	inputevent event;
	
	input_init();
	input_device_init();

	while (1)
	{
		printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
		ret = GetInputEvent(&event);

		printf("%s %s %d, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
		if (ret) 
		{
			printf("GetInputEvent err!\n");
			return -1;
		}
		else
		{
			printf("%s %s %d, event.iType = %d\n", __FILE__, __FUNCTION__, __LINE__, event.iType );
			if (event.iType == input_type_touchscreen)
			{
				printf("Type      : %d\n", event.iType);
				printf("iX        : %d\n", event.ix);
				printf("iY        : %d\n", event.iy);
				printf("iPressure : %d\n", event.iPressure);
			}
			else if (event.iType == input_type_net)
			{
				printf("Type      : %d\n", event.iType);
				printf("str       : %s\n", event.mesg);
			}
		}
	}
	return 0;	
}


