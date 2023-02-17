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

static int g_frame_len;
static page_list g_video_page_list;
disp_region tscreen_Region;
static inputevent g_input;  

//视频播放函数，输入为视频文件名和视频帧数
static void video_play(char *name, int frame_num)
{
	int v = 0;
	int ret;
	int state = 1;//state为1为播放状态，为0则暂停
	FILE *fd = NULL;
	unsigned char *video;
	disp_buffer video_buffer;

	video_buffer.ixres = g_video_page_list.screen_buffer.ixres;
	video_buffer.iyres = g_video_page_list.screen_buffer.iyres;
	video_buffer.ibpp =  g_video_page_list.screen_buffer.ibpp;

	//为视频内容分配内存
	video = calloc(g_frame_len * frame_num * sizeof(unsigned char), sizeof(char));

	fd = fopen((const char *)name, "rb");
	fread(video,sizeof(unsigned char), g_frame_len * frame_num,fd);

	while(v < (frame_num-3))
	{
		//播放状态判断
		if(state)
		{
			video_buffer.fb_base = (char *)(video + g_frame_len * v);
			FlushDisplayRegion(&video_buffer, &tscreen_Region);
			usleep(33300);//暂停时每次循环休息0.05s减轻运算压力
			v++;
		}
		else
			usleep(50000);//暂停时每次循环休息0.05s减轻运算压力
		
		/* 读取输入 */
		ret = GetInputEvent_unblock(&g_input);

		if (ret)
			continue;

		if ( g_input.iType == input_type_remote)
		{
			if (g_input.event.code == 0x15)//暂停
			{
				state = 1 - state;
			}
			else if(g_input.event.code == 0x9)//快进3s
			{
				v = v + 90;
			}
			else if(g_input.event.code == 0x45)//快退3s
			{
				v = v - 90;
				if(v < 0)
					v =  0;
			}
			else if(g_input.event.code == 0x43)//退出播放
			{
				break;
			}
			video_buffer.fb_base = (char *)(video + g_frame_len * v);
			FlushDisplayRegion(&video_buffer, &tscreen_Region);
		}
	}

	//关闭文件，释放内存
	fclose(fd);
	//video_buffer.fb_base = NULL;
	free(video);
}

static void ikun_on_pressed(void *pParams)
{
	char *name = "xiaoheizi";
	video_play(name, 1867);
}

static void list_on_pressed(void *pParams)
{
	return;
}

static void lists_regist(void)
{
	int list_num = 0;

	g_video_page_list.tlists[list_num].name = "ikun";
	g_video_page_list.tlists[list_num++].list_press = ikun_on_pressed;

	g_video_page_list.tlists[list_num].name = "liangfeifan";
	g_video_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_video_page_list.tlists[list_num].name = "kaijunjian";
	g_video_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_video_page_list.tlists[list_num].name = "xiyangyang";
	g_video_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_video_page_list.tlists[list_num].name = "gaoshu";
	g_video_page_list.tlists[list_num++].list_press = list_on_pressed;

	g_video_page_list.list_num = list_num;
}

static void video_page_run(void *pParams)
{
	int ret;
	int selected;

	/* list初始化 */
	lists_regist();
	list_page_init(&g_video_page_list);
	//分配页面和屏幕的buffer空间
	g_video_page_list.page_buffer.fb_base = (char *)calloc(g_video_page_list.line_size*LIST_Height*\
							 g_video_page_list.list_num, sizeof(char));
	g_video_page_list.screen_buffer.fb_base = (char *)calloc(g_video_page_list.list_size*LIST_NUM, sizeof(char));

	g_frame_len = g_video_page_list.line_size *g_video_page_list.screen_buffer.iyres;

	//生成页面buffer
	generate_list_page_buffer(&g_video_page_list);
	tscreen_Region.iRegion_x = 0;
	tscreen_Region.iRegion_y = 0;
	tscreen_Region.iRegion_width = g_video_page_list.screen_buffer.ixres;
	tscreen_Region.iRegion_height = g_video_page_list.screen_buffer.iyres;

	generate_list_screen_buffer(&g_video_page_list);
	FlushDisplayRegion(&g_video_page_list.screen_buffer, &tscreen_Region);

	while(1)
	{
		ret = GetInputEvent(&g_input);

		if (ret)
		{
			printf("GetInputEvent err!\n");
			continue;
		}
		if ( g_input.iType == input_type_remote)
		{
			if( g_input.event.code == 0x15)
			{
				selected = g_video_page_list.list_pos;
				g_video_page_list.tlists[selected].list_press(NULL);
			}
			else if( g_input.event.code == 0x19)
			{
				roll_animation(&g_video_page_list, 1);
			}
			else if(g_input.event.code == 0x40)
			{
				roll_animation(&g_video_page_list, -1);
			}
			else if(g_input.event.code == 0x43)
			{
				break;
			}

			generate_list_screen_buffer(&g_video_page_list);
			FlushDisplayRegion(&g_video_page_list.screen_buffer, &tscreen_Region);
		}	
	}

	/* 释放页面的buffer空间 */
	free(g_video_page_list.page_buffer.fb_base);
	free(g_video_page_list.screen_buffer.fb_base);
}

static Page_action g_tvideo_page = {
	.name = "video",
	.Run  = video_page_run,
};

void Video_page_register(void)
{
	Page_register(&g_tvideo_page);
}

