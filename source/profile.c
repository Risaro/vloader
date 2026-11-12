#include "profile.h"

#include "utils/datapath.h"
#include "utils/logger.h"
#include "utils/utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <falso_jni/FalsoJNI.h>

#include <psp2/kernel/processmgr.h>

Profile profile;

static char* trim(char* s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    char* e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

static void copy(char* dst, size_t cap, const char* src)
{
    snprintf(dst, cap, "%s", src);
}

static void parse_call(ProfileCall* c, char* v)
{
    memset(c, 0, sizeof(*c));
    char* save = NULL;
    char* tok = strtok_r(v, " \t", &save);
    if (!tok)
        return;
    copy(c->symbol, sizeof(c->symbol), tok);
    while ((tok = strtok_r(NULL, " \t", &save)) && c->argc < PROFILE_MAX_ARGS)
        copy(c->args[c->argc++], sizeof(c->args[0]), tok);
}

static void expand(char* dst, size_t cap, const char* src)
{
    if (!strncmp(src, "@data/", 6))
        snprintf(dst, cap, "%s%s", data_path(), src + 6);
    else if (!strcmp(src, "@data"))
        snprintf(dst, cap, "%s", data_path());
    else
        snprintf(dst, cap, "%s", src);
}

static void add_java(ProfileJavaType type, const char* name, const char* value)
{
    if (profile.java_count >= PROFILE_MAX_JAVA)
        return;
    ProfileJava* j = &profile.java[profile.java_count++];
    j->type = type;
    copy(j->name, sizeof(j->name), name);
    expand(j->value, sizeof(j->value), value);
}

bool profile_load(const char* path)
{
    memset(&profile, 0, sizeof(profile));
    profile.width = 960;
    profile.height = 544;
    profile.onload = 1;
    profile.touch_down = 0;
    profile.touch_up = 1;
    profile.touch_move = 2;

    uint8_t* buf = NULL;
    size_t size = 0;
    if (!file_load(path, &buf, &size) || !buf)
        return false;
    char* text = malloc(size + 1);
    memcpy(text, buf, size);
    text[size] = 0;
    free(buf);

    char* save = NULL;
    for (char* line = strtok_r(text, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        line = trim(line);
        if (!*line || *line == '#')
            continue;
        char* eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = 0;
        char* k = trim(line);
        char* v = trim(eq + 1);

        if (!strcmp(k, "name"))
            copy(profile.name, sizeof(profile.name), v);
        else if (!strcmp(k, "keep_apk"))
            continue;
        else if (!strcmp(k, "so"))
            copy(profile.so, sizeof(profile.so), v);
        else if (!strcmp(k, "onload"))
            profile.onload = atoi(v);
        else if (!strcmp(k, "width"))
            profile.width = atoi(v);
        else if (!strcmp(k, "height"))
            profile.height = atoi(v);
        else if (!strcmp(k, "init") || !strcmp(k, "start")) {
            if (profile.start_count < PROFILE_MAX_START)
                parse_call(&profile.start[profile.start_count++], v);
        } else if (!strcmp(k, "mkdir")) {
            char dir[320];
            expand(dir, sizeof(dir), v);
            file_mkpath(dir, 0777);
        }
        else if (!strcmp(k, "frame"))
            parse_call(&profile.frame, v);
        else if (!strcmp(k, "touch"))
            parse_call(&profile.touch, v);
        else if (!strcmp(k, "key"))
            parse_call(&profile.key, v);
        else if (!strcmp(k, "analog"))
            parse_call(&profile.analog, v);
        else if (!strcmp(k, "pause"))
            parse_call(&profile.pause, v);
        else if (!strcmp(k, "resume"))
            parse_call(&profile.resume, v);
        else if (!strcmp(k, "touch.down"))
            profile.touch_down = atoi(v);
        else if (!strcmp(k, "touch.up"))
            profile.touch_up = atoi(v);
        else if (!strcmp(k, "touch.move"))
            profile.touch_move = atoi(v);
        else if (!strncmp(k, "java.void.", 10))
            add_java(JAVA_VOID, k + 10, v);
        else if (!strncmp(k, "java.int.", 9))
            add_java(JAVA_INT, k + 9, v);
        else if (!strncmp(k, "java.bool.", 10))
            add_java(JAVA_BOOL, k + 10, v);
        else if (!strncmp(k, "java.float.", 11))
            add_java(JAVA_FLOAT, k + 11, v);
        else if (!strncmp(k, "java.long.", 10))
            add_java(JAVA_LONG, k + 10, v);
        else if (!strncmp(k, "java.string.", 12))
            add_java(JAVA_STRING, k + 12, v);
        else
            l_warn("profile: unknown key %s", k);
    }
    free(text);

    if (!profile.so[0])
        return false;
    snprintf(profile.so_path, sizeof(profile.so_path), "%s%s", data_path(), profile.so);
    return true;
}

static void bind_call(so_module* mod, ProfileCall* c, const char* what)
{
    if (!c->symbol[0])
        return;
    c->fn = so_symbol(mod, c->symbol);
    if (!c->fn)
        l_error("profile: %s symbol %s not found", what, c->symbol);
}

void profile_bind(so_module* mod)
{
    for (int i = 0; i < profile.start_count; i++)
        bind_call(mod, &profile.start[i], "start");
    bind_call(mod, &profile.frame, "frame");
    bind_call(mod, &profile.touch, "touch");
    bind_call(mod, &profile.key, "key");
    bind_call(mod, &profile.analog, "analog");
    bind_call(mod, &profile.pause, "pause");
    bind_call(mod, &profile.resume, "resume");
}

static uint32_t fbits(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

static uint32_t arg_value(const char* a, const ProfileEvent* ev)
{
    static const ProfileEvent none;
    if (!ev)
        ev = &none;
    if (!strcmp(a, "env"))
        return (uint32_t)&jni;
    if (!strcmp(a, "vm"))
        return (uint32_t)&jvm;
    if (!strcmp(a, "class") || !strcmp(a, "obj"))
        return 0x42424242;
    if (!strcmp(a, "width"))
        return (uint32_t)profile.width;
    if (!strcmp(a, "height"))
        return (uint32_t)profile.height;
    if (!strncmp(a, "str:", 4)) {
        char text[320];
        expand(text, sizeof(text), a + 4);
        return (uint32_t)jni->NewStringUTF(&jni, text);
    }
    if (!strcmp(a, "pad"))
        return 0;
    if (!strcmp(a, "time"))
        return (uint32_t)(sceKernelGetProcessTimeWide() / 1000);
    if (!strcmp(a, "x"))
        return fbits(ev->x);
    if (!strcmp(a, "y"))
        return fbits(ev->y);
    if (!strcmp(a, "xn"))
        return fbits(profile.width ? ev->x / (float)profile.width : 0.0f);
    if (!strcmp(a, "yn"))
        return fbits(profile.height ? ev->y / (float)profile.height : 0.0f);
    if (!strcmp(a, "xi"))
        return (uint32_t)(int32_t)ev->x;
    if (!strcmp(a, "yi"))
        return (uint32_t)(int32_t)ev->y;
    if (!strcmp(a, "id"))
        return (uint32_t)ev->id;
    if (!strcmp(a, "action"))
        return (uint32_t)ev->action;
    if (!strcmp(a, "keycode"))
        return (uint32_t)ev->keycode;
    if (!strcmp(a, "down"))
        return (uint32_t)ev->down;
    if (!strcmp(a, "stick"))
        return (uint32_t)ev->stick;
    size_t n = strlen(a);
    if (n > 1 && a[n - 1] == 'f' && strncmp(a, "0x", 2))
        return fbits(strtof(a, NULL));
    return (uint32_t)strtoul(a, NULL, 0);
}

typedef int (*call12)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

int profile_call(const ProfileCall* call, const ProfileEvent* ev)
{
    if (!call->fn)
        return 0;
    uint32_t a[PROFILE_MAX_ARGS] = {0};
    for (int i = 0; i < call->argc; i++)
        a[i] = arg_value(call->args[i], ev);
    return ((call12)call->fn)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]);
}
