#ifndef _LIST_H
#define _LIST_H

#define LIST_Height 16 
#define LIST_NUM 4
#define LIST_FONT_SIZE 16 
#define LEFT_GAP 3 

#include <display.h>

typedef struct list{
    char *name;
    int list_length;//占了多少个像素
    void (* list_press)(void *pParams);
}list, *plist;

typedef struct page_list{
    int list_num;
    list tlists[20];
    disp_buffer page_buffer;
    disp_buffer screen_buffer;
    int line_size;//一行内容的内存大小
    int list_size;//一个list的内存大小
    int list_pos;//选中了第几个list，从0开始
    int top_list;//屏幕中最上面是第几个list，从0开始
}page_list, *ppage_list;

/* 其他系统使用 */
void list_page_init(ppage_list ptpage_list);
void generate_list_page_buffer(ppage_list ptpage_list);
void generate_list_screen_buffer(ppage_list ptpage_list);
int roll_animation(ppage_list ptpage_list, int direction);

/* 顶层使用 */

#endif
