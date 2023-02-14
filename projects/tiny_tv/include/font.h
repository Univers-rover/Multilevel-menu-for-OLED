#ifndef _FONT_H
#define _FONT_H

#ifndef NULL
#define NULL (void *)0
#endif

#include <common.h>

typedef struct FontBitMap{
    disp_region tRegion;
    int iCurBaseX;//当前字符基点坐标
    int iCurBaseY;
    int iNextBaseX;//下一字符基点坐标
    int iNextBaseY;
    unsigned char *pucBuffer;
}FontBitMap, *PFontBitMap;

typedef struct FontOpr{
    char *name;
    int (* FontInit)(char *FontName);
    int (* SetFontSize)(int iFontSize);//某些固定大小的字体可能不能设置大小，所以需要返回值
    int (* GetFontBitMap)(unsigned int dwCode, PFontBitMap ptFontBitMap);//输入字符编码和保存地址
    int (* GetStrRegionCar)(char *str, pRegion_cartesian ptRegion_cartesian);
    struct FontOpr *Next;
}FontOpr, *PFontOpr;

/* 设备层使用 */
void FontsRegister(PFontOpr ptFontOpr);

/* 管理层使用 */
void FreetypeRegister(void);

/* 应用层使用 */
void Fonts_init(void);
int SelectAndInitFont(char *aFont_opr_name, char *aFont_file_name);
int set_font_size(int iFontSize);
int get_font_bitmap(unsigned int dwCode, PFontBitMap ptFontBitMap);
int get_str_RegionCar(char *str, pRegion_cartesian ptRegion_cartesian);

#endif // !FONT_ELC_H
