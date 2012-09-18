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
    
    constantUpdate = true;  
    receiver.setup(LISTEN_PORT);
    
    loadSpeed = 3.f; //Default speed for the loading process
}

//--------------------------------------------------------------
void gesturalReliefApp::update(){
	updateFromReliefHeight(); //Updates the height from the table and adjusts for direct manipulation
   
    if(loading) {
        processLoading();
    } else {
        //If not loading, have the target state follow the direct manipulation
        for (int x = 0; x < RELIEF_SIZE_X; x++) {
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                loadTarget[x][y] = mPinHeightToRelief[x][y];
            }
        }
    }
    
    //This resets the pins to the floor over time so they don't slam down
    if(ofGetElapsedTimef() - startTime < 3) {
        for (int x = 0; x < RELIEF_SIZE_X; x++) {
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                mPinHeightToRelief[x][y] = ofMap(ofGetElapsedTimef() - startTime, 1, 3, 50, RELIEF_FLOOR,1);
                loadTarget[x][y] = mPinHeightToRelief[x][y];
            }
        }
    }
    
    //send the height to the relief
    if(RELIEF_CONNECTED){
        unsigned char relief[RELIEF_SIZE_X][RELIEF_SIZE_Y];
        for (int x = 0; x < RELIEF_SIZE_X; x++) {
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                relief[x][y] = (unsigned char)mPinHeightToRelief[x][y];
            }
        }
        mIOManager->sendPinHeightToRelief(relief);
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
				//Adjust to allow direct manipulation
                mPinHeightToRelief[x][y] += (mPinHeightFromRelief[x][y] - (unsigned char)mPinHeightToRelief[x][y]) / DIRECT_MANIPULATION_DELAY;
			}
		}
	}
}
//--------------------------------------------------------------
void gesturalReliefApp::keyPressed(int key) {
	switch (key) {
        //resets the pins to the floor
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
	int diff_threshold = 13; //The threshold for considering a pin loaded
	int max_frames = (100/loadSpeed)+FPS; //The maximum number of loading frames, incase some pins are un-loadable
	int adjust_duration = FPS*0.5; //Duration for the direct manipulation 
	
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
						mPinHeightToRelief[x][y] += min(loadSpeed,(float)tdiff);
					} else if(fdiff < 0){
						mPinHeightToRelief[x][y] += max(-loadSpeed,(float)tdiff);
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

//Adds new clients
void gesturalReliefApp::addClient(string ip, int port) {
        clients.push_back(new Client(ip,port));
        //Send reply message
        ofxOscMessage m;
        m.setAddress("/relief/connect/reply");
        int inst = clients.size() -1;
		clients[inst]->sender.sendMessage(m);        
        updateClientsFromHeight(); //Send all clients a height update
        printf("Add Client: %s:%d\n",ip.c_str(),port);
}

//Process received messages
void gesturalReliefApp::processMessages() {
    while(receiver.hasWaitingMessages()){
        ofxOscMessage m;
        receiver.getNextMessage(&m);
        cout << "message received: " << m.getAddress() << " from " << m.getRemoteIp() << "\n";
        
        //Connect new clients from the heartbeat message
        if(m.getAddress() == "/relief/heartbeat") {
            string ip = m.getRemoteIp();
            bool add = true;
            for(int i = 0; i < clients.size(); i++) {
                if(ip == clients[i]->ip) {
                    add = false;
                }
            }
            if(add) {
                int port = LISTEN_PORT;
                if (m.getNumArgs() == 1) {
                    port = m.getArgAsInt32(0);
                } else {
                    cout << "Using default port for client." << endl;
                }
                addClient(ip, port);
            }
        }

        //Repeat broadcast messages to all other clients
        string::size_type pos = m.getAddress().find("/relief/broadcast", 0);
        if (pos == 0) {
            for (int i = 0; i < clients.size(); i++) {
                if (clients[i]->ip != m.getRemoteIp()) {
                    clients[i]->sender.sendMessage(m);
                }
            }
        }
        
        //Sets the relief immediately
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
        
        //Sets one pin immediately
        if(m.getAddress() == "/relief/set/pin"){           
            int pinx = m.getArgAsInt32(0);
            int piny = m.getArgAsInt32(1);
            int pinheight = m.getArgAsInt32(2);
            loadTarget[pinx][piny] = ofMap(pinheight,0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
            mPinHeightToRelief[pinx][piny] = ofMap(pinheight,0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
            startLoading();
        }
        
        //Loads an entire relief at a set speed
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
        
        //Removes the sender as a client
        if(m.getAddress() == "/relief/disconnect") {
            for(int i = 0; i < clients.size();i++) {
                if(clients[i]->ip == m.getRemoteIp()) {
                    clients.erase(clients.begin()+i);
                    i--;
                }
            }
        }
        
        //Sets the loading speed
        if(m.getAddress() == "/relief/set/speed") {
            loadSpeed = m.getArgAsFloat(0);
        }
        
        //Updats the heartbeat timer
        for(int i = 0; i < clients.size(); i++) {
            if(clients[i]->ip == m.getRemoteIp()) {
                clients[i]->lastPing = ofGetElapsedTimef();
            }
        }
    }

}

//Checks for clients which haven't sent a heartbeat recently enough
void gesturalReliefApp::checkDisconnects() {
    float time = ofGetElapsedTimef();
    for(int i = 0; i < clients.size(); i++) {
        if(time > clients[i]->lastPing +3.f) {
            clients.erase(clients.begin()+i);
            i--;
        }
    }
}

//Sends the from relief height to all clients
void gesturalReliefApp::updateClientsFromHeight() {
    for(int i = 0; i < clients.size(); i++) {
        ofxOscMessage m;
        m.setAddress("/relief/update");
        for (int x = 0; x < RELIEF_SIZE_X; x++) { 
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                m.addIntArg((unsigned char)ofMap(mPinHeightFromRelief[x][y],RELIEF_FLOOR,RELIEF_CEIL,0,100,1));
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

