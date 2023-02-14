#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <common.h>
#include <page.h>
#include <display.h>
#include <font.h>
#include <list.h>

static page_list g_main_page_list;

static void list_on_pressed(void *pParams)
{
	return;
}

static void lists_regist(void)
{
	int list_num = 0;

	g_main_page_list.tlists[list_num].name = "led";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "music";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "video";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "settings";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "about";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.list_num = list_num;
}

static void Main_page_run(void *pParams)
{
	int run_state = 2;
	disp_region tscreen_Region;
	int ac[6] = {1,-1,1,-1,1,-1};
	int aa = -1;

	/* list初始化 */
	lists_regist();
	list_page_init(&g_main_page_list);
	//分配页面和屏幕的buffer空间
	g_main_page_list.page_buffer.fb_base = (char *)calloc(g_main_page_list.line_size*LIST_Height*\
							 g_main_page_list.list_num, sizeof(char));
	g_main_page_list.screen_buffer.fb_base = (char *)calloc(g_main_page_list.list_size*LIST_NUM, sizeof(char));

	//生成页面buffer
	generate_list_page_buffer(&g_main_page_list);
	tscreen_Region.iRegion_x = 0;
	tscreen_Region.iRegion_y = 0;
	tscreen_Region.iRegion_width = g_main_page_list.screen_buffer.ixres;
	tscreen_Region.iRegion_height = g_main_page_list.screen_buffer.iyres;

	for (int i = 0; i < 4; i++)
	{
		aa = aa * -1;
		roll_animation(&g_main_page_list, aa);
		//generate_list_screen_buffer(&g_main_page_list);
		//FlushDisplayRegion(&g_main_page_list.screen_buffer, &tscreen_Region);
		//(g_main_page_list.list_pos)++;
		//if ((g_main_page_list.list_pos - g_main_page_list.top_list) >= LIST_NUM)
			//g_main_page_list.top_list++;
		//sleep(1);
	}
	
	//draw_text_at_center(&g_main_page_list.screen_buffer, strr, &tscreen_Region, 1, 24);
	FlushDisplayRegion(&g_main_page_list.screen_buffer, &tscreen_Region);

	/* 释放页面的buffer空间 */
	free(g_main_page_list.page_buffer.fb_base);
	free(g_main_page_list.screen_buffer.fb_base);

}

static Page_action g_tMain_page = {
	.name = "main",
	.Run  = Main_page_run,
};

void Main_page_register(void)
{
	Page_register(&g_tMain_page);
}

