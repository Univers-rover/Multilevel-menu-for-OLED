#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <common.h>
#include <list.h>


/* 计算page_list的大小，空间等基本信息 */
void list_page_init(ppage_list ptpage_list)
{
    int i;
    Region_cartesian tRegion_cartesian;

    disp_buffer tp_buffer;

    tp_buffer = *GetDisplayBuffer();

    //初始化页面buffer值
    ptpage_list->page_buffer.ixres = tp_buffer.ixres;
    ptpage_list->page_buffer.iyres = LIST_Height*ptpage_list->list_num;
    ptpage_list->page_buffer.ibpp = tp_buffer.ibpp;

    ptpage_list->screen_buffer.ixres = tp_buffer.ixres;
    ptpage_list->screen_buffer.iyres = tp_buffer.iyres;
    ptpage_list->screen_buffer.ibpp = tp_buffer.ibpp;

    ptpage_list->line_size = tp_buffer.ixres*tp_buffer.ibpp/8;
    ptpage_list->list_size = ptpage_list->line_size*LIST_Height;
    ptpage_list->list_pos = 0;
    ptpage_list->top_list = 0;

    /* 计算每个list name的长度 */
    tRegion_cartesian.iLeftUpX=0;
    tRegion_cartesian.iLeftUpY=0;
    set_font_size(LIST_FONT_SIZE);
    for(i = 0; i < ptpage_list->list_num; i++) {
        get_str_RegionCar(ptpage_list->tlists[i].name, &tRegion_cartesian);
        ptpage_list->tlists[i].list_length = tRegion_cartesian.iWidth;
    }
}

/* 用来绘制page_list的整个buffer */
void generate_list_page_buffer(ppage_list ptpage_list)
{
    int i;
    disp_region line_region;//绘制的区域

    line_region.iRegion_x = LEFT_GAP;
    line_region.iRegion_y = 0;
    line_region.iRegion_height= LIST_Height;

    /* 循环num次，绘制每栏菜单的buffer并存放进页面buffer */
    for (i = 0; i < ptpage_list->list_num; i++)
    {
        line_region.iRegion_width = ptpage_list->tlists[i].list_length + LEFT_GAP;
        draw_text_at_center(&ptpage_list->page_buffer, ptpage_list->tlists[i].name,\
                            &line_region, 1, LIST_FONT_SIZE);
        line_region.iRegion_y+= LIST_Height;
    }

}

/* 将选中list的矩形区域做反色处理以表示被选中 */
static void draw_selected_list(ppage_list ptpage_list, pdisp_region ptdisp_region)
{
    disp_region selected_region = *ptdisp_region;//选中的区域

    selected_region.iRegion_y = selected_region.iRegion_y + 2;
    selected_region.iRegion_height = selected_region.iRegion_height - 4;
    
    /* 将选中区域反色 */
    inversion_buffer(&ptpage_list->screen_buffer, &selected_region);

    /* 将四个角做成圆角,圆角在边上长度为2，只反色中间一段 */
    selected_region.iRegion_height= 1;
    //第一行
    selected_region.iRegion_x = 2;
    selected_region.iRegion_width = selected_region.iRegion_width - 4;
    selected_region.iRegion_y = selected_region.iRegion_y - 2;
    inversion_buffer(&ptpage_list->screen_buffer, &selected_region);
    //倒数第一行
    selected_region.iRegion_y = selected_region.iRegion_y + LIST_Height - 1;
    inversion_buffer(&ptpage_list->screen_buffer, &selected_region);
    //倒数第二行
    selected_region.iRegion_x--;
    selected_region.iRegion_width = selected_region.iRegion_width + 2;
    selected_region.iRegion_y = selected_region.iRegion_y - 1;
    inversion_buffer(&ptpage_list->screen_buffer, &selected_region);
    //第二行
    selected_region.iRegion_y = selected_region.iRegion_y + 3 - LIST_Height;
    inversion_buffer(&ptpage_list->screen_buffer, &selected_region);
}

/* 在屏幕右侧绘制滚动条 */
static void draw_rollbar(ppage_list ptpage_list,float offset)
{
    int i;
    int y_res = LIST_Height * LIST_NUM;
    int bar_height = y_res / ptpage_list->list_num;
    int top_height = (y_res % ptpage_list->list_num)/2;
    int below_height = y_res - top_height - bar_height*ptpage_list->list_num;
    disp_region bar_region;//绘制的区域

    /* 绘制固定部分 */
    //绘制上方空余加第一条线
    bar_region.iRegion_x = ptpage_list->screen_buffer.ixres - 5;
    bar_region.iRegion_y = 0;
    bar_region.iRegion_width  = 5;
    bar_region.iRegion_height = top_height + 1;
    draw_rectangle(&ptpage_list->screen_buffer, &bar_region, 1);
    //绘制中间竖线
    bar_region.iRegion_x = ptpage_list->screen_buffer.ixres - 3;
    bar_region.iRegion_width  = 1;
    bar_region.iRegion_height = ptpage_list->screen_buffer.iyres;
    draw_rectangle(&ptpage_list->screen_buffer, &bar_region, 1);
    //绘制下方空余最后一条线
    bar_region.iRegion_x = ptpage_list->screen_buffer.ixres - 5;
    bar_region.iRegion_width  = 5;
    bar_region.iRegion_y = ptpage_list->screen_buffer.iyres - below_height - 2;
    bar_region.iRegion_height = below_height + 1;
    draw_rectangle(&ptpage_list->screen_buffer, &bar_region, 1);
    //绘制分隔线
    bar_region.iRegion_height = 2;
    for (i = 0 ; i < (ptpage_list->list_num); i++) {
        bar_region.iRegion_y = top_height + i * bar_height - 1;
        draw_rectangle(&ptpage_list->screen_buffer, &bar_region, 1);
    }

    /* 绘制滚动部分 */
    bar_region.iRegion_y = top_height + bar_height*ptpage_list->list_pos + (int)(offset*bar_height);
    bar_region.iRegion_height = bar_height;
    draw_rectangle(&ptpage_list->screen_buffer, &bar_region, 1);
}

