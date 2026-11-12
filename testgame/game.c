#include <jni.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "testgame", __VA_ARGS__)

static const char* VS =
    "attribute vec2 aPos;\n"
    "attribute vec3 aColor;\n"
    "uniform vec4 uXform;\n"
    "varying vec3 vColor;\n"
    "void main() {\n"
    "  float c = cos(uXform.z);\n"
    "  float s = sin(uXform.z);\n"
    "  vec2 p = vec2(aPos.x * c - aPos.y * s, aPos.x * s + aPos.y * c) * uXform.w;\n"
    "  gl_Position = vec4(p.x * 0.5666 + uXform.x, p.y + uXform.y, 0.0, 1.0);\n"
    "  vColor = aColor;\n"
    "}\n";

static const char* FS =
    "precision mediump float;\n"
    "varying vec3 vColor;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(vColor, 1.0);\n"
    "}\n";

static GLuint program, vbo;
static GLint uXform;
static int width = 960, height = 544;
static float angle, speed = 0.02f;
static float touch_x = 0.6f, touch_y = -0.5f;
static int touching, frames, refresh = 60;
static float bg[3] = {0.05f, 0.07f, 0.12f};
static char data_line[64] = "no data file";

static GLuint compile(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = "";
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        LOG("shader %d failed: %s", (int)type, log);
    }
    return s;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOG("JNI_OnLoad");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_vpkgj_test_Native_init(JNIEnv* env, jclass cls, jint w, jint h)
{
    width = w;
    height = h;
    LOG("init %dx%d", w, h);

    jclass host = (*env)->FindClass(env, "com/vpkgj/test/Host");
    if (host) {
        jmethodID m = (*env)->GetStaticMethodID(env, host, "getRefreshRate", "()I");
        if (m)
            refresh = (*env)->CallStaticIntMethod(env, host, m);
        m = (*env)->GetStaticMethodID(env, host, "getLanguage", "()Ljava/lang/String;");
        if (m) {
            jstring js = (jstring)(*env)->CallStaticObjectMethod(env, host, m);
            const char* lang = js ? (*env)->GetStringUTFChars(env, js, NULL) : NULL;
            LOG("language %s refresh %d", lang ? lang : "?", refresh);
            if (lang && !strcmp(lang, "ru")) {
                bg[0] = 0.10f;
                bg[1] = 0.05f;
                bg[2] = 0.14f;
            }
            if (lang)
                (*env)->ReleaseStringUTFChars(env, js, lang);
        }
        m = (*env)->GetStaticMethodID(env, host, "getDataPath", "()Ljava/lang/String;");
        if (m) {
            jstring js = (jstring)(*env)->CallStaticObjectMethod(env, host, m);
            const char* dir = js ? (*env)->GetStringUTFChars(env, js, NULL) : NULL;
            if (dir) {
                char path[256];
                snprintf(path, sizeof(path), "%shello.txt", dir);
                FILE* f = fopen(path, "r");
                if (f) {
                    if (!fgets(data_line, sizeof(data_line), f))
                        strcpy(data_line, "empty data file");
                    fclose(f);
                }
                LOG("data %s: %s", path, data_line);
                (*env)->ReleaseStringUTFChars(env, js, dir);
            }
        }
    }

    GLuint vs = compile(GL_VERTEX_SHADER, VS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, FS);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "aPos");
    glBindAttribLocation(program, 1, "aColor");
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    LOG("link %d", ok);
    uXform = glGetUniformLocation(program, "uXform");

    static const GLfloat verts[] = {
        0.0f,  0.6f,  1.0f, 0.3f, 0.3f,
        -0.52f, -0.3f, 0.3f, 1.0f, 0.3f,
        0.52f, -0.3f, 0.3f, 0.4f, 1.0f,
        -0.5f, -0.5f, 1.0f, 0.9f, 0.2f,
        0.5f, -0.5f, 1.0f, 0.9f, 0.2f,
        -0.5f, 0.5f, 1.0f, 0.5f, 0.1f,
        0.5f, 0.5f, 1.0f, 0.5f, 0.1f,
    };
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glViewport(0, 0, width, height);
}

JNIEXPORT void JNICALL Java_com_vpkgj_test_Native_step(JNIEnv* env, jclass cls)
{
    angle += speed;
    frames++;
    if (frames % 300 == 0)
        LOG("frame %d angle %.2f", frames, angle);

    glClearColor(bg[0], bg[1], bg[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glUniform4f(uXform, -0.35f, 0.0f, angle, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUniform4f(uXform, touch_x, touch_y, touching ? angle * 2.0f : 0.0f, touching ? 0.35f : 0.2f);
    glDrawArrays(GL_TRIANGLE_STRIP, 3, 4);
}

JNIEXPORT void JNICALL Java_com_vpkgj_test_Native_touch(JNIEnv* env, jclass cls, jint action, jfloat x, jfloat y)
{
    touching = action != 0;
    touch_x = x / (float)width * 2.0f - 1.0f;
    touch_y = 1.0f - y / (float)height * 2.0f;
    if (action != 2)
        LOG("touch %d %.0f %.0f", action, x, y);
}

JNIEXPORT void JNICALL Java_com_vpkgj_test_Native_key(JNIEnv* env, jclass cls, jint keycode, jint down)
{
    LOG("key %d %d", keycode, down);
    if (!down)
        return;
    if (keycode == 96)
        speed = speed > 0.0f ? speed * 1.5f : 0.02f;
    else if (keycode == 97)
        speed = 0.0f;
    else if (keycode == 99)
        speed = -speed;
}
