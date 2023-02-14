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

#include <display.h>
#include <font.h>
#include <page.h>

int main(int argc, char **argv)
{
	int error;
    char font_file[10] = "simsun.ttc";

	/* 显示系统初始化 */
	displayinit();
	SelectDefaultDisplay("fb");//选择显示设备为fb
	defaultdev_init();//将选中的显示设备初始化

	/* 文字系统初始化 */
	Fonts_init();
	error = SelectAndInitFont("freetype", font_file);//选择freetype或点阵，选择字体
	if (error)
	{
		printf("SelectAndInitFont_error\n");
        return -1;
	}

    /* 页面系统初始化 */
	Pages_register();

    /* 运行业务系统主页面 */
	Page("main")->Run(NULL);

	defaultdev_exit();
	return 0;	
}


