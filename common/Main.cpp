/*  reacTIVision tangible interaction framework
	Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#ifdef __APPLE__
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif
#ifdef LINUX
#include <signal.h>
#endif

#include "Settings.h"
#include "SDLinterface.h"
#include "VisionEngine.h"

//#include "FrameEqualizer.h"
#include "FrameThresholder.h"
#include "FidtrackFinder.h"
#include "CalibrationEngine.h"

#include "TuioServer.h"

VisionEngine *engine;

static void terminate (int param)
{
	if (engine!=NULL) engine->stop();
}

void printUsage(const char* app_name) {
	std::cout << "usage: " << app_name << " -c [config_file]" << std::endl;
	std::cout << "the default configuration file is " << app_name << ".xml" << std::endl;
	std::cout << "\t -n starts " << app_name << " without GUI" << std::endl;
	std::cout << "\t -l lists all available cameras" << std::endl;
	std::cout << "\t -h shows this help message" << std::endl;
	std::cout << std::endl;
}

int main(int argc, char* argv[]) {

	application_settings config;
	sprintf(config.file,"none");

	const char *app_name = "reacTIVision";
	const char *version_no = "1.6";

	bool headless = false;

	std::cout << app_name << " " << version_no << " (" << __DATE__ << ")" << std::endl << std::endl;

	if (argc>1) {
		if (strcmp( argv[1], "-h" ) == 0 ) {
			printUsage(app_name);
			return 0;
		} else if( strcmp( argv[1], "-c" ) == 0 ) {
			if (argc==3) sprintf(config.file,"%s",argv[2]);
			else {
				printUsage(app_name);
				return 0;
			}
		} else if( strcmp( argv[1], "-n" ) == 0 ) {
			headless = true;
		} else if( strcmp( argv[1], "-l" ) == 0 ) {
			CameraTool::listDevices();
			return 0;
		} else if ( (std::string(argv[1]).find("-NSDocumentRevisionsDebugMode")==0 ) || (std::string(argv[1]).find("-psn_")==0) ){
			// ignore mac specific arguments
		} else {
			printUsage(app_name);
		}
	}

#ifndef WIN32
	signal(SIGINT,terminate);
	signal(SIGHUP,terminate);
	signal(SIGQUIT,terminate);
	signal(SIGTERM,terminate);
#endif

	readSettings(&config);
	config.headless = headless;

	engine = new VisionEngine(app_name,&config);

	if (!headless) {
		UserInterface *uiface = new SDLinterface(app_name,config.fullscreen);
		switch (config.display_mode) {
			case 0: uiface->setDisplayMode(NO_DISPLAY); break;
			case 1: uiface->setDisplayMode(SOURCE_DISPLAY); break;
			case 2: uiface->setDisplayMode(DEST_DISPLAY); break;
		}
		engine->setInterface(uiface);
	}

	TuioServer *server = NULL;
	FrameProcessor *fiducialfinder	= NULL;
	FrameProcessor *thresholder	= NULL;
	FrameProcessor *calibrator	= NULL;

	for (int i=0;i<config.tuio_count;i++) {
		OscSender *sender = NULL;
		try { switch (config.tuio_type[i]) {
			case TUIO_UDP: sender = new UdpSender(config.tuio_host[i].c_str(),config.tuio_port[i]); break;
			case TUIO_TCP_CLIENT: sender = new TcpSender(config.tuio_host[i].c_str(),config.tuio_port[i]); break;
			case TUIO_TCP_HOST: sender = new TcpSender(config.tuio_port[i]); break;
			case TUIO_WEB: sender = new WebSockSender(config.tuio_port[i]); break;
			case TUIO_FLASH: sender = new FlashSender(); break;
			default: continue;
		} } catch (std::exception e) {}

		if (sender) {
			if(i==0) server = new TuioServer(sender);
			else server->addOscSender(sender);
			pv_sleep(1);
		}
	}
	server->setSourceName(config.tuio_source);
	server->setInversion(config.invert_x, config.invert_y, config.invert_a);

	thresholder = new FrameThresholder(config.gradient_gate, config.tile_size, config.thread_count);
	if (config.background) thresholder->toggleFlag(KEY_SPACE,false);
	engine->addFrameProcessor(thresholder);

	fiducialfinder = new FidtrackFinder(server, &config);
	engine->addFrameProcessor(fiducialfinder);

	calibrator = new CalibrationEngine(config.grid_config);
	engine->addFrameProcessor(calibrator);

	engine->start();

	engine->removeFrameProcessor(calibrator);
	delete calibrator;

	config.finger_size = ((FidtrackFinder*)fiducialfinder)->getFingerSize();
	config.finger_sensitivity = ((FidtrackFinder*)fiducialfinder)->getFingerSensitivity();
	config.max_blob_size = ((FidtrackFinder*)fiducialfinder)->getBlobSize();
	config.object_blobs = ((FidtrackFinder*)fiducialfinder)->getFiducialBlob();
	config.cursor_blobs = ((FidtrackFinder*)fiducialfinder)->getFingerBlob();
	config.yamaarashi = ((FidtrackFinder*)fiducialfinder)->getYamaarashi();
	config.yama_flip = ((FidtrackFinder*)fiducialfinder)->getYamaFlip();

	engine->removeFrameProcessor(fiducialfinder);
	delete fiducialfinder;

	config.gradient_gate = ((FrameThresholder*)thresholder)->getGradientGate();
	config.tile_size = ((FrameThresholder*)thresholder)->getTileSize();
	config.background = ((FrameThresholder*)thresholder)->getEqualizerState();
	engine->removeFrameProcessor(thresholder);
	delete thresholder;

	config.invert_x = server->getInvertXpos();
	config.invert_y = server->getInvertYpos();
	config.invert_a = server->getInvertAngle();

	delete engine;
	delete server;

	writeSettings(&config);
	return 0;
}
