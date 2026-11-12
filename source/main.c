#include "profile.h"
#include "reimpl/controls.h"
#include "utils/glutil.h"
#include "utils/init.h"
#include "utils/logger.h"

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

so_module so_mod;

int main()
{
    soloader_init_all();

    if (profile.onload) {
        int (*JNI_OnLoad)(void* vm, void* reserved) = (void*)so_symbol(&so_mod, "JNI_OnLoad");
        if (JNI_OnLoad)
            JNI_OnLoad(&jvm, NULL);
    }

    gl_init();

    profile_call(&profile.init, NULL);
    profile_call(&profile.resume, NULL);

    while (1) {
        controls_poll();
        profile_call(&profile.frame, NULL);
        gl_swap();
    }

    sceKernelExitDeleteThread(0);
}

void controls_handler_key(int32_t keycode, ControlsAction action)
{
    if (action == CONTROLS_ACTION_MOVE)
        return;
    ProfileEvent ev = {0};
    ev.keycode = keycode;
    ev.down = action == CONTROLS_ACTION_DOWN;
    ev.action = ev.down ? 0 : 1;
    profile_call(&profile.key, &ev);
}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action)
{
    ProfileEvent ev = {0};
    ev.id = id;
    ev.x = x * (float)profile.width / 960.0f;
    ev.y = y * (float)profile.height / 544.0f;
    ev.action = action == CONTROLS_ACTION_DOWN ? profile.touch_down : action == CONTROLS_ACTION_UP ? profile.touch_up : profile.touch_move;
    profile_call(&profile.touch, &ev);
}

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action)
{
    ProfileEvent ev = {0};
    ev.stick = which;
    ev.x = x;
    ev.y = y;
    ev.action = action;
    profile_call(&profile.analog, &ev);
}
