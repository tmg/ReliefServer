#include "ReliefServer.h"

//--------------------------------------------------------------
gesturalReliefApp::~gesturalReliefApp() {
    for(int i = 0 ; i < clients.size();i++) {
        delete clients[i];
    }
}
void gesturalReliefApp::setup(){
	
	/*****************
	 * General setup *
	 *****************/
		
	ofBackground(0, 0, 0);
	ofSetFrameRate(FPS);
	ofSetWindowTitle("Relief Server");
	/****************
	 * Relief setup *
	 ****************/
	
	// Relief mask
	unsigned char mPinMaskDummy[RELIEF_SIZE_X][RELIEF_SIZE_Y] = {
		{0,0,0,1,1,1,1,1,1,0,0,0},
		{0,0,1,1,1,1,1,1,1,1,0,0},
		{0,1,1,1,1,1,1,1,1,1,1,0},
		{1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1,1,1,1,1},
		{0,1,1,1,1,1,1,1,1,1,1,0},
		{0,0,1,1,1,1,1,1,1,1,0,0},
		{0,0,0,1,1,1,1,1,1,0,0,0}
	};
	memcpy(mPinMask, mPinMaskDummy, RELIEF_SIZE_X * RELIEF_SIZE_Y);
	// Initialize communication with Relief table
	if(RELIEF_CONNECTED)
		mIOManager = new ReliefIOManager();
	
	// Reset height pin arrays
	updateFromReliefHeight();

	for (int x = 0; x < RELIEF_SIZE_X; x++) {
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			mPinHeightToRelief[x][y] = 100;
			mPinHeightToRelief[x][y] = 50; //use this starting value for a safe reset
			loadTarget[x][y] = mPinHeightToRelief[x][y];
		}
	}
	startLoading();
    startTime = ofGetElapsedTimef();
    //Network setup
    
    constantUpdate = false;  
    receiver.setup(LISTEN_PORT);
}

//--------------------------------------------------------------
void gesturalReliefApp::update(){

		

	updateFromReliefHeight();	
   
    if(loading) {
        processLoading();
    } else {
        for (int x = 0; x < RELIEF_SIZE_X; x++) {
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                loadTarget[x][y] = mPinHeightToRelief[x][y];
            }
        }
    }
    
    //Safe Loading
    if(ofGetElapsedTimef() - startTime < 3) {
        for (int x = 0; x < RELIEF_SIZE_X; x++) {
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                mPinHeightToRelief[x][y] = ofMap(ofGetElapsedTimef() - startTime, 1, 3, 25, RELIEF_FLOOR,1);
                loadTarget[x][y] = mPinHeightToRelief[x][y];
            }
        }
    }
    
    if(RELIEF_CONNECTED){
        mIOManager->sendPinHeightToRelief(mPinHeightToRelief);
    }
    
    processMessages();
    checkDisconnects();
    if (constantUpdate) {
        updateClientsFromHeight();
    }
}

//--------------------------------------------------------------
void gesturalReliefApp::draw() {
	char reportStr[1024];
	int text_position = 0;
	
    if (!loading) {
        text_position += 20;
        sprintf(reportStr,"FPS: %.2f",ofGetFrameRate());
        ofDrawBitmapString(reportStr, 20,text_position);
		
	} else {
        text_position += 20;
		sprintf(reportStr,"FPS: %.2f Loading Relief...", ofGetFrameRate());
		ofDrawBitmapString(reportStr, 20, text_position);
    }
    text_position += 20;
    sprintf(reportStr,"Clients Connected: %d",(int)clients.size());
    ofDrawBitmapString(reportStr, 20, text_position);
	for(int i = 0; i< clients.size(); i++) {
        text_position += 20;
        sprintf(reportStr," - Sending to %s:%d",clients[i]->ip.c_str(),clients[i]->out_port);
        ofDrawBitmapString(reportStr, 20, text_position);
    }
}
//--------------------------------------------------------------
void gesturalReliefApp::updateFromReliefHeight() {
	mIOManager->getPinHeightFromRelief(mPinHeightFromRelief);
	if(!loading || adjust_frame != 0){ //allow manipulation if not loading or in adjust phase
		for (int x = 0; x < RELIEF_SIZE_X; x++) {
			for (int y = 0; y < RELIEF_SIZE_Y; y++) {
				//disable server side manual manipulation
                mPinHeightToRelief[x][y] += (mPinHeightFromRelief[x][y] - mPinHeightToRelief[x][y]) / DIRECT_MANIPULATION_DELAY;
			}
		}
	}
}
//--------------------------------------------------------------
void gesturalReliefApp::keyPressed(int key) {
	switch (key) {
        case 'r':
            for(int x = 0; x < RELIEF_SIZE_X; x++) {
                for(int y = 0; y < RELIEF_SIZE_Y;y++) {
                    loadTarget[x][y] = RELIEF_FLOOR;
                }
            }
            printf("Reset pins to floor\n");
            startLoading();
            break; 
	}
}

