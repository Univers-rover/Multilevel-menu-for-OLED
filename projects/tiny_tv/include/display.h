#ifndef _DISPLAY_H
#define _DISPLAY_H

#ifndef NULL
#define NULL (void *)0
#endif

#include <common.h>
#include <font.h>

typedef struct disp_buffer{
    char *fb_base;
    int  ixres;
    int  iyres;
    int  ibpp;
}disp_buffer, *pdisp_buffer;

/* display device operation struct */
typedef struct disp_opr{
    char *cname;
    //int  display_init(void);/* 错误做法 */
    int  (*display_init)(void);
    void (*display_exit)(void);
    int  (*display_getbuffer) (pdisp_buffer prtrn_buffer);
    int  (*display_FlushRegion)(pdisp_buffer ptbuffer, pdisp_region ptRegion);
    struct disp_opr *ptNext;
}disp_opr, *pdisp_opr;

void display_register    (pdisp_opr ptDispOpr);/* 底层设备使用 */
void framebuffer_register();

/* 管理层使用 */
void put_pixel_buffer(pdisp_buffer ptdisp_buffer, int x, int y, unsigned int color);

/* 顶层app使用 */
int  SelectDefaultDisplay    (char *name);
void displayinit             (void);
int  defaultdev_init         (void);
int  defaultdev_exit         (void);
pdisp_buffer GetDisplayBuffer(void);

/* 其他系统调用 */
void inversion_pixel(pdisp_buffer ptdisp_buffer, int x, int y);
void draw_rectangle(pdisp_buffer ptdisp_buffer, pdisp_region ptRegion, unsigned int color);
void inversion_buffer(pdisp_buffer ptdisp_buffer, pdisp_region ptRegion);
void draw_font_bitmap(pdisp_buffer ptdisp_buffer, PFontBitMap ptFontBitMap, unsigned int color);
void draw_text_at_center(pdisp_buffer ptdisp_buffer, char *str,\
						 pdisp_region ptRegion, unsigned int color, int font_size);
int FlushDisplayRegion(pdisp_buffer ptBuffer,pdisp_region ptRegion);

#endif

