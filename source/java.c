#include "profile.h"

#include <stdlib.h>
#include <string.h>

#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>

#define METHOD_ID_BASE 1000

NameToMethodID nameToMethodId[PROFILE_MAX_JAVA];

MethodsBoolean methodsBoolean[PROFILE_MAX_JAVA];
MethodsByte methodsByte[1];
MethodsChar methodsChar[1];
MethodsDouble methodsDouble[1];
MethodsFloat methodsFloat[PROFILE_MAX_JAVA];
MethodsInt methodsInt[PROFILE_MAX_JAVA];
MethodsLong methodsLong[PROFILE_MAX_JAVA];
MethodsObject methodsObject[PROFILE_MAX_JAVA];
MethodsShort methodsShort[1];
MethodsVoid methodsVoid[PROFILE_MAX_JAVA];

static size_t n_names, n_bool, n_float, n_int, n_long, n_object, n_void;

static const ProfileJava* by_id(jmethodID id)
{
    int i = (int)id - METHOD_ID_BASE;
    if (i < 0 || i >= profile.java_count)
        return NULL;
    return &profile.java[i];
}

static void java_void(jmethodID id, va_list args)
{
}

static jint java_int(jmethodID id, va_list args)
{
    const ProfileJava* j = by_id(id);
    return j ? (jint)strtol(j->value, NULL, 0) : 0;
}

static jboolean java_bool(jmethodID id, va_list args)
{
    const ProfileJava* j = by_id(id);
    return j && (!strcmp(j->value, "true") || atoi(j->value)) ? JNI_TRUE : JNI_FALSE;
}

static jfloat java_float(jmethodID id, va_list args)
{
    const ProfileJava* j = by_id(id);
    return j ? strtof(j->value, NULL) : 0.0f;
}

static jlong java_long(jmethodID id, va_list args)
{
    const ProfileJava* j = by_id(id);
    return j ? (jlong)strtoll(j->value, NULL, 0) : 0;
}

static jobject java_string(jmethodID id, va_list args)
{
    const ProfileJava* j = by_id(id);
    return j ? (jobject)jni->NewStringUTF(&jni, j->value) : NULL;
}

void java_from_profile(void)
{
    n_names = n_bool = n_float = n_int = n_long = n_object = n_void = 0;
    for (int i = 0; i < profile.java_count; i++) {
        ProfileJava* j = &profile.java[i];
        int id = METHOD_ID_BASE + i;
        METHOD_TYPE type = METHOD_TYPE_VOID;
        switch (j->type) {
        case JAVA_VOID:
            methodsVoid[n_void++] = (MethodsVoid){id, java_void};
            type = METHOD_TYPE_VOID;
            break;
        case JAVA_INT:
            methodsInt[n_int++] = (MethodsInt){id, java_int};
            type = METHOD_TYPE_INT;
            break;
        case JAVA_BOOL:
            methodsBoolean[n_bool++] = (MethodsBoolean){id, java_bool};
            type = METHOD_TYPE_BOOLEAN;
            break;
        case JAVA_FLOAT:
            methodsFloat[n_float++] = (MethodsFloat){id, java_float};
            type = METHOD_TYPE_FLOAT;
            break;
        case JAVA_LONG:
            methodsLong[n_long++] = (MethodsLong){id, java_long};
            type = METHOD_TYPE_LONG;
            break;
        case JAVA_STRING:
            methodsObject[n_object++] = (MethodsObject){id, java_string};
            type = METHOD_TYPE_OBJECT;
            break;
        }
        nameToMethodId[n_names++] = (NameToMethodID){id, j->name, type};
    }
}

char WINDOW_SERVICE[] = "window";
const int SDK_INT = 19;

NameToFieldID nameToFieldId[] = {
    {0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT},
    {1, "SDK_INT", FIELD_TYPE_INT},
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
    {1, SDK_INT},
};
FieldsObject fieldsObject[] = {
    {0, WINDOW_SERVICE},
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

size_t nameToMethodId_size() { return n_names * sizeof(NameToMethodID); }
size_t methodsBoolean_size() { return n_bool * sizeof(MethodsBoolean); }
size_t methodsByte_size() { return 0; }
size_t methodsChar_size() { return 0; }
size_t methodsDouble_size() { return 0; }
size_t methodsFloat_size() { return n_float * sizeof(MethodsFloat); }
size_t methodsInt_size() { return n_int * sizeof(MethodsInt); }
size_t methodsLong_size() { return n_long * sizeof(MethodsLong); }
size_t methodsObject_size() { return n_object * sizeof(MethodsObject); }
size_t methodsShort_size() { return 0; }
size_t methodsVoid_size() { return n_void * sizeof(MethodsVoid); }
size_t nameToFieldId_size() { return sizeof nameToFieldId; }
size_t fieldsBoolean_size() { return sizeof fieldsBoolean; }
size_t fieldsByte_size() { return sizeof fieldsByte; }
size_t fieldsChar_size() { return sizeof fieldsChar; }
size_t fieldsDouble_size() { return sizeof fieldsDouble; }
size_t fieldsFloat_size() { return sizeof fieldsFloat; }
size_t fieldsInt_size() { return sizeof fieldsInt; }
size_t fieldsObject_size() { return sizeof fieldsObject; }
size_t fieldsLong_size() { return sizeof fieldsLong; }
size_t fieldsShort_size() { return sizeof fieldsShort; }
