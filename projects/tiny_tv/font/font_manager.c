#include <stdio.h>
#include <string.h>

#include <font.h>

static PFontOpr g_ptFontopr;//链表头
static PFontOpr g_ptdefaut_Fontopr;

void FontsRegister(PFontOpr ptFontOpr)
{
    ptFontOpr->Next = g_ptFontopr;
    g_ptFontopr = ptFontOpr;
}

int SelectAndInitFont(char *aFont_opr_name, char *aFont_file_name)
{
    PFontOpr ptemp = g_ptFontopr;

    while(ptemp)
    {
        if(strcmp(ptemp->name, aFont_opr_name) == 0)
        {
            g_ptdefaut_Fontopr = ptemp;
            g_ptdefaut_Fontopr->FontInit(aFont_file_name);
            return 0;
        }
        ptemp = ptemp->Next;
    }
    return -1;
}

int set_font_size(int iFontSize)
{
    return g_ptdefaut_Fontopr->SetFontSize(iFontSize);
}

int get_font_bitmap(unsigned int dwCode, PFontBitMap ptFontBitMap)
{
     return g_ptdefaut_Fontopr->GetFontBitMap(dwCode, ptFontBitMap);
}

int get_str_RegionCar(char *str, pRegion_cartesian ptRegion_cartesian)
{
    return g_ptdefaut_Fontopr->GetStrRegionCar(str, ptRegion_cartesian);
}

void Fonts_init(void)
{
    extern void FreetypeRegister(void);
    FreetypeRegister();
}

