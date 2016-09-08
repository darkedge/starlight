#include "starlight_java.h"
#include "starlight_java_generated.h"
#include "starlight_log.h"

#include <stdio.h>

static JavaVM *sl_jvm;

void Java_ModdingApi_print(JNIEnv *env, jobject obj) {
    // State needs to be set here
    logger::LogInfo("C++ function called from Java!");
}

void Java_ModdingApi_copyState(JNIEnv *, jobject, jlong) {
    // TODO: Assign the state here
}

void slCreateJVM() {
    JNIEnv *env;       /* pointer to native method interface */
	JavaVMInitArgs vm_args = {}; /* JDK/JRE 6 VM initialization arguments */
    JavaVMOption options[2] = {};
    options[0].optionString = "-Djava.class.path=modding-api/out/production/modding-api";
    options[1].optionString = "-Djava.library.path=.";
    vm_args.version = JNI_VERSION_1_6;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    /* load and initialize a Java VM, return a JNI interface
     * pointer in env */
    JNI_CreateJavaVM(&sl_jvm, (void**)&env, &vm_args);

    logger::LogInfo("JVM Created.");
    

    jclass cls = env->FindClass("ModdingApi");
    assert(cls);


    jmethodID mid = env->GetStaticMethodID(cls, "start", "(J)Ljava/lang/String;");
    assert(mid);

    // TODO: This should be our state pointer
    void* ptr = NULL;
    jstring returnString = (jstring) env->CallObjectMethod(cls, mid, (jlong) ptr);

    const char *js = env->GetStringUTFChars(returnString, NULL);
    assert(js);
    std::string cs(js);
    env->ReleaseStringUTFChars(returnString, js);
    
    logger::LogInfo(cs);
}

void slDestroyJVM() {
    if (sl_jvm) sl_jvm->DestroyJavaVM();
}
