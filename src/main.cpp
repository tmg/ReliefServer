#include "ofMain.h"
#include "ReliefServer.h"
#include "ofAppGlutWindow.h"

#include "constants.h"

//========================================================================
int main( ){

    ofAppGlutWindow window;
	ofSetupOpenGL(&window, SCREEN_WIDTH, SCREEN_HEIGHT, OF_WINDOW);
	//ofSetupOpenGL(&window, SCREEN_WIDTH, SCREEN_HEIGHT, OF_FULLSCREEN);
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp( new gesturalReliefApp());

}
