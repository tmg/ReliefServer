#include "ReliefServer.h"

//--------------------------------------------------------------
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
	generateMeshMask();
	// Initialize communication with Relief table
	if(RELIEF_CONNECTED)
		mIOManager = new ReliefIOManager();
	
	// Reset height pin arrays
	updateFromReliefHeight();
		
	current_frame = 0;
	editing = 1;
	recording = 0;
	current_instance = 0;
	for (int x = 0; x < RELIEF_SIZE_X; x++) {
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			mPinHeightToRelief[x][y] = 100;
			//mPinHeightToRelief[x][y] = 50; //use this starting value for a safe reset			
		}
	}
	pushInstance();
	buildshape();
	startLoading();
    //Network setup
    
    //receive.setup(PORT);
    next_client_in_port = ADD_CLIENT_PORT+20;
    client_receiver.setup(ADD_CLIENT_PORT);    
}

//--------------------------------------------------------------
void gesturalReliefApp::update(){

		
	/*****************
	 * State machine *
	 *****************/
	updateFromReliefHeight();	
		
	
	/*****************
	 * Relief update *
	 *****************/
	if(recording == 1) { // if recording, record the frame
		pushFrame(current_instance);
	}	
	if(animating != 0) {
		if (current_frame == instances[current_instance].frames.size()-1) { //animating finished
			animating = 0;
			startLoading();
			current_frame = 0;
		}
		changeFrame(animating);
	}
	if(loading) {
		processLoading();
	} else if(editing == 1 && current_frame == 0) {
		//updateCurrentMesh();		
		instances[current_instance].frames[current_frame] = reliefatov(mPinHeightToRelief);
		updateMesh(current_instance);
	}
	
	
	float sensativity = 0.15;
	int frame_buffer = 15;
	float average = 0;
	int num = 0;
	for(int x = 0; x < RELIEF_SIZE_X; x++) {
		for(int y = 0; y < RELIEF_SIZE_Y; y++) {
			if (mPinMask[x][y]) {
				num++;
				average+=abs(mPinHeightFromRelief[x][y]-previousHeightFromRelief[x][y]);
				previousHeightFromRelief[x][y] = mPinHeightFromRelief[x][y];
			}
		}
	}	
	average /= num;
	if(average < sensativity) {
		if(stable_frames > frame_buffer) {
			recording = 0;//David's automatic record stop
		}
		stable_frames++;
	} else {
		stable_frames = 0;		
	}	
	
	
	if(RELIEF_CONNECTED){
		mIOManager->sendPinHeightToRelief(mPinHeightToRelief);
	}
    
    receiveClients();
    processMessages();
    checkDisconnects();
    
}

//--------------------------------------------------------------
void gesturalReliefApp::draw(){
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
        sprintf(reportStr," - Receiving on port %d / Sending to %s:%d",clients[i].in_port,clients[i].ip.c_str(),clients[i].out_port);
        ofDrawBitmapString(reportStr, 20, text_position);
    }
}
//--------------------------------------------------------------
void gesturalReliefApp::updateFromReliefHeight() {
	mIOManager->getPinHeightFromRelief(mPinHeightFromRelief);
	if(!loading || adjust_frame != 0){ //allow manipulation if not loading or in adjust phase
		for (int x = 0; x < RELIEF_SIZE_X; x++) {
			for (int y = 0; y < RELIEF_SIZE_Y; y++) {
				mPinHeightToRelief[x][y] += (mPinHeightFromRelief[x][y] - mPinHeightToRelief[x][y]) / DIRECT_MANIPULATION_DELAY;
			}
		}
	}
}
//--------------------------------------------------------------
void gesturalReliefApp::keyPressed(int key){
	switch (key) {
		case '=':
			loading = 0;
			break;
		case 357: //up arrow
			pushInstance();
			break;
		case 356: //left arrow
			changeInstance(-1);
			break;
		case 359: //down arrow
			//popInstance();
			break;
		case 358: //right arrow
			changeInstance(1);
			break;
		case '0':
			//interpolateInstances(20);
			break;
		case '9':
			animate(1);
			break;
		case '8':
			animate(-1);
			break;
		case '7':
			buildshape();
			break;		
		case '+':
			if(editing){
				editing = 0;
			} else {
				editing = 1;
			}
			break;
		case '3':
			resetAnimation();
			break;
		case 'c':
			//resetInstances();
			//break;
		case 'i':
			//changeInstance(1);
			break;
		case 'p':
			break;
		default:
			break;
	}
}