/* 用来绘制一个屏幕的buffer */
void generate_list_screen_buffer(ppage_list ptpage_list)
{
    int count = 0;
    int list_size = ptpage_list->list_size;
    int size_before_screen = list_size * ptpage_list->top_list;
    disp_region select_region;
    printf("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    /* 从page_buffer中找到当前应该显示的部分并分配给临时的页面buffer */
    while (count<list_size*LIST_NUM)
    {
        memset(ptpage_list->screen_buffer.fb_base+count, \
           *(ptpage_list->page_buffer.fb_base+size_before_screen+count), 1);
           count++;
    }
    printf("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    /* 绘制选中框 */
    select_region.iRegion_x = 0;
    select_region.iRegion_y = (ptpage_list->list_pos - ptpage_list->top_list) * LIST_Height;
    select_region.iRegion_width = ptpage_list->tlists[ptpage_list->list_pos].list_length + LEFT_GAP*2;;
    select_region.iRegion_height = LIST_Height;
    draw_selected_list(ptpage_list, &select_region);
    printf("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    /* 绘制滚动条 */
    draw_rollbar(ptpage_list, 0);
}

/* 滚动动画 */
int roll_animation(ppage_list ptpage_list, int direction)
{
    int i;
    int count;
    int page_roll = 0;
    int start_pos = ptpage_list->list_pos;
    int end_pos = ptpage_list->list_pos + direction;
    int sl_start_pos = (start_pos - ptpage_list->top_list) * LIST_Height;//选中区域的y起始位置
    int sl_gap_height;
    int sl_gap_width,sl_start_width;//选中区域的起始和结束宽度
    int size_before_screen = 0;
    float offset;
    int speed[16] = {1,2,4,7,10,10,10,10,10,10,10,10,7,4,2,1};

    disp_region select_region;
    disp_region screen_region;

    //如果已经翻到尽头就返回-1
    if(end_pos >= ptpage_list->list_num || end_pos <0){
        return -1;
    }

    //判断是否需要滚动页面
    if(end_pos < ptpage_list->top_list)
        page_roll = -1;//往上翻页
    else if((end_pos - ptpage_list->top_list) >= LIST_NUM)
        page_roll = 1;//往下翻页

    //初始化选中区域参数
    sl_gap_width = ptpage_list->tlists[end_pos].list_length - ptpage_list->tlists[start_pos].list_length;
    sl_start_width = ptpage_list->tlists[start_pos].list_length + 2 * LEFT_GAP;
    select_region.iRegion_x = 0;
    select_region.iRegion_height = LIST_Height;
    if(sl_start_pos < 0)
        sl_start_pos = 0;

    if(page_roll)
        sl_gap_height = 0;
    else
        sl_gap_height = direction * LIST_Height;
    
    //屏幕显示区域
    screen_region.iRegion_x = 0;
    screen_region.iRegion_y = 0;
    screen_region.iRegion_width = ptpage_list->screen_buffer.ixres/2;
    screen_region.iRegion_height = ptpage_list->screen_buffer.iyres/2;

    //printf("sl_start_pos = %d \n",sl_start_pos);
    for (i = 0; i < LIST_Height; i++)
    {
        offset = (float)(i+1)/16;
        //更新文字内容
        size_before_screen = ptpage_list->top_list * ptpage_list->list_size + \
                             page_roll * i* ptpage_list->line_size;
        count = 0;
        while (count<ptpage_list->list_size*LIST_NUM){
            memset(ptpage_list->screen_buffer.fb_base+count, \
                *(ptpage_list->page_buffer.fb_base+size_before_screen+count), 1);
            count++;
        }

        //更新选中框
        select_region.iRegion_y = sl_start_pos + (int)sl_gap_height*offset;
        select_region.iRegion_width = sl_start_width + (int)(offset*sl_gap_width) + LEFT_GAP*2;
        draw_selected_list(ptpage_list, &select_region);


        //更新滚动条
        draw_rollbar(ptpage_list, (float)offset*direction);
        FlushDisplayRegion(&ptpage_list->screen_buffer, &screen_region);
        usleep(speed[i] * 1100);
    }

    //更新page_list的参数
    ptpage_list->list_pos +=direction;
    ptpage_list->top_list +=page_roll;

    return 0;
}


