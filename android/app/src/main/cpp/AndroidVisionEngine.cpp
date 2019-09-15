#include <jni.h>
#include <string>

#include "Settings.h"
#include "FrameProcessor.h"
#include "CameraTool.h"
#include "TuioServer.h"
#include "FrameThresholder.h"
#include "FidtrackFinder.h"
#include "CalibrationEngine.h"
#include "LogcatInterface.h"
#include "Logging.h"

using namespace TUIO;

#define FMT 1

struct AndroidVisionEngine {
    std::unique_ptr<TuioServer> server;
    std::unique_ptr<FrameProcessor> fiducialfinder;
    std::unique_ptr<FrameProcessor> thresholder;
    //std::unique_ptr<FrameProcessor> calibrator;
    std::unique_ptr<UserInterface> interface;
    std::vector<unsigned char> result;
    bool showVideo = false;
};

extern "C" JNIEXPORT jlong JNICALL
Java_mkalten_reactivision_VisionEngine_initPeer(JNIEnv *env, jobject thiz) {
    auto peer = new AndroidVisionEngine;
    return (jlong) peer;
}

extern "C" JNIEXPORT void JNICALL
Java_mkalten_reactivision_VisionEngine_deletePeer(JNIEnv *env, jobject, jlong peer) {
    auto* ptr = reinterpret_cast<AndroidVisionEngine*>(peer);
    delete ptr;
}

AndroidVisionEngine* ptr(JNIEnv* e, jobject jpeer) {
    jclass cls = e->GetObjectClass(jpeer);
    assert(cls != nullptr);
    jmethodID met = e->GetMethodID(cls, "getPeer", "()J");
    assert(met != nullptr);
    jlong jptr = e->CallLongMethod(jpeer, met);
    return reinterpret_cast<AndroidVisionEngine*>(jptr);
}

extern "C" JNIEXPORT void JNICALL
Java_mkalten_reactivision_VisionEngine_showVideo(JNIEnv *env,jobject thiz, jboolean jshow) {

    ptr(env, thiz)->showVideo = (bool) jshow;

}

extern "C" JNIEXPORT jboolean JNICALL
Java_mkalten_reactivision_VisionEngine_setup(JNIEnv *env,jobject thiz, jint w, jint h, jstring ip) {

    auto eng = ptr(env, thiz);

    eng->result.resize(w*h);

    application_settings s;
    sprintf(s.file, "none");
    s.headless = true;
    readSettings(&s);

    auto cIp = env->GetStringUTFChars(ip, nullptr);
    s.tuio_host[0] = std::string(cIp);
    env->ReleaseStringUTFChars(ip, cIp);

    s.tuio_port[0] = 3333;
    sprintf(s.tuio_source, "my_tuio_source");
    s.invert_x = false;
    s.invert_y = false;
    s.invert_a = false;

    s.gradient_gate = 32;
    s.tile_size = 5;
    s.thread_count = 1;

    sprintf(s.tree_config, "small");

    s.finger_size = 0;
    s.finger_sensitivity = 0;
    s.max_blob_size = 0;
    s.min_blob_size = 0;
    s.yamaarashi = false;
    s.object_blobs = false;
    s.cursor_blobs = false;
    s.max_fid = 100;
    s.obj_filter = false;
    s.cur_filter = false;
    s.blb_filter = false;

    OscSender* sender = new UdpSender(s.tuio_host[0].c_str(), s.tuio_port[0]);
    //OscSender* sender = new TcpSender(config.tuio_host[i].c_str(),config.tuio_port[i]);
    //OscSender* sender = new TcpSender(config.tuio_port[i]); break;

    eng->server = std::make_unique<TuioServer>(sender);
    // NB: server now owns the sender and deletes it on destruction
    eng->server->setSourceName(s.tuio_source);
    eng->server->setInversion(s.invert_x, s.invert_y, s.invert_a);

    eng->interface = std::make_unique<LogcatInterface>();

    eng->thresholder = std::make_unique<FrameThresholder>(s.gradient_gate, s.tile_size, s.thread_count);
    eng->thresholder->addUserInterface(eng->interface.get());
    bool ok = eng->thresholder->init(w,h,FMT,FMT);

    if (!ok) {
        eng->server.reset();
        eng->interface.reset();
        eng->thresholder.reset();
        return JNI_FALSE;
    }

    eng->fiducialfinder = std::make_unique<FidtrackFinder>(eng->server.get(), &s);
    eng->fiducialfinder->addUserInterface(eng->interface.get());
    ok = eng->fiducialfinder->init(w,h,FMT,FMT);

    if (!ok) {
        eng->server.reset();
        eng->interface.reset();
        eng->thresholder.reset();
        eng->fiducialfinder.reset();
        return JNI_FALSE;
    }

    // TODO: calibrator = new CalibrationEngine(config.grid_config);

    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_mkalten_reactivision_VisionEngine_process(JNIEnv *env, jobject thiz, jobject jInput) {

    auto eng = ptr(env, thiz);
    auto src =  (unsigned char*) env->GetDirectBufferAddress(jInput);
    long size = (long) env->GetDirectBufferCapacity(jInput);

    if (size != eng->result.size()) abort();

    eng->thresholder->process(src, eng->result.data());
    eng->fiducialfinder->process(src, eng->result.data());

    if (eng->showVideo) {
        memcpy(src, eng->result.data(), size);
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}