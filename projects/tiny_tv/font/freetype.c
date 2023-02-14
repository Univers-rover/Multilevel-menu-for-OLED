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

#include <common.h>
#include <ft2build.h>
#include <font.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static FT_Face g_tFace;
static int g_DefaultFontSize = 12;

static int freetype_get_str_box(char *str, pRegion_cartesian ptRegion_cartesian)
{
    int i;
    int error;

    FT_BBox bbox;
    FT_BBox glyph_bbox;

    FT_Vector pen;
    FT_Glyph  glyph;
    FT_GlyphSlot slot = g_tFace->glyph;

    /* 初始化 */
    bbox.xMin = bbox.yMin = 32000;
    bbox.xMax = bbox.yMax = -32000;

    /* 指定原点为(0, 0) */
    pen.x = 0;
    pen.y = 0;

    /* 计算每个字符的bounding box */
    /* 先translate, 再load char, 就可以得到它的外框了 */
    for (i = 0; i < strlen(str); i++)
    {
        /* 转换：transformation */
        FT_Set_Transform(g_tFace, 0, &pen);

        /* 加载位图: load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(g_tFace, str[i], FT_LOAD_RENDER);
        if (error)
        {
            printf("FT_Load_Char error\n");
            return -1;
        }

        /* 取出glyph */
        error = FT_Get_Glyph(g_tFace->glyph, &glyph);
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

    /* Region_cartesian */
    ptRegion_cartesian->iLeftUpX = bbox.xMin;
    ptRegion_cartesian->iLeftUpY = bbox.yMin;
    ptRegion_cartesian->iWidth   = bbox.xMax - bbox.xMin +1 ;
    ptRegion_cartesian->iHeight   = bbox.yMax - bbox.yMin +1 ;

    return 0;
}

static int freetype_init(char *FontName)
{
    FT_Library    library;

    int error;

    error = FT_Init_FreeType( &library );              /* initialize library */
    if (error)
    {
        printf("FT_Init FreeType_error\n");
    }

    error = FT_New_Face( library, FontName, 0, &g_tFace ); /* 加载字体文件 */
    if (error)
    {
        printf("FT New Face error\n");
    }

    FT_Set_Pixel_Sizes(g_tFace, g_DefaultFontSize, 0);//设置字体为默认大小，避免忘记设置

    return 0;
}

static int freetype_SetFontSize(int iFontSize)
{
    FT_Set_Pixel_Sizes(g_tFace, iFontSize, 0);
    return 0;
}

static int freetype_GetFontBitMap(unsigned int dwCode, PFontBitMap ptFontBitMap)
{
    int ret;
    FT_Vector pen;
    FT_GlyphSlot slot = g_tFace->glyph;
    pen.x = ptFontBitMap->iCurBaseX * 64; /* 单位: 1/64像素 */
    pen.y = ptFontBitMap->iCurBaseY * 64; /* 单位: 1/64像素 */

    /* 转换：transformation */
	FT_Set_Transform(g_tFace, 0, &pen);

	/* 加载位图: load glyph image into the slot (erase previous one) */
	ret = FT_Load_Char(g_tFace, dwCode, FT_LOAD_RENDER);
	if (ret)
	{
		printf("FT_Load_Char error\n");
		return -1;
	}

    ptFontBitMap->pucBuffer = slot->bitmap.buffer;

    ptFontBitMap->tRegion.iRegion_x      = slot->bitmap_left;
    ptFontBitMap->tRegion.iRegion_y      = 2*ptFontBitMap->iCurBaseY-slot->bitmap_top;
    ptFontBitMap->tRegion.iRegion_width  = slot->bitmap.width;
    ptFontBitMap->tRegion.iRegion_height = slot->bitmap.rows;
    ptFontBitMap->iNextBaseX = ptFontBitMap->iCurBaseX + slot->advance.x / 64;//下一字符基点坐标
    ptFontBitMap->iNextBaseY = ptFontBitMap->iCurBaseY;

    return 0;
}

static FontOpr g_tFreetypeOpr = {
    .name          = "freetype",
    .FontInit      = freetype_init,
    .SetFontSize   = freetype_SetFontSize,
    .GetFontBitMap = freetype_GetFontBitMap,
    .GetStrRegionCar = freetype_get_str_box,
};

void FreetypeRegister(void)
{
	FontsRegister(&g_tFreetypeOpr);
}
