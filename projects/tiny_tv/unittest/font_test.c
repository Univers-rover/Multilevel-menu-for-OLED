#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#include <display_elc.h>
#include <font_elc.h>

int main(int argc, char **argv)
{
	pdisp_buffer ptBuffer;
	int error;
	FontBitMap FontBitMap;

	wchar_t *wstr = L"百问网www.100ask.net";
	int i = 0;
	int lcd_x, lcd_y;
	int font_size;

	if (argc!= 5)
    {
		printf("Usage: %s <font_file> <lcd_x> <lcd_y> <font_size>\n", argv[0]);
	}
	
	lcd_x     = atoi(argv[2]);
	lcd_y     = atoi(argv[3]);
	font_size = atoi(argv[4]);
	
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
		printf("SelectAndInitFont_error\n");
        return -1;
	}

	error = set_font_size( font_size );
	if (error)
	{
		printf("Set_font_size_error\n");
        return -1;
	}

	while (wstr[i])
	{
		FontBitMap.iCurBaseX = lcd_x;
		FontBitMap.iCurBaseY = lcd_y;

		error = get_font_bitmap(wstr[i], &FontBitMap);
		if (error)
		{
			printf("Get_font_bitmap_error\n");
			return -1;
		}
		/* 绘制bitmap */

		draw_font_bitmap(&FontBitMap, 0xff0000);//修改对应区域的buffer
		FlushDisplayRegion(ptBuffer, &FontBitMap.tRegion);//将对应区域刷到设备上

		lcd_x = FontBitMap.iNextBaseX;
		lcd_y = FontBitMap.iNextBaseY;
		i++;
	}
	
	sleep(5);
	defaultdev_exit();
	
	return 0;
}

