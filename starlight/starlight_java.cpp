#include "starlight_java.h"
#include "starlight_log.h"
#include <jni.h>

#include <stdio.h>

void slCreateJVM() {
    JavaVM *jvm;       /* denotes a Java VM */
    JNIEnv *env;       /* pointer to native method interface */
    JavaVMInitArgs vm_args; /* JDK/JRE 6 VM initialization arguments */
    JavaVMOption* options = new JavaVMOption[1];
    options[0].optionString = "-Djava.class.path=/usr/lib/java";
    vm_args.version = JNI_VERSION_1_6;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    /* load and initialize a Java VM, return a JNI interface
     * pointer in env */
    JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
    delete options;

    logger::LogInfo("JVM Created.");
    
    /* invoke the Main.test method using the JNI */
    //jclass cls = env->FindClass("Main");
    //jmethodID mid = env->GetStaticMethodID(cls, "test", "(I)V");
    //env->CallStaticVoidMethod(cls, mid, 100);
    /* We are done. */
    jvm->DestroyJavaVM();

    logger::LogInfo("JVM Destroyed.");
}
