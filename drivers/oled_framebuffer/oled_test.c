#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <wchar.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static int fd_fb;
static struct fb_var_screeninfo var;	/* Current var */
struct fb_fix_screeninfo fix;   /* Current fix */
static int screen_size;//显存空间大小
static unsigned char *fb_base;//framebuffer基地址
static unsigned int line_width;//一行占多少字节
static unsigned int pixel_width = 1;//一个像素占多少字节

/**********************************************************************
 * 函数名称： lcd_put_pixel
 * 功能描述： 在LCD指定位置上输出指定颜色（描点）
 * 输入参数： x坐标，y坐标，颜色
 * 输出参数： 无
 * 返 回 值： 会
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/05/12	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/ 
void lcd_put_pixel(int x, int y, unsigned int color)
{
	unsigned char *pen_1_base = fb_base + y*16 + (x >> 3);
	unsigned int pen_1_offset = x & 0x7;
	unsigned char *pen_8 = fb_base+y*line_width+x*pixel_width;
	unsigned short *pen_16;	
	unsigned int *pen_32;
	unsigned char temp;

	unsigned int red, green, blue;

	pen_16 = (unsigned short *)pen_8;
	pen_32 = (unsigned int *)pen_8;

	switch (var.bits_per_pixel)
	{
		case 1:
		{
			temp = *pen_1_base; 
            //printf("pen_1_base=%hx,temp = %d,pen_1_offset=%d\n",pen_1_base ,temp, pen_1_offset);
			if (color)
				temp = temp |  ( 1 << (7 - pen_1_offset));
			else
				temp = temp & ~( 1 << (7 - pen_1_offset));

            memset(pen_1_base, temp, 1);
            //printf("temp = %d, base-1 = %d, base = %d, base+1 = %d\n\n",temp, *(pen_1_base-1), *pen_1_base, *(pen_1_base+1));
            break;
		}

		case 8:
		{
			*pen_8 = color;
			break;
		}
		case 16:
		{
			/* 565 */
			red   = (color >> 16) & 0xff;
			green = (color >> 8) & 0xff;
			blue  = (color >> 0) & 0xff;
			color = ((red  >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
			*pen_16 = color;
			break;
		}
		case 32:
		{
			*pen_32 = color;
			break;
		}
		default:
		{
			printf("can't surport %dbpp\n", var.bits_per_pixel);
			break;
		}
	}
}

/**********************************************************************
 * 函数名称： draw_bitmap
 * 功能描述： 根据bitmap位图，在LCD指定位置显示汉字
 * 输入参数： x坐标，y坐标，位图指针
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2020/05/12        V1.0     zh(angenao)         创建
 ***********************************************************************/ 
void draw_bitmap( FT_Bitmap* bitmap, FT_Int x,  FT_Int y)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;

	printf("x = %d, y = %d\n", x, y);
    printf("x_max = %d, y_max = %d\n", x_max, y_max);

    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
        for ( i = x, p = 0; i < x_max; i++, p++ )
        {
            if ( i < 0  || j < 0  || i >= var.xres || j >= var.yres )
            	continue;

            //image[j][i] |= bitmap->buffer[q * bitmap->width + p];
            lcd_put_pixel(i, j, bitmap->buffer[q * bitmap->width + p]);
        }
    }
}

int compute_string_bbox(FT_Face face, wchar_t *wstr, FT_BBox  *abbox)
{
    int i;
    int error;
    FT_BBox bbox;
    FT_BBox glyph_bbox;
    FT_Vector pen;
    FT_Glyph  glyph;
    FT_GlyphSlot slot = face->glyph;

    /* 初始化 */
    bbox.xMin = bbox.yMin = 32000;
    bbox.xMax = bbox.yMax = -32000;

    /* 指定原点为(0, 0) */
    pen.x = 0;
    pen.y = 0;

    /* 计算每个字符的bounding box */
    /* 先translate, 再load char, 就可以得到它的外框了 */
    for (i = 0; i < wcslen(wstr); i++)
    {
        /* 转换：transformation */
        FT_Set_Transform(face, 0, &pen);

        /* 加载位图: load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(face, wstr[i], FT_LOAD_RENDER);
        if (error)
        {
            printf("FT_Load_Char error\n");
            return -1;
        }

        /* 取出glyph */
        error = FT_Get_Glyph(face->glyph, &glyph);
        if (error)
        {
            printf("FT_Get_Glyph error!\n");
            return -1;
        }
        
        /* 从glyph得到外框: bbox */
        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_TRUNCATE, &glyph_bbox);

        /* 更新外框 */
        if ( glyph_bbox.xMin < bbox.xMin )
            bbox.xMin = glyph_bbox.xMin;

        if ( glyph_bbox.yMin < bbox.yMin )
            bbox.yMin = glyph_bbox.yMin;

        if ( glyph_bbox.xMax > bbox.xMax )
            bbox.xMax = glyph_bbox.xMax;

        if ( glyph_bbox.yMax > bbox.yMax )
            bbox.yMax = glyph_bbox.yMax;
        
        /* 计算下一个字符的原点: increment pen position */
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }

    /* return string bbox */
    *abbox = bbox;
}

