#include "ofMain.h"
#include "ofxOsc.h"
#include "ReliefIOManager.h"
#include "constants.h"

#define ADD_CLIENT_PORT 78746

class gesturalReliefApp : public ofBaseApp{

	public:
	
		typedef vector<vector< unsigned char> > frame;
		struct instance {
			vector<ofRectangle> cursorRect;
			vector<bool> manipulationOn;
			vector<frame> frames;
		};		
		
		struct mesh{
			vector<vector<ofVec3f> > normals;
			vector<vector<float> > vertices;
		};
    
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
		unsigned char mPinHeightToRelief [RELIEF_SIZE_X][RELIEF_SIZE_Y];
		unsigned char mPinMask[RELIEF_SIZE_X][RELIEF_SIZE_Y];
		unsigned char previousHeightToRelief [RELIEF_SIZE_X][RELIEF_SIZE_Y];
		unsigned char previousHeightFromRelief [RELIEF_SIZE_X][RELIEF_SIZE_Y];
		vector<vector<float> > meshMask;
		void generateMeshMask();
	
		vector<instance> instances;
		
		int current_frame;
		int current_instance;
		
		void updateFromReliefHeight();
		void reliefvtoa(frame, unsigned char arr[RELIEF_SIZE_X][RELIEF_SIZE_Y]);
		frame reliefatov(unsigned char arr[RELIEF_SIZE_X][RELIEF_SIZE_Y]);
		
		void pushFrame(int inst);
		void pushInstance();
		void pushMesh();
		void changeFrame(int dist);
		void changeInstance(int dist);
		void animate(int dir);
		void resetInstance(int inst);
		void resetInstances();
		void startLoading();
		void processLoading();
		void resetAnimation();
		
		void updateMesh(int index);
		void updateCurrentMesh();
		mesh generateMesh(frame relief);
		float spline(vector<float> x, vector<float> y, float desired);
	
		template <class T>
		vector<T> splinedouble(vector<T> y);
		vector<mesh> meshes;
	
		bool loading;
		int adjust_frame;
		bool editing;
		int animating;
		int recording;
		int frames_loading;
	
		void buildshape();
    
        int stable_frames;
	
		mesh current_mesh;	
    
        //Networking
        struct Client {
            ofxOscReceiver receiver;
            ofxOscSender   sender;
            string         ip;
            int            in_port;
            int            out_port;
            float          lastPing;
            
            void setup(int inport, string out_ip, int outport) {
                in_port = inport;
                out_port = outport;
                receiver.setup(in_port);
                sender.setup(out_ip,out_port);
                ip = out_ip;
                lastPing = ofGetElapsedTimef();
            }
        };
        ofxOscReceiver test_rec;
        ofxOscSender test_send;
        vector<Client> clients;
        ofxOscReceiver client_receiver;
        int next_client_in_port;
        void receiveClients();
        void addClient(string ip, int port);
        void processMessages();
        void updateClients();
        void checkDisconnects();
        //void disconnectClient(int index);
};