void gesturalReliefApp::reliefvtoa(vector<vector<unsigned char> > vec, unsigned char arr[RELIEF_SIZE_X][RELIEF_SIZE_Y]){
	for (int x = 0; x < RELIEF_SIZE_X; x++) {
		for(int y = 0;y < RELIEF_SIZE_Y; y++) {
			arr[x][y] = vec[x][y];
		}
	}
}

vector<vector<unsigned char> > gesturalReliefApp::reliefatov(unsigned char arr[RELIEF_SIZE_X][RELIEF_SIZE_Y]){
	vector<vector<unsigned char> > relief;	
	for (int x = 0; x < RELIEF_SIZE_X; x++) {
		vector<unsigned char> row;
		for(int y = 0;y < RELIEF_SIZE_Y; y++) {
			row.push_back(arr[x][y]);
		}
		relief.push_back(row);
	}
	return relief;
}

void gesturalReliefApp::processLoading() {
	//loading constants	
	int max_speed = 2;
	//int min_speed = 4;
	int diff_threshold = 13;
	int max_frames = FPS*2; // maximum loading frames incase of a pin-misfire
	int adjust_duration = FPS*0.5;
	
	if(adjust_frame == 0) {
		frames_loading++;
		bool continue_loading_flag = 0; //flag to see if any pins are still loading
		//frame saved = instances[current_instance].frames[current_frame]; //the goal state
		for (int x = 0; x < RELIEF_SIZE_X; x++) {
			for (int y = 0; y < RELIEF_SIZE_Y; y++) {
				if(mPinMask[x][y]){ //only consider pins that exist					
					int fdiff = (int)loadTarget[x][y] - (int)mPinHeightFromRelief[x][y];
					int tdiff = (int)loadTarget[x][y] - (int)mPinHeightToRelief[x][y];
					if(abs(fdiff) >= diff_threshold){						
						continue_loading_flag = 1;
					}
					if (fdiff > 0) {
						mPinHeightToRelief[x][y] += min(max_speed,tdiff);
					} else if(fdiff < 0){
						mPinHeightToRelief[x][y] += max(-max_speed,tdiff);
					}
					
				}
			}
		}
		
		
		if(!continue_loading_flag || frames_loading > max_frames) { //conditions for loading finished
			adjust_frame = 1;			
		}
	} else {
		adjust_frame++;
		if (adjust_frame >= adjust_duration) { //conditions to end adjustment
			adjust_frame = 0;
			loading = 0;
		}
	}
}


void gesturalReliefApp::startLoading() {
	loading = 1;
	frames_loading = 0;
	adjust_frame = 0;
}

//ManyMouse

void gesturalReliefApp::addClient(string ip, int port) {
    bool add = true;
    for(int i = 0; i < clients.size(); i++) {
        if(ip == clients[i]->ip) {
            add = false;
        }
    }
    if(add) {
        clients.push_back(new Client(ip,port));
        //Send reply message
        ofxOscMessage m;
        m.setAddress("/relief/connect/reply");
        //m.addIntArg(next_client_in_port);
        int inst = clients.size() -1;
		clients[inst]->sender.sendMessage(m);
        
        printf("Add Client: Sending-%s, %d\n",ip.c_str(),port);
    }
}

