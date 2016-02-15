#pragma once

//#include "config.h"
#include "ofMain.h"
#include "ofxOsc.h"
#include "ofThread.h"
#include "ofUtils.h"
#include "edflib.h"

//#include "fftw3.h"
#include <Accelerate/Accelerate.h>

#define X_SIZE 128
#define PEAK_AMP 200

//#define PATHTOEDF "/Users/laboratormultimedia/Desktop/Physiomimi Work/iv-1-23.06.2014.17.27.57.edf"
#define PATHTOEDF "/Users/laboratormultimedia/Desktop/edf files/reparat.edf"

#include "edk.h"
#include "edkErrorCode.h"
#include "EmoStateDLL.h"

#pragma comment(lib, "lib/edk.lib")

class ofApp;

class clockThread : public ofThread {
    
public:
    
    int sampleIdx;
    ofxOscSender sender;
    float starttime;
    short framescale;  
    
    // Methods
    void start(ofApp* p);
    void stop();
    void threadedFunction();
    
    
private:
    
    // Parent application
    ofApp* parent;
    
};


class ofApp : public ofBaseApp{
    
public:

    clockThread clock;
    eDataHandle hData;
    
    EE_DataChannel_t targetChannelList[22] = {
        ED_COUNTER,
        ED_AF3, ED_F7, ED_F3, ED_FC5, ED_T7,
        ED_P7, ED_O1, ED_O2, ED_P8, ED_T8,
        ED_FC6, ED_F4, ED_F8, ED_AF4, ED_GYROX, ED_GYROY, ED_TIMESTAMP,
        ED_FUNC_ID, ED_FUNC_VALUE, ED_MARKER, ED_SYNC_SIGNAL
    };
    
    int edfChannelList[22] = {
		0, // as above
        2, 3, 4, 5, 6, // as above
        7, 8, 9, 10, 11, // as above
        12, 13, 14, 15, 33, 34, 1, // no timestamp
        1, 1, 35, 1 // just marker
    };
    
    float engBo, excLT, excST, frust, medit;
    
    EmoEngineEventHandle eEvent;
    EmoStateHandle eState;
    EE_SignalStrength_t signalStr;
    unsigned int userID;
    float secs;
    unsigned int datarate;
    bool readytocollect;
    int option;
    int state;
    
    //OSC
    string hostname;
    int port;
    string osc_file_path;
    
    double data[22][X_SIZE]; // 22 channels, X_SIZE samples
    double averages[22], last[22], maxframe[22]; // 22 channels
    double buf_fft[22][X_SIZE];
    double          *fft_input, *power_spectrum;
    double          *fft_result;
    DOUBLE_COMPLEX_SPLIT split_data;
    //fftw_plan       plan_forward;
    FFTSetupD fftSetup;
    double *window;
    float scale;
    
    double delta[22], theta[22], alpha[22], beta[22], total[22];

    unsigned int nSamplesTaken;
    unsigned int nBuffersTaken;
    
    struct edf_hdr_struct hdr;
    string edf_file_path;// = PATHTOEDF;
    string errortext;
    void setupEDF();
    
    void setup();
    void update();
    void draw();
    
    void updatebuffer();
    void updateAff();
    void processFFT(int sensor);
    void sendOSC();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    ~ofApp();
    
};