void gesturalReliefApp::buildshape() {
	//sphere radius 100
	float radius = 100;
	unsigned char relief[RELIEF_SIZE_X][RELIEF_SIZE_Y];
	for (int x = 0; x < RELIEF_SIZE_X; x++) { 
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			if(mPinMask[x][y] == 1) { 
				float xpos = ofMap(x,0,RELIEF_SIZE_X-1,-radius,radius);
				float ypos = ofMap(y,0,RELIEF_SIZE_X-1,-radius,radius);
				double zpos = sqrt(pow(radius,2) - pow(ypos,2) - pow(xpos, 2));
				if(zpos > 0) {
					relief[x][y] = (unsigned char)(100-zpos);
				} else {
					relief[x][y] = 100;
				}
			}
		}
	}
	//cout << "here" << endl;
	pushInstance();
	changeInstance(1);
	instances[current_instance].frames[0] = reliefatov(relief);
	updateMesh(current_instance);
	
	//square
	for (int x = 0; x < RELIEF_SIZE_X; x++) { 
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			if(mPinMask[x][y] == 1) {
				if (x>2 && x < 9 && y >2 && y < 9) {
					relief[x][y] = 0;
				} else {
					relief[x][y] = 100;
				}
			}
		}
	}
	pushInstance();	
	changeInstance(1);
	instances[current_instance].frames[0] = reliefatov(relief);
	updateMesh(current_instance);
	
	//cone center 6,6

	for (int x = 0; x < RELIEF_SIZE_X; x++) { 
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			if(mPinMask[x][y] == 1) {
				float xpos = ofMap(x,0,RELIEF_SIZE_X-1,-radius,radius);
				float ypos = ofMap(y,0,RELIEF_SIZE_X-1,-radius,radius);
				double zpos = 1.5*sqrt(pow(ypos,2) + pow(xpos, 2));
				if(zpos > 100){
					zpos = 100;
				}
				relief[x][y] = zpos;
			}
		}
	}
	pushInstance();	
	changeInstance(1);
	instances[current_instance].frames[0] = reliefatov(relief);
	updateMesh(current_instance);
	
	for (int x = 0; x < RELIEF_SIZE_X; x++) { 
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			if(mPinMask[x][y] == 1) {
				if (x == 6) {
					relief[x][y] = 10*(abs(y-7));
				} else {
					relief[x][y] = 100;
				}

				
			}
		}
	}
	pushInstance();	
	changeInstance(1);
	instances[current_instance].frames[0] = reliefatov(relief);
	updateMesh(current_instance);
	
	changeInstance(1);
	startLoading();
	//visualizationOffset = 0;
}

void gesturalReliefApp::animate(int dir){
	animating = dir;
}

void gesturalReliefApp::pushFrame(int inst){
	frame relief = reliefatov(mPinHeightToRelief);
	for (int x = 0; x < RELIEF_SIZE_X; x++) { 
		for (int y = 0; y < RELIEF_SIZE_Y; y++) {
			if(relief[x][y] > RELIEF_FLOOR) { //Set the floor for the motor
				relief[x][y] = RELIEF_FLOOR;
			}
		}
	}
	if(current_frame >= (int)(instances[inst].frames.size())-1) {
		instances[inst].frames.push_back(relief);
		current_frame = instances[inst].frames.size()-1;
	} else {
		current_frame++;
		instances[inst].frames[current_frame] = relief;
	}
	
}

void gesturalReliefApp::pushInstance() {
	instance n;
	instances.push_back(n);
	pushFrame(instances.size()-1);
	pushMesh();
}

void gesturalReliefApp::pushMesh() {
	meshes.push_back(*(new mesh));
	updateMesh(meshes.size()-1);
}

void gesturalReliefApp::changeFrame(int dist){
	startLoading();
	current_frame += dist;
	current_frame = (current_frame+instances[current_instance].frames.size())%instances[current_instance].frames.size();
}

