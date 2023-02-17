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

static disp_buffer gtmain_page_screen;
static disp_region main_page_region;

static void Main_page_run(void *pParams)
{
	int ret;
    static pdisp_buffer tp_buffer;
	static inputevent input;
	char *sys_name = "ikun os v1.1";

    tp_buffer = GetDisplayBuffer();
	
	//为buffer分配内存空间,初始化buffer
	gtmain_page_screen.fb_base = (char *)calloc(tp_buffer->ixres*tp_buffer->iyres/8, sizeof(char));
    gtmain_page_screen.ixres = tp_buffer->ixres;
    gtmain_page_screen.iyres = tp_buffer->iyres;
    gtmain_page_screen.ibpp = tp_buffer->ibpp;

	main_page_region.iRegion_x = 0;
	main_page_region.iRegion_y = 40;
	main_page_region.iRegion_width = tp_buffer->ixres;
	main_page_region.iRegion_height = 24;

	draw_text_at_center(&gtmain_page_screen, sys_name, &main_page_region, 1, 18);
	
	main_page_region.iRegion_y = 0;
	main_page_region.iRegion_height = tp_buffer->iyres;
	FlushDisplayRegion(&gtmain_page_screen, &main_page_region);
	
	while(1)
	{
		ret = GetInputEvent(&input);
		if (ret){
			//printf("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
			printf("GetInputEvent err!\n");
			continue;
		}
		if ( input.iType == input_type_remote)
		{
			printf("input.event.code = %x \n", input.event.code);
			if (input.event.code == 0x47)
				Page("main_list")->Run(NULL);
			else if(input.event.code == 0x45)
				break;
			
			FlushDisplayRegion(&gtmain_page_screen, &main_page_region);
		}	
	}

	/* 释放页面的buffer空间 */
	free(gtmain_page_screen.fb_base);
} 

static Page_action g_tMain_page = {
	.name = "main",
	.Run  = Main_page_run,
};

void Main_page_register(void)
{
	Page_register(&g_tMain_page);
}

