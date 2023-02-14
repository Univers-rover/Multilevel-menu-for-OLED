#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <display.h>

static int fd_fb;
static unsigned char *fb_base;
static struct fb_var_screeninfo var;	/* Current var */
static int screen_size;
static int line_size;//一行图像占用内存大小

static int framebuffer_init(void)
{
    int fd_fb;
    fd_fb = open("/dev/fb2", O_RDWR);
    
	if (fd_fb < 0)
	{
		printf("can't open /dev/fb2\n");
		return -1;
	}
	if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var))
	{
		printf("can't get var\n");
		return -1;
	}

    line_size = var.xres * var.bits_per_pixel / 8;
	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
    fb_base = (unsigned char *)mmap(NULL , screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
    return 0;
}

static void framebuffer_exit(void)
{
	munmap(fb_base, screen_size);
	close(fd_fb);
}

static int framebuffer_getbuffer(pdisp_buffer pt_buffer)
{
    pt_buffer->fb_base = (char *)fb_base;
    pt_buffer->ixres   = var.xres;
    pt_buffer->iyres   = var.yres;
    pt_buffer->ibpp    = var.bits_per_pixel;
    return 0;
}

/* 将指定区域的buffer刷入显示设备 */
static int display_FlushRegion(pdisp_buffer ptbuffer, pdisp_region ptRegion)
{
    int i,j;
    int count = 0;
    int head_x, length_x;
    int end_y = ptRegion->iRegion_height + ptRegion->iRegion_y;

    head_x = ptRegion->iRegion_x>>3;
    length_x = (ptRegion->iRegion_width>>3) + 1;

    if((length_x + head_x) > 16)
        length_x--;
    /*
    for (i = ptRegion->iRegion_y; i < end_y; i++){
        for (j = 0)
         memset(fb_base+i*line_size+head_x, *(ptbuffer->fb_base+i*line_size+head_x), length_x);
    }*/

    while(count < (screen_size)){
        memset(fb_base+count, *(ptbuffer->fb_base+count), 1);
        count++;
    }
    //memset(fb_base, &ptbuffer->fb_base, screen_size);
    return 0;
}



static disp_opr framebuffer_opr = {
    .cname               = "fb",
    .display_init        = framebuffer_init,
    .display_exit        = framebuffer_exit,
    .display_getbuffer   = framebuffer_getbuffer,
    .display_FlushRegion = display_FlushRegion,
};

void framebuffer_register(void)
{
    display_register(&framebuffer_opr);
}