void gesturalReliefApp::changeInstance(int dist){
	startLoading();
	int prev = current_instance;
	current_instance += dist;
	current_instance = (current_instance+instances.size())%instances.size();
	current_frame=0;
	updateMesh(prev);
}

void gesturalReliefApp::resetInstances() {
	instances.clear();
	pushInstance();
}

void gesturalReliefApp::resetInstance(int inst) {
	instances[inst].frames.clear();
	instances[inst].cursorRect.clear();
	instances[inst].manipulationOn.clear();
	current_frame = 0;
	pushFrame(inst);
	startLoading();
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

void gesturalReliefApp::updateCurrentMesh() {	
	current_mesh = generateMesh(reliefatov(mPinHeightToRelief));
}

void gesturalReliefApp::updateMesh(int index) {
	bool current = (index == current_instance);	
	frame relief;
	if (current) {
		relief = instances[index].frames[current_frame];
	} else {
		relief = instances[index].frames[0];
	}
	meshes[index] = generateMesh(relief);
}

gesturalReliefApp::mesh gesturalReliefApp::generateMesh(frame relief){
	mesh m;
	vector<vector<float> > cols;
	for (int y = -1; y < RELIEF_SIZE_X+1 ; y++) {		
		vector<float> row;
		for(int x = -1;x < RELIEF_SIZE_Y+1; x++) {
			if(x == -1 || y == -1 || x == RELIEF_SIZE_X || y == RELIEF_SIZE_Y) { //border
				row.push_back(0);
			} else if(mPinMask[x][y] == 1) {								
				row.push_back(120-relief[x][y]); //also flipped
			} else {
				row.push_back(0);
			}
		}
		row = splinedouble(row);
		for (int i = 0; i < row.size(); i++) {
			if(i >= cols.size()){
				cols.push_back(*(new vector<float>));
			}
			cols[i].push_back(row[i]);
		}
	}
	for (int i = 0; i < cols.size(); i++) {
		m.vertices.push_back(splinedouble(cols[i]));
	}
	int size = m.vertices.size(); //convenience variable
	if(size > 0){
		float width = (float)FRAME_WIDTH/(size);
		float depth = width;
		
		vector<vector<ofVec3f> > faceNormals;
		for(int x = 0; x < size-1;x++) {
			vector<ofVec3f> col;
			for(int y = 0; y < size-1;y++) {
				float h1 = m.vertices[x][y];
				float h2 = m.vertices[x+1][y];
				float h3 = m.vertices[x+1][y+1];
				ofVec3f norm(depth*(h1-h2),width*depth,width*(h2-h1)); //calculated the normal by hand and simplfied for speed
				norm.normalize();
				col.push_back(-norm); //flip normals
			}
			faceNormals.push_back(col);
		}
		m.normals.clear();
		for(int x = 0; x < size;x++) {
			vector<ofVec3f> col;
			for(int y = 0; y < size;y++) {
				ofVec3f norm;
				if (x > 0) {
					if(y > 0) {
						norm += faceNormals[x-1][y-1];
					}
					if (y <= faceNormals.size()-1) {
						norm += faceNormals[x-1][y];
					}
				}
				if(x < faceNormals.size()) {
					if (y > 0) {
						norm += faceNormals[x][y-1];
					}
					if (y < faceNormals.size()) {
						norm += faceNormals[x][y];
					}					
				}
				norm.normalize();
				col.push_back(norm);
			}
			m.normals.push_back(col);
		}
	}
	return m;
}

void gesturalReliefApp::processLoading() {
	//loading constants	
	int max_speed = 4;
	//int min_speed = 4;
	int diff_threshold = 13;
	int max_frames = FPS*2; // maximum loading frames incase of a pin-misfire
	int adjust_duration = FPS*0.5;
	
	if(adjust_frame == 0) {
		if(animating != 0) {
			max_speed = 100;
		}
		frames_loading++;
		bool continue_loading_flag = 0; //flag to see if any pins are still loading
		frame saved = instances[current_instance].frames[current_frame]; //the goal state
		for (int x = 0; x < RELIEF_SIZE_X; x++) {
			for (int y = 0; y < RELIEF_SIZE_Y; y++) {
				if(mPinMask[x][y]){ //only consider pins that exist					
					int fdiff = (int)saved[x][y] - (int)mPinHeightFromRelief[x][y];
					int tdiff = (int)saved[x][y] - (int)mPinHeightToRelief[x][y];
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
	stable_frames = 0;
	adjust_frame = 0;
}

void gesturalReliefApp::resetAnimation() {
	animating = 0;
	current_frame = 0;
	startLoading();
}

//ManyMouse


float gesturalReliefApp::spline(vector<float> x,vector<float>y,float desired){
	for(int pos = 1; pos < x.size()-2; pos++){
		//double desired = 0;
		if(desired >= x[pos] && desired <= x[pos+1]){
			float h00,h01,h11,h10,p0,m0,p1,m1;
			float tension = 0;
			float t = ofMap(desired, x[pos], x[pos+1], 0, 1);
			//float dist = y[pos+1] - y[pos];
			h00 = 2*pow(t,3) - 3*pow(t,2) + 1;
			h10 = pow(t,3) - 2*pow(t,2) + t;
			h01 = -2*pow(t,3) + 3*pow(t,2);
			h11 = pow(t, 3) - pow(t, 2);
			p0 = y[pos];
			p1 = y[pos+1];
			m0 = (1-tension)*(y[pos+1]-y[pos-1])/(x[pos+1]-x[pos-1]);
			m1 = (1-tension)*(y[pos+2]-y[pos])/(x[pos+2]-x[pos]);
			//printf("%f.2 %f.2 %f.2 %f.2\n",p0,m0,p1,m1);
			float val = h00*p0 + h10*m0 + h01*p1 + h11*m1;
			return val;
		}
	}
}

template <class T>
vector<T> gesturalReliefApp::splinedouble(vector<T> y){
	vector<T> output;
	y.insert(y.begin(),y[0]-y[1]);
	y.push_back(y[y.size()-1]-y[y.size()-2]);
	int steps = 3;
	for(int pos = 1; pos < y.size()-2; pos++){
		for(int i = 0; i < steps; i++) { 
			float h00,h01,h11,h10,p0,m0,p1,m1;
			float tension = 0.5;
			float t = (float)i/steps;
			//float t = ofMap(desired, x[pos], x[pos+1], 0, 1);
			//float dist = y[pos+1] - y[pos];
			h00 = 2*pow(t,3) - 3*pow(t,2) + 1;
			h10 = pow(t,3) - 2*pow(t,2) + t;
			h01 = -2*pow(t,3) + 3*pow(t,2);
			h11 = pow(t, 3) - pow(t, 2);
			p0 = y[pos];
			p1 = y[pos+1];
			//m0 = (1-tension)*(y[pos+1]-y[pos-1])/(x[pos+1]-x[pos-1]);
			//m1 = (1-tension)*(y[pos+2]-y[pos])/(x[pos+2]-x[pos]);
			m0 = (1-tension)*(y[pos+1]-y[pos-1])/(2);
			m1 = (1-tension)*(y[pos+2]-y[pos])/(2);
			//printf("%.2f %.2f %.2f %.2f\n",p0,m0,p1,m1);
			T val = h00*p0 + h10*m0 + h01*p1 + h11*m1;
			//output.push_back(p0);
			output.push_back(val);
		}
	}
	output.push_back(y[(int)y.size()-2]);
	return output;
}

void gesturalReliefApp::generateMeshMask() {	
	meshMask.clear();
	vector<vector<float> > cols;
	frame relief = reliefatov(mPinMask);
	//frame relief = instances[current_instance].frames[0];
	for (int y = -1; y < RELIEF_SIZE_X+1 ; y++) {		
		vector<float> row;
		for(int x = -1;x < RELIEF_SIZE_Y+1; x++) {
			if(x == -1 || y == -1 || x == RELIEF_SIZE_X || y == RELIEF_SIZE_Y) { //border
				row.push_back(0);
			} else {
				row.push_back(relief[x][y]);
			}
		}
		row = splinedouble(row);
		for (int i = 0; i < row.size(); i++) {
			if(i >= cols.size()){
				cols.push_back(*(new vector<float>));
			}
			cols[i].push_back(row[i]);
		}
	}
	for (int i = 0; i < cols.size(); i++) {
		meshMask.push_back(splinedouble(cols[i]));
	}
	for(int x = 0; x < meshMask.size(); x++) {
		for(int y = 0; y < meshMask[x].size(); y++) {
			meshMask[x][y] = (meshMask[x][y]-0.5)*2;
			if (meshMask[x][y] > 1) {
				meshMask[x][y] = 1;
			} else if(meshMask[x][y] < 0) {
				meshMask[x][y] = 0;
			}
		}
	}
}

void gesturalReliefApp::addClient(string ip, int port) {
    bool add = true;
    for(int i = 0; i < clients.size(); i++) {
        if(ip.compare(clients[i].ip) == 0) {
            add = false;
        }
    }
    if(add) {
        Client client;
        clients.push_back(client);
        int inst = (int)clients.size()-1;
        clients[inst].setup(next_client_in_port,ip,port);
        
        //Send reply message
        ofxOscMessage m;
        m.setAddress("/relief/connect/reply");
        m.addIntArg(next_client_in_port);
		clients[inst].sender.sendMessage(m);
        
        printf("Client: receiving-%d sending-%s, %d\n",next_client_in_port,ip.c_str(),port);
        next_client_in_port++;
    } else {
        printf("Attempted connection from %s, but they are already connected.\n",ip.c_str());
    }
}

void gesturalReliefApp::receiveClients() {
    while(client_receiver.hasWaitingMessages()){
        ofxOscMessage m;
        client_receiver.getNextMessage(&m);        
        // check for mouse moved message
        if(m.getAddress() == "/relief/connect"){
            string ip = m.getRemoteIp();
            int port = m.getArgAsInt32(0);
            addClient(ip, port);
            updateClients();
        }
    }
}

void gesturalReliefApp::processMessages() {
    for(int i = 0; i < clients.size(); i++) {
        ofxOscReceiver &receiver = clients[i].receiver;
        while(receiver.hasWaitingMessages()){
            ofxOscMessage m;
            receiver.getNextMessage(&m);      
            if(m.getAddress() == "/relief/set/pin"){           
                int pinx = m.getArgAsInt32(0);
                int piny = m.getArgAsInt32(1);
                int pinheight = m.getArgAsInt32(2);
                instances[current_instance].frames[current_frame][pinx][piny] = ofMap(pinheight,0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
                startLoading();
                printf("%d\n",pinheight);
            }
            if(m.getAddress() == "/relief/set") {
                if(m.getNumArgs() != RELIEF_SIZE_X*RELIEF_SIZE_Y) {
                    printf("Incorrect number of arguments for /relief/set\n");
                } else {
                    unsigned char relief[RELIEF_SIZE_X][RELIEF_SIZE_Y];
                    for(int x = 0; x < RELIEF_SIZE_X; x++) {
                        for(int y = 0; y < RELIEF_SIZE_Y; y++) {
                            relief[x][y] = ofMap(m.getArgAsInt32(x*RELIEF_SIZE_Y+y),0,100,RELIEF_FLOOR,RELIEF_CEIL,1);
                        }
                    }
                    resetInstance(current_instance);
                    instances[current_instance].frames[current_frame] = reliefatov(relief);
                    startLoading();
                }
            }
            if(m.getAddress() == "/relief/disconnect") {
                clients.erase(clients.begin()+i);
                i--;
            }
            
            //Update ping time because a mesage was received
            clients[i].lastPing = ofGetElapsedTimef();
        }
    }
}

void gesturalReliefApp::checkDisconnects() {
    float time = ofGetElapsedTimef();
    for(int i = 0; i < clients.size(); i++) {
        if(time > clients[i].lastPing +3.f) {
            clients.erase(clients.begin()+i);
            i--;
        }
    }
}

void gesturalReliefApp::updateClients() {
    for(int i = 0; i < clients.size(); i++) {
        ofxOscMessage m;
        m.setAddress("/relief/update");
        frame &relief = instances[current_instance].frames[current_frame];
        for (int x = 0; x < RELIEF_SIZE_X; x++) { 
            for (int y = 0; y < RELIEF_SIZE_Y; y++) {
                m.addIntArg(relief[x][y]);
            }
        }
        clients[i].sender.sendMessage(m);
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

