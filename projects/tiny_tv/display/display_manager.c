#include <stdio.h>
#include <string.h>

#include <display.h>
#include <font.h>

static pdisp_opr p_dispopr     = NULL;/* 链表头 */
static pdisp_opr pdefault_dispopr = NULL;/* 要使用的设备的结构体指针 */
static disp_buffer g_default_buffer;/* 默认设备的buffer */

static int line_width;//一行占用的内存大小
static int pixel_width;//一个像素占用的内存大小

void put_pixel_buffer(pdisp_buffer ptdisp_buffer, int x, int y, unsigned int color)
{
	unsigned char temp;
	//x++;y++;
	unsigned char *pen_1_base;
	unsigned int pen_1_offset;
	unsigned char *pen_8 = (unsigned char *)(ptdisp_buffer->fb_base + \
							y*line_width + x*pixel_width);
	unsigned short *pen_16;	
	unsigned int *pen_32;	

	unsigned int red, green, blue;	

	switch (ptdisp_buffer->ibpp)
	{
		case 1:
		{
			pen_1_base = (unsigned char *)(ptdisp_buffer->fb_base + \
						y*(ptdisp_buffer->ixres)/8 + (x >> 3));
			pen_1_offset = x & 0x7;
			temp = *pen_1_base; 
			if (color)
				temp = temp |  ( 1 << (7 - pen_1_offset));
			else
				temp = temp & ~( 1 << (7 - pen_1_offset));
            memset(pen_1_base, temp, 1);
            break;
		}
		case 8:
		{
			*pen_8 = color;
			break;
		}
		case 16:
		{
			pen_16 = (unsigned short *)pen_8;
			/* 565 */
			red   = (color >> 16) & 0xff;
			green = (color >> 8)  & 0xff;
			blue  = (color >> 0)  & 0xff;
			color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
			*pen_16 = color;
			break;
		}
		case 32:
		{
			pen_32 = (unsigned int *)pen_8;
			*pen_32 = color;
			break;
		}
		default:
		{
			printf("can't surport %dbpp\n", g_default_buffer.ibpp);
			break;
		}
	}

}

/* 将某个像素反色 */
void inversion_pixel(pdisp_buffer ptdisp_buffer, int x, int y)
{
	unsigned char temp;
	unsigned char *pen_1_base = (unsigned char *)(ptdisp_buffer->fb_base + \
								y*(ptdisp_buffer->ixres)/8 + (x >> 3));
	unsigned int pen_1_offset = 7 - (x&0x7);

	temp = *pen_1_base;
	if ( GET_BIT( temp, pen_1_offset ) )
		temp = temp & ~( 1 << pen_1_offset);
	else
		temp = temp |  ( 1 << pen_1_offset);
	
	memset(pen_1_base, temp, 1);

}

/* 将某个矩形区域反色 */
void inversion_buffer(pdisp_buffer ptdisp_buffer, pdisp_region ptRegion)
{
	int i,j;
	int x_max = ptRegion->iRegion_x + ptRegion->iRegion_width;
	int y_max = ptRegion->iRegion_y + ptRegion->iRegion_height;
	for(j = ptRegion->iRegion_y; j < y_max; j++)
	{
		for(i = ptRegion->iRegion_x; i < x_max; i++)
			inversion_pixel(ptdisp_buffer, i, j);
	}
}

/* 将某个矩形区域全部画上color */
void draw_rectangle(pdisp_buffer ptdisp_buffer, pdisp_region ptRegion, unsigned int color)
{
	int x      = ptRegion->iRegion_x;
	int y      = ptRegion->iRegion_y;
	int width  = ptRegion->iRegion_width;
	int height = ptRegion->iRegion_height;

    int i, j;

    for ( j = y; j < y+height; j++)
    {
        for ( i = x; i < x+width; i++ )
        {
            put_pixel_buffer(ptdisp_buffer, i, j, color);
        }
    }
}

