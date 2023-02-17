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

#include <input_my.h>

static page_list g_main_page_list;

static void list_on_pressed(void *pParams)
{
	return;
}

static void video_on_pressed(void *pParams)
{
	Page("video")->Run(NULL);
}

static void lists_regist(void)
{
	int list_num = 0;

	g_main_page_list.tlists[list_num].name = "led";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "music";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "video";
	g_main_page_list.tlists[list_num++].list_press = video_on_pressed;

	g_main_page_list.tlists[list_num].name = "settings";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.tlists[list_num].name = "about";
	g_main_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_main_page_list.list_num = list_num;
}

static void Main_list_page_run(void *pParams)
{
	int ret;
	int selected;
	static inputevent input;
	disp_region tscreen_Region;
	
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

	generate_list_screen_buffer(&g_main_page_list);
	FlushDisplayRegion(&g_main_page_list.screen_buffer, &tscreen_Region);

	while(1)
	{
		ret = GetInputEvent(&input);

		if (ret){
			printf("GetInputEvent err!\n");
			continue;
		}

		if ( input.iType == input_type_remote)
		{
			if (input.event.code == 0x15)
			{
				selected = g_main_page_list.list_pos;
				g_main_page_list.tlists[selected].list_press(NULL);
			}
			else if(input.event.code == 0x19)
			{
				roll_animation(&g_main_page_list, 1);
			}
			else if(input.event.code == 0x40)
			{
				roll_animation(&g_main_page_list, -1);
			}
			else if(input.event.code == 0x43)
			{
				break;
			}
			generate_list_screen_buffer(&g_main_page_list);
			FlushDisplayRegion(&g_main_page_list.screen_buffer, &tscreen_Region);
		}

	}

	/* 释放页面的buffer空间 */
	free(g_main_page_list.page_buffer.fb_base);
	free(g_main_page_list.screen_buffer.fb_base);

}

static Page_action g_tMain_list_page = {
	.name = "main_list",
	.Run  = Main_list_page_run,
};

void Main_list_page_register(void)
{
	Page_register(&g_tMain_list_page);
}