int display_string(FT_Face face, wchar_t *wstr, int lcd_x, int lcd_y)
{
    int i;
    int error;
    FT_BBox bbox;
    FT_Vector pen;
    FT_Glyph  glyph;
    FT_GlyphSlot slot = face->glyph;

    /* 把LCD坐标转换为笛卡尔坐标 */
    int x = lcd_x;
    int y = var.yres - lcd_y;

    /* 计算外框 */
    compute_string_bbox(face, wstr, &bbox);

	printf("bbox.xMin=%d, bbox.yMax=%d, y=%d\n", bbox.xMin, bbox.yMax, y);

    /* 反推原点 */
    pen.x = (x - bbox.xMin) ; /* 单位: 1/64像素 */
    pen.y = (y - bbox.yMax) ; /* 单位: 1/64像素 */
	printf("pen.x=%d, pen.y=%d\n", pen.x, pen.y);
    /* 处理每个字符 */
    for (i = 0; i < wcslen(wstr); i++)
    {
        /* 转换：transformation */
        FT_Set_Transform(face, 0, &pen);

        /* 加载位图: load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(face, wstr[i], FT_LOAD_RENDER);
        if (error)
        {
            printf("FT_Load_Char error\n");
            return -1;
        }
		printf("slot->bitmap_top=%d\n",slot->bitmap_top);
        /* 在LCD上绘制: 使用LCD坐标 */
        draw_bitmap( &slot->bitmap,  slot->bitmap_left,  var.yres - slot->bitmap_top - pen.y);

        /* 计算下一个字符的原点: increment pen position */
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }

    return 0;
}

/* ./show_pixel <dev> */
int main(int argc, char **argv)
{
	int i;
	int j;
    int error;
	
    FT_Library    library;
    FT_Face       face;
    FT_BBox bbox;

    int font_size = 30;
    int lcd_x = 5;
	int lcd_y = 20;

	wchar_t *wstr = L"张狗蛋";
	char font_file[] = "simsun.ttc";

	if (argc != 2)
	{
		printf("Usage: %s <dev>\n", argv[0]);
		return -1;
	}
	
	fd_fb = open(argv[1], O_RDWR);
	if (fd_fb < 0)
	{
		printf("can't open %s\n", argv[1]);
		return -1;
	}
	if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var))
	{
		printf("can't get var\n");
		return -1;
	}

	printf("LCD info: %d x %d, %dbpp\n", var.xres, var.yres, var.bits_per_pixel);

	line_width  = var.xres * var.bits_per_pixel / 8;
	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
	fb_base = (unsigned char *)mmap(NULL , screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if (fb_base == (unsigned char *)-1)
	{
		printf("can't mmap\n");
		return -1;
	}

	/* 清屏: 全部设为黑 */
	memset(fb_base, 0x00, screen_size);
    /*
	for (i= 0; i < 1; i++)
	{
		for (j = 0; j < 17; j++)
        {
            lcd_put_pixel(j, i, 1);
        }
	}
    */
    printf("base = %d\n",*(fb_base+2));

	error = FT_Init_FreeType( &library );              /* initialize library */
    //printf("FT_init_ok\n");
    error = FT_New_Face( library, font_file, 0, &face ); /* create face object */

    FT_Set_Pixel_Sizes(face, font_size, 0);

    display_string(face, wstr, lcd_x, lcd_y);
	munmap(fb_base , screen_size);
	close(fd_fb);
	
	return 0;	
}

