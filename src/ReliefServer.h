#include "ofMain.h"
#include "ofxOsc.h"
#include "ReliefIOManager.h"
#include "constants.h"

#define LISTEN_PORT 78746 //Port to listen for clients on

class gesturalReliefApp : public ofBaseApp{

	public:
        ~gesturalReliefApp();
		typedef vector<vector< unsigned char> > frame;
		
    
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

		// Relief
		ReliefIOManager * mIOManager;
		unsigned char mPinHeightFromRelief [RELIEF_SIZE_X][RELIEF_SIZE_Y];
		float mPinHeightToRelief [RELIEF_SIZE_X][RELIEF_SIZE_Y];
		unsigned char mPinMask[RELIEF_SIZE_X][RELIEF_SIZE_Y];

        float loadTarget[RELIEF_SIZE_X][RELIEF_SIZE_Y];
		
		void updateFromReliefHeight();
		void reliefvtoa(frame, unsigned char arr[RELIEF_SIZE_X][RELIEF_SIZE_Y]);
		frame reliefatov(unsigned char arr[RELIEF_SIZE_X][RELIEF_SIZE_Y]);
		
        void startLoading();
		void processLoading();
	
		bool loading;
		int adjust_frame;
		int frames_loading;   
        float startTime;
        float loadSpeed;
	
		bool constantUpdate; //controls if the server constantly streams the relief state
    
        //Networking    
        struct Client {
            ofxOscSender   sender;
            string         ip;
            int            out_port;
            float          lastPing;
            
            Client(string out_ip, int outport) {
                out_port = outport;                
                sender.setup(out_ip,out_port);
                ip = out_ip;
                lastPing = ofGetElapsedTimef();
            }
        };
        vector<Client*> clients;
        ofxOscReceiver receiver;
        void addClient(string ip, int port);
        void processMessages();
        void updateClientsFromHeight();
        void checkDisconnects();
};

