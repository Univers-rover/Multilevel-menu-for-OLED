#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#include <input_elc.h>

static pinputdevice pinput_dev;/* 输入设备链表头 */
/* 线程锁变量 */
static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;//用宏初始化
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

/* start of 实现环形buffer */
#define BUFFER_LEN 20
static int g_iRead  = 0;
static int g_iWrite = 0;
static inputevent g_atInputEvents[BUFFER_LEN];

static int isInputBufferFull(void)
{
	return (g_iRead == ((g_iWrite + 1) % BUFFER_LEN));
}

static int isInputBufferEmpty(void)
{
	return (g_iRead == g_iWrite);
}

static void PutInputEventToBuffer(pinputevent ptInputEvent)
{
	if (!isInputBufferFull())
	{
		g_atInputEvents[g_iWrite] = *ptInputEvent;
		g_iWrite = (g_iWrite + 1) % BUFFER_LEN;
	}
}

static int GetInputEventFromBuffer(pinputevent ptInputEvent)
{
    /* 有数据返回1 没数据返回0 */
	if (!isInputBufferEmpty())
	{
		*ptInputEvent = g_atInputEvents[g_iRead];
		g_iRead = (g_iRead + 1) % BUFFER_LEN;
		return 1;
	}
	else
	{
		return 0;
	}
}
/* end of 实现环形buffer */

void input_device_register(pinputdevice ptinputdevice)
{
    ptinputdevice->next = pinput_dev;
    pinput_dev = ptinputdevice;
}

static void *inputdev_thread_func(void *data)
{
    int ret;
    pinputdevice ptinputdevice = (pinputdevice)data;
    inputevent temp;

    while (1)
    {
        ret = ptinputdevice->device_getinputevent(&temp);
        if(!ret)
        {
            pthread_mutex_lock(&g_tMutex);//加锁之后获得锁的线程会被阻塞
            PutInputEventToBuffer(&temp);
            pthread_cond_signal(&g_tConVar); // 通知主线程读数据 
            pthread_mutex_unlock(&g_tMutex);//写完数据解锁
        }
    }

    return NULL;
}

int input_device_init(void)
{
    int ret ;
    pthread_t tid;
    pinputdevice ptemp = pinput_dev;
    
    while (ptemp)
    {
        /* init current device */
        ret = ptemp->device_init();
        if (ret)
        {
            printf("Dev: %s init err\n", ptemp->cname);
		    return -1;
        }
        /* create thread for current device */
        ret = pthread_create(&tid, NULL, inputdev_thread_func, ptemp);
        if (ret)
        {
            printf("%s_pthread_create err!\n", ptemp->cname);
            return -1;
        }
        ptemp = ptemp->next;
    }
    return 0;
}

int GetInputEvent(pinputevent ptinputevent)
{
    inputevent temp;

    pthread_mutex_lock(&g_tMutex);
    if (GetInputEventFromBuffer(&temp))//有数据，直接读到返回
    {
        *ptinputevent = temp;
        return 0;
    }
    else//没有数据，休眠等待
    {
        pthread_cond_wait(&g_tConVar, &g_tMutex);//
        if (GetInputEventFromBuffer(&temp))
        {
            *ptinputevent = temp;
            pthread_mutex_unlock(&g_tMutex);
            return 0;            
        }
        else
        {
            pthread_mutex_unlock(&g_tMutex);
            return -1;
        }
    }
}

void input_init(void)
{
    /* 向顶层app提供初始化函数注册各个输入设备 */
    extern void touchinput_register(void);
    touchinput_register();
    
    extern void netinput_register(void);
    netinput_register();
}