/* 绘制文字系统的bitmap */
void draw_font_bitmap(pdisp_buffer ptdisp_buffer, PFontBitMap ptFontBitMap, unsigned int color)
{
    int i, j, p, q;

	int x     = ptFontBitMap->tRegion.iRegion_x;
	int y     = ptFontBitMap->tRegion.iRegion_y;
	int x_max = x + ptFontBitMap->tRegion.iRegion_width;
    int y_max = y + ptFontBitMap->tRegion.iRegion_height;
	int width = ptFontBitMap->tRegion.iRegion_width;
	unsigned char *buffer = ptFontBitMap->pucBuffer;

	//printf("x = %d, y = %d\n", x, y);
    //printf("x_max = %d, y_max = %d\n", x_max, y_max);

    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
        for ( i = x, p = 0; i < x_max; i++, p++ )
        {
            if (i < 0 || j < 0 || i >= ptdisp_buffer->ixres || j >= ptdisp_buffer->iyres)
            	continue;
			if ( buffer[q * width + p] )//如果这个像素点的值为真，就将它改为要求的颜色
			{
				//printf("x = %d, y = %d\n", i, j);
				put_pixel_buffer(ptdisp_buffer, i, j, color);
			}
        }
    }
}

/* 在区域中心绘制文字 */
void draw_text_at_center(pdisp_buffer ptdisp_buffer, char *str,\
						 pdisp_region ptRegion, unsigned int color, int font_size)
{
	int i = 0;
	int error;
	FontBitMap FontBitMap;
	Region_cartesian tRegion_cartesian;
	
	int n = strlen(str);
	if (font_size == 0)
	{
		font_size = 0.5 * ptRegion->iRegion_width / n;
		if ( font_size > ptRegion->iRegion_height )
			font_size = ptRegion->iRegion_height;
	}
	set_font_size(font_size);

	/* 左下角原点的坐标 */
	get_str_RegionCar(str, &tRegion_cartesian);
	int lcd_x = ptRegion->iRegion_x + 0.5 * \
	            ( ptRegion->iRegion_width - tRegion_cartesian.iWidth );
	int lcd_y = ptRegion->iRegion_y + 0.5 * \
	            ( ptRegion->iRegion_height - tRegion_cartesian.iHeight) + tRegion_cartesian.iHeight;

	while ( i < n )
	{
		FontBitMap.iCurBaseX = lcd_x;
		FontBitMap.iCurBaseY = lcd_y;

		error = get_font_bitmap(str[i], &FontBitMap);
		if (error)
		{
			printf("Get_font_bitmap_error\n");
		}

		/* 绘制bitmap */
		draw_font_bitmap(ptdisp_buffer, &FontBitMap, color);//修改对应区域的buffer

		lcd_x = FontBitMap.iNextBaseX;
		lcd_y = FontBitMap.iNextBaseY;
		i++;
	}
}

void display_register(pdisp_opr ptDispOpr)
{
    /* 将结构体放入链表，并将该函数传给底层设备 */
    ptDispOpr->ptNext = p_dispopr;
    p_dispopr         = ptDispOpr;
}

/* 根据输入的name选择对应的设备赋予默认设备指针 */
int SelectDefaultDisplay(char *name)
{
    pdisp_opr ptemp = p_dispopr;
    while (ptemp)
    {
        if (strcmp(ptemp->cname, name) == 0)
        {
            pdefault_dispopr = ptemp;
            return 0;
        }
            ptemp = ptemp->ptNext;
    }
    return -1;
}

/* 将选中的默认显示设备初始化 */
int defaultdev_init(void)
{
    int ret;
    ret = pdefault_dispopr->display_init();
    if (ret)
    {
        printf("Devinit err\n");
		return -1;
    }

    ret = pdefault_dispopr->display_getbuffer(&g_default_buffer);
    if (ret)
    {
        printf("GetBuffer err\n");
		return -1;
    }

    line_width  = g_default_buffer.ixres * g_default_buffer.ibpp/8;
	pixel_width = g_default_buffer.ibpp/8;

    return 0;
}

/* 外部文件获取显示设备信息的接口 */
pdisp_buffer GetDisplayBuffer(void)
{
	return &g_default_buffer;
}

int FlushDisplayRegion(pdisp_buffer ptBuffer,pdisp_region ptRegion)
{
	pdefault_dispopr->display_FlushRegion(ptBuffer, ptRegion);
	return 0;
}

int defaultdev_exit(void)
{
    /* 向顶层代码提供默认设备退出函数 */
    pdefault_dispopr->display_exit();
    return 0;
}

void displayinit(void)
{
    /* 向顶层代码提供设备初始化函数，将各个设备放入链表 */
	extern void framebuffer_register(void);
    framebuffer_register();
}

