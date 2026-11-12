#include "utils/datapath.h"

#include <stdio.h>
#include <string.h>

#include <psp2/appmgr.h>

static char id[256];
static char path[300];

const char* title_id(void)
{
    if (!id[0]) {
        if (sceAppMgrAppParamGetString(0, 12, id, sizeof(id)) < 0 || !id[0])
            strcpy(id, "VLDR00001");
        id[9] = 0;
    }
    return id;
}

const char* data_path(void)
{
    if (!path[0])
        snprintf(path, sizeof(path), "ux0:data/vloader/%s/", title_id());
    return path;
}
