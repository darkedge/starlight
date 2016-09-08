// Launch command
// cl /I"C:\Program Files\Java\jdk1.8.0_102\include" /I"C:\Program Files\Java\jdk1.8.0_102\include\win32" starlight_java.cpp /link /delayload:jvm.dll /libpath:"C:\Program Files\Java\jdk1.8.0_102\lib" jvm.lib Shell32.lib Ole32.lib Delayimp.lib

#include <jni.h>

#include <stdio.h>    
#include <string>

int main() {
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

    printf("yay!\n");
    
    /* invoke the Main.test method using the JNI */
    //jclass cls = env->FindClass("Main");
    //jmethodID mid = env->GetStaticMethodID(cls, "test", "(I)V");
    //env->CallStaticVoidMethod(cls, mid, 100);
    /* We are done. */
    jvm->DestroyJavaVM();
}
