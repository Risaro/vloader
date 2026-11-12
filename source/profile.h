#ifndef VLOADER_PROFILE_H
#define VLOADER_PROFILE_H

#include <stdbool.h>
#include <stdint.h>

#include <so_util/so_util.h>

#define PROFILE_MAX_ARGS 12
#define PROFILE_MAX_START 16
#define PROFILE_MAX_JAVA 128

typedef struct {
    char symbol[128];
    char args[PROFILE_MAX_ARGS][128];
    int argc;
    uintptr_t fn;
} ProfileCall;

typedef struct {
    int32_t id;
    float x, y;
    int32_t action;
    int32_t keycode;
    int32_t down;
    int32_t stick;
} ProfileEvent;

typedef enum {
    JAVA_VOID,
    JAVA_INT,
    JAVA_BOOL,
    JAVA_FLOAT,
    JAVA_LONG,
    JAVA_STRING,
} ProfileJavaType;

typedef struct {
    char name[96];
    char value[256];
    ProfileJavaType type;
} ProfileJava;

typedef struct {
    char name[64];
    char so[64];
    char so_path[256];
    int onload;
    int width, height;
    ProfileCall start[PROFILE_MAX_START];
    int start_count;
    ProfileCall frame, touch, key, analog, pause, resume;
    int touch_down, touch_up, touch_move;
    ProfileJava java[PROFILE_MAX_JAVA];
    int java_count;
} Profile;

extern Profile profile;

bool profile_load(const char* path);
void profile_bind(so_module* mod);
int profile_call(const ProfileCall* call, const ProfileEvent* ev);

#endif
