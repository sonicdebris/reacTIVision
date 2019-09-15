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

TuioServer* server = nullptr;
FrameProcessor* fiducialfinder	= nullptr;
FrameProcessor* thresholder	= nullptr;
//FrameProcessor* calibrator	= nullptr;
UserInterface* interface = nullptr;
unsigned char* result = nullptr;
int frameSize = 0;

#define FMT 1

extern "C" JNIEXPORT jboolean JNICALL
Java_mkalten_reactivision_VisionEngine_setup(JNIEnv *env,jobject) {

    int w = 640;
    int h = 480;
    result = new unsigned char[w*h];
    frameSize = w*h;

    application_settings s;
    sprintf(s.file, "none");
    s.headless = true;
    readSettings(&s);

    s.tuio_host[0] = "192.168.1.47";
    s.tuio_port[0] = 3333;
    sprintf(s.tuio_source, "my_tuio_source");
    s.invert_x = false;
    s.invert_y = false;
    s.invert_a = false;

    s.gradient_gate = 32;
    s.tile_size = 5;
    s.thread_count = 1;

    //s.tree_config = ""
    s.finger_size = 0;
    s.finger_sensitivity = 0;
    s.max_blob_size = 100;
    s.min_blob_size = 30;
    s.yamaarashi = false;
    s.object_blobs = false;
    s.cursor_blobs = false;
    s.max_fid = 20;
    s.obj_filter = false;
    s.cur_filter = false;
    s.blb_filter = false;

    OscSender* sender = new UdpSender(s.tuio_host[0].c_str(), s.tuio_port[0]);
    //OscSender* sender = new TcpSender(config.tuio_host[i].c_str(),config.tuio_port[i]);
    //OscSender* sender = new TcpSender(config.tuio_port[i]); break;

    server = new TuioServer(sender);
    server->setSourceName(s.tuio_source);
    server->setInversion(s.invert_x, s.invert_y, s.invert_a);

    interface = new LogcatInterface();

    thresholder = new FrameThresholder(s.gradient_gate, s.tile_size, s.thread_count);
    thresholder->addUserInterface(interface);
    bool ok = thresholder->init(w,h,FMT,FMT);

    if (!ok) {
        return JNI_FALSE;
        // TODO: cleanup!
    }

    fiducialfinder = new FidtrackFinder(server, &s);
    fiducialfinder->addUserInterface(interface);
    ok = fiducialfinder->init(w,h,FMT,FMT);

    if (!ok) {
        return JNI_FALSE;
        // TODO: cleanup!
    }

    // TODO: calibrator = new CalibrationEngine(config.grid_config);

    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_mkalten_reactivision_VisionEngine_process(JNIEnv *env, jobject thiz, jobject jInput) {

    auto src =  (unsigned char*) env->GetDirectBufferAddress(jInput);
    long size = (long) env->GetDirectBufferCapacity(jInput);

    if (size != frameSize) abort();

    thresholder->process(src, result);
    fiducialfinder->process(src, result);

    memcpy(src, result, size);
}