void gesturalReliefApp::processMessages() {
    while(receiver.hasWaitingMessages()){
        ofxOscMessage m;
        receiver.getNextMessage(&m);
        if(m.getAddress() == "/relief/connect"){
            string ip = m.getRemoteIp();
            int port = m.getArgAsInt32(0);
            addClient(ip, port);
            updateClientsFromHeight();
        }
        if(m.getAddress() == "/relief/heartbeat") {
            if (m.getNumArgs() == 1) {
                string ip = m.getRemoteIp();
                int port = m.getArgAsInt32(0);
                addClient(ip, port);
                updateClientsFromHeight();
            }
        }
        if(m.getAddress() == "/relief/set") {
            if(m.getNumArgs() != RELIEF_SIZE_X*RELIEF_SIZE_Y) {
                printf("Incorrect number of arguments for /relief/set\n");
            } else {
                for(int x = 0; x < RELIEF_SIZE_X; x++) {
                    for(int y = 0; y < RELIEF_SIZE_Y; y++) {
                        unsigned char val = ofMap(m.getArgAsInt32(x*RELIEF_SIZE_Y+y),0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
                        mPinHeightToRelief[x][y] = val;
                        loadTarget[x][y] = val;
                    }
                }
                startLoading();
            }
        }
        if(m.getAddress() == "/relief/set/pin"){           
            int pinx = m.getArgAsInt32(0);
            int piny = m.getArgAsInt32(1);
            int pinheight = m.getArgAsInt32(2);
            loadTarget[pinx][piny] = ofMap(pinheight,0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
            mPinHeightToRelief[pinx][piny] = ofMap(pinheight,0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
            startLoading();
        }        
        if(m.getAddress() == "/relief/load") {
            if(m.getNumArgs() != RELIEF_SIZE_X*RELIEF_SIZE_Y) {
                printf("Incorrect number of arguments for /relief/set\n");
            } else {
                for(int x = 0; x < RELIEF_SIZE_X; x++) {
                    for(int y = 0; y < RELIEF_SIZE_Y; y++) {
                        loadTarget[x][y] = ofMap(m.getArgAsInt32(x*RELIEF_SIZE_Y+y),0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
                    }
                }
                startLoading();
            }
        }
        if(m.getAddress() == "/relief/disconnect") {
            for(int i = 0; i < clients.size();i++) {
                if(clients[i]->ip == m.getRemoteIp()) {
                    clients.erase(clients.begin()+i);
                    i--;
                }
            }
        }
        for(int i = 0; i < clients.size(); i++) {
            if(clients[i]->ip == m.getRemoteIp()) {
                clients[i]->lastPing = ofGetElapsedTimef();
            }
        }
    }

}

void gesturalReliefApp::checkDisconnects() {
    float time = ofGetElapsedTimef();
    for(int i = 0; i < clients.size(); i++) {
        if(time > clients[i]->lastPing +3.f) {
            clients.erase(clients.begin()+i);
            i--;
        }
    }
}


void gesturalReliefApp::updateClientsFromHeight() {
    for(int i = 0; i < clients.size(); i++) {
        ofxOscMessage m;
        m.setAddress("/relief/update");
        for (int x = 0; x < RELIEF_SIZE_X; x++) { 
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                m.addIntArg(ofMap(mPinHeightFromRelief[x][y],RELIEF_FLOOR,RELIEF_CEIL,0,100,1));
            }
        }
        clients[i]->sender.sendMessage(m);
    }
}

//--------------------------------------------------------------
void gesturalReliefApp::keyReleased(int key){
}	

//--------------------------------------------------------------
void gesturalReliefApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void gesturalReliefApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void gesturalReliefApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void gesturalReliefApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void gesturalReliefApp::windowResized(int w, int h){

}

