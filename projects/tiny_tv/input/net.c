#include <sys/types.h>          
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

#include <input_my.h>

#define SERVER_PORT 8888

static int iSocketServer;

static int netinput_init(void)
{
    int ret;
    struct sockaddr_in tSocketServerAddr;

    iSocketServer = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == iSocketServer)
	{
		printf("socket error!\n");
		return -1;
	}

	tSocketServerAddr.sin_family      = AF_INET;
	tSocketServerAddr.sin_port        = htons(SERVER_PORT);  /* host to net, short */
 	tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;
	memset(tSocketServerAddr.sin_zero, 0, 8);

    ret = bind(iSocketServer, (const struct sockaddr *)&tSocketServerAddr, sizeof(struct sockaddr));
	if (-1 == ret)
	{
		printf("bind error!\n");
		return -1;
	}

    return 0;
}

static int netinput_exit(void)
{
	close(iSocketServer);

    return 0;
}

static int netinput_getinputevent(pinputevent ptInputEvent)
{
    int iRecvLen;
    char ucRecvBuf[1000];
	
	struct sockaddr_in tSocketClientAddr;

    unsigned int iAddrLen = sizeof(struct sockaddr);
    iRecvLen = recvfrom(iSocketServer, ucRecvBuf, 999, 0, (struct sockaddr *)&tSocketClientAddr, &iAddrLen);

    if (iRecvLen > 0)
    {
        ptInputEvent->iType           = input_type_net;
        ucRecvBuf[iRecvLen]           = '\0';
        strncpy(ptInputEvent->mesg, ucRecvBuf, 1000);
        ptInputEvent->mesg [999]      = '\0';
        gettimeofday(&ptInputEvent->tTime, NULL);
    }

    return 0;
}

static inputdevice netinput = {
    .cname                = "netinput",
    .device_init          = netinput_init,
    .device_exit          = netinput_exit,
    .device_getinputevent = netinput_getinputevent,
};


void netinput_register(void)
{
    input_device_register(&netinput);
}


#if 0

int main(int argc, char **argv)
{
	inputevent event;
	int ret;
	
	netinput.device_init();

	while (1)
	{
		ret = netinput.device_getinputevent(&event);
		if (ret) {
			printf("GetInputEvent err!\n");
			return -1;
		}
		else
		{
			printf("Type      : %d\n", event.iType);
			printf("str       : %s\n", event.mesg);
		}
	}
	return 0;
}

#endif
