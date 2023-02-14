#ifndef _COMMON_H
#define _COMMON_H

#ifndef NULL
#define NULL (void *)0
#endif

#define	GET_BIT(x, bit)	((x & (1 << bit)) >> bit)	/* 获取第bit位 */

typedef struct disp_region{
    int iRegion_x;
    int iRegion_y;
    int iRegion_width;
    int iRegion_height;
}disp_region, *pdisp_region;

typedef struct Region_cartesian {
	int iLeftUpX;
	int iLeftUpY;
	int iWidth;
	int iHeight;
}Region_cartesian, *pRegion_cartesian;

#endif