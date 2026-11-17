#include "mobile_platform.h"
#include <stdio.h>
#include "mobile_save_migration.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <android_native_app_glue.h>
extern struct android_app *GetAndroidApp(void);
bool __wrap_FileExists(const char *path) {
    FILE *f=fopen(path,"rb"); if(!f)return false; fclose(f); return true;
}
void MobilePrepareStorage(void) {
    struct android_app *app=GetAndroidApp();
    if(app && app->activity->internalDataPath)chdir(app->activity->internalDataPath);
    mkdir("saves",0700);
    // Retain a prior v1.5 install's files. v1.7 loaders accept legacy records.
    const char *names[]={"dungeons_stats.txt","dungeons_shop.txt","dungeons_settings.txt"};
    for(int i=0;i<3;i++) {
        char dst[128];snprintf(dst,sizeof(dst),"saves/%s",names[i]);
        FILE *exists=fopen(dst,"rb");if(exists){fclose(exists);continue;}
        FILE *src=fopen(names[i],"rb");if(!src)continue;
        FILE *out=fopen(dst,"wb");
        if(out){
            if(strcmp(names[i],"dungeons_shop.txt")==0)MobileCopyLegacyShop(src,out);
            else{char b[4096];size_t n;while((n=fread(b,1,sizeof(b),src))>0)fwrite(b,1,n,out);}
            fclose(out);
        }
        fclose(src);
    }
}
static bool MobileConsumeActivityBack(void);
bool MobileBackPressed(void){
    bool activityBack=MobileConsumeActivityBack();
    return activityBack||IsKeyPressed(KEY_BACK)||IsKeyPressed(KEY_ESCAPE);
}

#include "mobile_input.h"
extern float __real_GetMouseWheelMove(void);
extern Vector2 __real_GetMouseWheelMoveV(void);
float __wrap_GetMouseWheelMove(void){return __real_GetMouseWheelMove()+MobileGetScrollWheel();}
Vector2 __wrap_GetMouseWheelMoveV(void){Vector2 v=__real_GetMouseWheelMoveV();v.y+=MobileGetScrollWheel();return v;}

#include <jni.h>
#include <pthread.h>
#include <string.h>
static pthread_mutex_t seedMutex=PTHREAD_MUTEX_INITIALIZER;
static char pendingSeed[25];
static bool seedReady=false;
JNIEXPORT void JNICALL Java_com_dungeoncrawler_playfree_FullscreenNativeActivity_onSeedEntered(JNIEnv *env,jclass cls,jstring value){
    (void)cls;
    const char *text=(*env)->GetStringUTFChars(env,value,NULL);
    if(!text)return;
    pthread_mutex_lock(&seedMutex);
    snprintf(pendingSeed,sizeof(pendingSeed),"%s",text);seedReady=true;
    pthread_mutex_unlock(&seedMutex);
    (*env)->ReleaseStringUTFChars(env,value,text);
}
void MobileRequestSeedInput(const char *seed){
    struct android_app *app=GetAndroidApp();JNIEnv *env=NULL;bool attached=false;
    if((*app->activity->vm)->GetEnv(app->activity->vm,(void**)&env,JNI_VERSION_1_6)!=JNI_OK){
        if((*app->activity->vm)->AttachCurrentThread(app->activity->vm,&env,NULL)!=JNI_OK)return;
        attached=true;
    }
    jclass cls=(*env)->GetObjectClass(env,app->activity->clazz);
    // NativeActivity loads the shared object outside Java's loadLibrary list.
    // Register this callback explicitly with the actual Activity class.
    JNINativeMethod callback={"onSeedEntered","(Ljava/lang/String;)V",(void*)Java_com_dungeoncrawler_playfree_FullscreenNativeActivity_onSeedEntered};
    (*env)->RegisterNatives(env,cls,&callback,1);
    jmethodID method=(*env)->GetMethodID(env,cls,"requestSeedInput","(Ljava/lang/String;)V");
    if(method){jstring value=(*env)->NewStringUTF(env,seed?seed:"");(*env)->CallVoidMethod(env,app->activity->clazz,method,value);(*env)->DeleteLocalRef(env,value);}
    if((*env)->ExceptionCheck(env)){(*env)->ExceptionDescribe(env);(*env)->ExceptionClear(env);}
    (*env)->DeleteLocalRef(env,cls);
    if(attached)(*app->activity->vm)->DetachCurrentThread(app->activity->vm);
}
bool MobilePollSeedInput(char *buffer,unsigned capacity){
    pthread_mutex_lock(&seedMutex);bool ready=seedReady;
    if(ready&&capacity){snprintf(buffer,capacity,"%s",pendingSeed);seedReady=false;}
    pthread_mutex_unlock(&seedMutex);return ready;
}

// Android 13+ predictive back does not always emit a native KEY_BACK event.
// Consume the Activity callback instead of allowing Android to finish the game.
static bool MobileConsumeActivityBack(void){
    struct android_app *app=GetAndroidApp();
    if(!app||!app->activity)return false;
    JNIEnv *env=NULL;bool attached=false;
    if((*app->activity->vm)->GetEnv(app->activity->vm,(void**)&env,JNI_VERSION_1_6)!=JNI_OK){
        if((*app->activity->vm)->AttachCurrentThread(app->activity->vm,&env,NULL)!=JNI_OK)return false;
        attached=true;
    }
    static jmethodID consume;
    if(!consume){
        jclass cls=(*env)->GetObjectClass(env,app->activity->clazz);
        consume=(*env)->GetMethodID(env,cls,"consumeGameBack","()Z");
        (*env)->DeleteLocalRef(env,cls);
    }
    bool result=consume&&(*env)->CallBooleanMethod(env,app->activity->clazz,consume);
    if((*env)->ExceptionCheck(env)){(*env)->ExceptionClear(env);result=false;}
    if(attached)(*app->activity->vm)->DetachCurrentThread(app->activity->vm);
    return result;
}
