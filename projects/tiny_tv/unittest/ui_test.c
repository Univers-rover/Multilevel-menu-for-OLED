#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include <display_elc.h>
#include <font_elc.h>
#include <ui_elc.h>

int main(int argc, char **argv)
{
	pdisp_buffer ptBuffer;
	Button tButton;
	disp_region tRegion;
	int error;

	if (argc != 2)
	{
		printf("Usage: %s <font_size>\n", argv[0]);
		return -1;
	}
	
	/* 显示系统初始化 */
	displayinit();

	SelectDefaultDisplay("fb");

	defaultdev_init();
	
	ptBuffer = GetDisplayBuffer();
	
	/* 字体系统初始化 */
	Fonts_init();
	
	error = SelectAndInitFont("freetype", argv[1]);
	
	if (error)
	{
		printf("SelectAndInitFont err\n");
		return -1;
	}

	tRegion.iRegion_x        = 200;
	tRegion.iRegion_y        = 200;
	tRegion.iRegion_width    = 300;
	tRegion.iRegion_height   = 100;
	
	Button_init("test", &tButton, &tRegion, NULL, NULL);
	tButton.OnDraw(&tButton, ptBuffer);
	while (1)
	{
		tButton.OnPress(&tButton, ptBuffer, NULL);
		sleep(2);
	}
	
	return 0;	
}


