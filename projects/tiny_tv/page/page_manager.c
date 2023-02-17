#include <string.h>

#include <common.h>
#include <page.h>

static pPage_action g_ptPages = NULL;

void Page_register(pPage_action ptPage_action)
{
	ptPage_action->Next = g_ptPages;
	g_ptPages = ptPage_action;
}

pPage_action Page(char *name)
{
	pPage_action ptTmp = g_ptPages;

	while (ptTmp)
	{
		if (strcmp(name, ptTmp->name) == 0)
			return ptTmp;
		ptTmp = ptTmp->Next;
	}

	return NULL;
}

void Pages_register(void)
{
	extern void Main_page_register(void);
	Main_page_register();

	extern void Main_list_page_register(void);
	Main_list_page_register();

	extern void Video_page_register(void);
	Video_page_register();	
}

