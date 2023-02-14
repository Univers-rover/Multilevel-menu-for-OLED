#ifndef _INPUT_H
#define _INPUT_H

#ifndef NULL
#define NULL (void *)0
#endif

#include <sys/time.h>

#define input_type_touchscreen 1
#define input_type_net         2

/* 输入事件结构体 */
typedef struct inputevent{
    struct timeval	tTime;
    int iType;
    int ix;
    int iy;
    int iPressure;
    char mesg[1024];
}inputevent, *pinputevent;

typedef struct inputdevice{
    char *cname;
    int (*device_init)(void);
    int (*device_exit)(void);
    int (*device_getinputevent)(pinputevent ptinputevent);
    struct inputdevice *next;
}inputdevice, *pinputdevice;

/* 设备层使用 */
void input_device_register(pinputdevice ptinputdevice);

/* 管理层使用 */
void touchinput_register(void);
void netinput_register  (void);

/* 应用层使用 */
void input_init(void);
int  GetInputEvent(pinputevent ptinputevent);
int  input_device_init(void);

#endif