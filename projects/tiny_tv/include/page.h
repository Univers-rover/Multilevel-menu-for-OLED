#ifndef _PAGE_H
#define _PAGE_H

typedef struct Page_action{
    char *name;
    void (* Run)(void *pParams);
    struct Page_action *Next;
}Page_action, *pPage_action;

/* 底层使用 */
void Page_register(pPage_action ptPage_action);

/* 顶层使用 */
pPage_action Page(char *name);
void Pages_register(void);

#endif