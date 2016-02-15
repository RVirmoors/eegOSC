#include "ofApp.h"


void clockThread::start(ofApp* p) {
    // Get parent app
    parent = p;
    
    // Setup OSC
    sender.setup(parent->hostname, parent->port);
    sampleIdx = 0;
    
    // Start thread -- blocking
    startThread(true);
    
}


void clockThread::stop() {
    // Stop thread
    stopThread();
    
}

void clockThread::threadedFunction() {
    
    // start
    
    while(isThreadRunning()) {
        if(sampleIdx < parent->nSamplesTaken) {
            // if the time is ripe for a new sample to be sent
            if((ofGetElapsedTimef() - starttime > parent->data[17][sampleIdx]) || // headset OR
               (parent->option == 2)) {                                             // .edf
                //lock thread
                if (lock()) {

                    
                    ofxOscBundle b;
                    ofxOscMessage m;
                    
                    // send raw EEG
                    
                    for (int i = 0; i<17; i++) {
                        m.setAddress("/"+ofToString(i));
                        m.addFloatArg(parent->data[i][sampleIdx]);
                        b.addMessage(m);
                        m.clear();
                    }
                    sender.sendBundle(b);
                    
                    // calculate FFT
                    parent->processFFT(1); // AF3
                    parent->processFFT(14); // AF4
                    // send OSC bundle
                    parent->sendOSC();
                    
                    
                    ofSleepMillis((int)(1000.0000f / X_SIZE * framescale));
                    sampleIdx += framescale;
                    unlock();
                }
            }
            else {
                std::cout << "not yet!" << std::endl;
                ofSleepMillis(3);
            }
        }
        else {
            // get new buffer
            parent->updatebuffer();

            
            sampleIdx = 0;
        }
        
        // done
    }
}


//--------------------------------------------------------------
void ofApp::setup(){
    ofBackground(40, 100, 40);
    ofSetFrameRate(X_SIZE);
    
    //fft_input   = (double*) fftw_malloc(sizeof(double) * X_SIZE);
    //fft_result  = (double*) fftw_malloc(sizeof(double) * X_SIZE);
    //power_spectrum = (double*) fftw_malloc(sizeof(double) * (X_SIZE/2 + 1));
    fft_input = new double[X_SIZE];
    fft_result = new double[X_SIZE];
    split_data.imagp = new double[X_SIZE/2];
    split_data.realp = new double[X_SIZE/2];
    power_spectrum = new double[X_SIZE/2];
    window = new double[X_SIZE];
    vDSP_hann_windowD(window, X_SIZE, vDSP_HANN_NORM);
    
    scale = 1.0f / (float)(4.0f * X_SIZE);
    
    //plan_forward  = fftw_plan_r2r_1d(X_SIZE, fft_input, fft_result, FFTW_R2HC, FFTW_ESTIMATE);
    fftSetup = vDSP_create_fftsetupD(log2(X_SIZE), kFFTRadix2);
    
    eEvent			= EE_EmoEngineEventCreate();
    eState				= EE_EmoStateCreate();
    userID					= 0;
    secs							= 1;
    datarate				= 0;
    readytocollect					= false;
    option							= 0;
    state							= 0;
    nSamplesTaken           = 0;
    nBuffersTaken           = 0;
    
    hData = EE_DataCreate();
    
    hostname = "localhost";
    port = 7000;
    clock.framescale = 32; // 2 = 64fps; 4 = 32fps; 8 = 16fps etc.
    
    osc_file_path = "osc.txt";
    ofFile file(osc_file_path);
    if(file.exists()) {
        ofBuffer buffer(file);
        hostname = buffer.getFirstLine();
        port = ofToInt(buffer.getNextLine());
        clock.framescale = ofToInt(buffer.getNextLine());
    }
   
    
    
    for (int i = 0; i < 17; i++) {
        averages[i] = 0;
        last[i] = 0;
    }

}

//--------------------------------------------------------------
void ofApp::update(){

    if (option == 1)
        
        {
            state = EE_EngineGetNextEvent(eEvent);
            EE_Event_t eventType;
            
            if (state == EDK_OK) {
                
                EE_DataSetBufferSizeInSec(secs);
                
                eventType = EE_EmoEngineEventGetType(eEvent);
                EE_EmoEngineEventGetUserId(eEvent, &userID);
                
                signalStr = ES_GetWirelessSignalStatus(eState);
                
                // Log the EmoState if it has been updated
                if (eventType == EE_UserAdded) {
                    std::cout << "User added" << std::endl;
                    EE_DataAcquisitionEnable(userID,true);
                    readytocollect = true;
                }
            }
        }
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofDrawBitmapString("sending osc messages to " + hostname + ":" + ofToString(port) + " -- press 9 to change config", 10, 20);
    ofDrawBitmapString("SELECT off / headset / .edf file : 0 / 1 / 2: "+ofToString(option), 10, 35);
    //ofDrawBitmapString(ofToString(ofGetFrameRate()) + " fps", 800, 20);
    
    if (state != EDK_OK) {
        ofDrawBitmapString("Emotiv Engine State: "+ofToString(state), 10, 50);
    }
    
    if (option == 1)
        ofDrawBitmapString("Live EEG Data from headset:", 10, 80);
    if (option == 2)
        ofDrawBitmapString("EEG Data from file " + edf_file_path, 10, 80);
    if (option) {
        for (int i = 0; i<17; i++)
            ofDrawBitmapString("/" + ofToString(i) +
                               ": "+ ofToString(data[i][clock.sampleIdx]), 10, 95 + i*15);
        
        ofDrawBitmapString("AF3 EEG waves:", 200, 110);
        ofDrawBitmapString("/1/alpha: " + ofToString(alpha[1], 4), 200, 125);
        ofDrawBitmapString("/1/beta: " + ofToString(beta[1], 4), 200, 140);
        ofDrawBitmapString("/1/delta: " + ofToString(delta[1], 4), 200, 155);
        ofDrawBitmapString("/1/theta: " + ofToString(theta[1], 4), 200, 170);
        
        ofDrawBitmapString("AF4 EEG waves:", 200, 185);
        ofDrawBitmapString("/14/alpha: " + ofToString(alpha[14], 4), 200, 200);
        ofDrawBitmapString("/14/beta: " + ofToString(beta[14], 4), 200, 215);
        ofDrawBitmapString("/14/delta: " + ofToString(delta[14], 4), 200, 230);
        ofDrawBitmapString("/14/theta: " + ofToString(theta[14], 4), 200, 245);
 //       ofDrawBitmapString("/total: " + ofToString(total, 4), 200, 155);
        
        ofDrawBitmapString("/1/max: " + ofToString(maxframe[1], 4), 200, 290);
        ofDrawBitmapString("/14/max: " + ofToString(maxframe[14], 4), 200, 305);
        
        
        if(signalStr == 0) {
            ofDrawBitmapString("No Signal", 500, 110);
        }
        else if( signalStr == 1) {
            ofDrawBitmapString("Bad Signal", 500, 110);
        }
        else if( signalStr == 2){
            ofDrawBitmapString("Good Signal", 500, 110);
        }
        
        ofDrawBitmapString("/engBo: " + ofToString(engBo), 500, 125);
        ofDrawBitmapString("/excLT: " + ofToString(excLT), 500, 140);
        ofDrawBitmapString("/excST: " + ofToString(excST), 500, 155);
        ofDrawBitmapString("/frust: " + ofToString(frust), 500, 170);
        ofDrawBitmapString("/medit: " + ofToString(medit), 500, 185);
    }
    
    ofDrawBitmapString(errortext, 10, 500);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    switch (key) {
        case '0':
            option = 0;
            state = 0;
            break;
            
        case '1':
            option = 1;
            state = EE_EngineConnect();
            clock.starttime = ofGetElapsedTimef();
            clock.start(this);
            break;
            
        case '2': {
            option = 2;
            state = 0;
            //Open the Open File Dialog
            ofFileDialogResult openFileResult = ofSystemLoadDialog("Select an .edf file");
            //Check if the user opened a file
            if (openFileResult.bSuccess) {
                //We have a file, check it and process it
               edf_file_path = openFileResult.getPath();
            }
            setupEDF();
            clock.starttime = ofGetElapsedTimef();
            clock.start(this);
            break;
        }
            
        case '9':
            ofFileDialogResult openFileResult = ofSystemLoadDialog("Select an OSC config file");
            //Check if the user opened a file
            if (openFileResult.bSuccess) {
                //We have a file, check it and process it
                osc_file_path = openFileResult.getPath();
            }
            ofFile file(osc_file_path);
            ofBuffer buffer(file);
            hostname = buffer.getFirstLine();
            port = ofToInt(buffer.getNextLine());
            clock.framescale = ofToInt(buffer.getNextLine());
            break;
            
            
        //default:
           // break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}

//---------
void ofApp::updatebuffer() {
    if (option == 1) {
        if (readytocollect) {
            
            EE_DataUpdateHandle(0, hData);
            EE_DataGetNumberOfSample(hData,&nSamplesTaken);
            
            //std::cout << "Updated " << nSamplesTaken << std::endl;
            
            for (int i = 0 ; i<22 ; i++) { // 22 channels are being read
                EE_DataGet(hData, targetChannelList[i], data[i], nSamplesTaken);
            }
            
            updateAff();
        }
    }
    if (option == 2) {
        
        for (int i = 0 ; i<22 ; i++) { // 22 channels are being read
            nSamplesTaken = X_SIZE;
            edfread_physical_samples(hdr.handle, edfChannelList[i], nSamplesTaken, data[i]);
            //std::cout << "Updated " << nSamplesTaken << std::endl;
            
            if (edfseek(hdr.handle, edfChannelList[i], nSamplesTaken+1, EDFSEEK_CUR) == -1) {
                option = 0;
                edfseek(hdr.handle, edfChannelList[i], 0, EDFSEEK_SET);
                clock.stop();
            }
        }
    }
    
    if(averages[1] != 0) // unless we're on the first buffer...
        for(int i = 1; i<=14; i++) { // remove noise peaks
            if(abs(averages[i] - data[i][0]) > PEAK_AMP)
                data[i][0] = last[i];
            for (int j = 1; j<X_SIZE; j++)
                if(abs(averages[i] - data[i][j]) > PEAK_AMP)
                    data[i][j] = data[i][j-1];
        }
    
    for(int i = 1; i<=14; i++) {
        last[i] = data[i][X_SIZE-1];
    }
    
    for(int i = 1; i<=14; i++) { // get averages of the 14 EEG sensors
        averages[i] = 0;
        for (int j = 0; j<X_SIZE; j++)
            averages[i] += data[i][j];
        averages[i] /= X_SIZE;
        
        for (int j = 0; j<X_SIZE; j++) // move the 14 EEG sensors around zero
            data[i][j] -= averages[i];
    }
    
    maxframe[1] = 0;
    maxframe[14] = 0;
    for(int j=0; j<X_SIZE; j++) {
        if (abs(data[1][j]) > maxframe[1]) maxframe[1] = abs(data[1][j]);
        if (abs(data[14][j]) > maxframe[14]) maxframe[14] = abs(data[14][j]);
    }

}

//---------

void ofApp::updateAff() {
    EE_EmoEngineEventGetEmoState(eEvent, eState);
    engBo = ES_AffectivGetEngagementBoredomScore(eState);
    excLT = ES_AffectivGetExcitementLongTermScore(eState);
    excST = ES_AffectivGetExcitementShortTermScore(eState);
    frust = ES_AffectivGetFrustrationScore(eState);
    medit = ES_AffectivGetMeditationScore(eState);
}



//--------------

void ofApp::processFFT(int sensor) {
    // fill (circular) buffer
    for (int k = 1; k < X_SIZE; k++) {
        buf_fft[sensor][k-1] = buf_fft[sensor][k];
    }
    buf_fft[sensor][X_SIZE - 1] = data[sensor][clock.sampleIdx];

    //cout << clock.sampleIdx <<"/"<< buf_fft[sensor][0] << " " << buf_fft[sensor][1] << " " << buf_fft[sensor][2] << endl;
    
    //for (int k = 0; k<X_SIZE; k++) {
    //    fft_input[k] = data[sensor][k];
    //}
    
    
    vDSP_vmulD(buf_fft[sensor], 1, window, 1, fft_input, 1, X_SIZE); // multiply by window
    //convert to split complex format with evens in real and odds in imag
    vDSP_ctozD((DOUBLE_COMPLEX *)fft_input, 2, &split_data, 1, X_SIZE/2);
    
    // do fft
    vDSP_fft_zripD(fftSetup, &split_data, 1, log2(X_SIZE), FFT_FORWARD);
    split_data.imagp[0] = 0.0;
    
    vDSP_ztocD(&split_data, 1, (DOUBLE_COMPLEX *) fft_input, 2, X_SIZE/2);
    vDSP_polarD(fft_input, 2, fft_result, 2, X_SIZE/2);
    cblas_dcopy(X_SIZE/2, fft_result, 2, power_spectrum, 1);
    
    
    //fftw_execute( plan_forward );
    //power_spectrum[0] = fft_result[0]*fft_result[0];  /* DC component */
    //for (int k = 1; k < (X_SIZE+1)/2; ++k)  /* (k < N/2 rounded up) */
    //    power_spectrum[k] = fft_result[k]*fft_result[k] + fft_result[X_SIZE-k]*fft_result[X_SIZE-k];
    //power_spectrum[X_SIZE/2] = fft_result[X_SIZE/2]*fft_result[X_SIZE/2];  /* Nyquist freq. */
    
    delta[sensor]=theta[sensor]=alpha[sensor]=beta[sensor]=total[sensor]=0;
    
    float maxwave = 0;
    
    for (int i=0; i<(X_SIZE/2); i++) total[sensor] += power_spectrum[i]; //X_SIZE/2
    
    for (int i=0; i<4; i++) {
        maxwave = (maxwave > power_spectrum[i]) ? maxwave : power_spectrum[i];
        //delta[sensor] += power_spectrum[i];
    }
    delta[sensor] = maxwave;// / delta[sensor];
    maxwave = 0;
    
    for (int i=4; i<=8; i++) {
        maxwave = (maxwave > power_spectrum[i]) ? maxwave : power_spectrum[i];
        //theta[sensor] += power_spectrum[i];
    }
    theta[sensor] = maxwave;// / theta[sensor];
    maxwave = 0;
    
    for (int i=8; i<=13; i++) {
        maxwave = (maxwave > power_spectrum[i]) ? maxwave : power_spectrum[i];
        //alpha[sensor] += power_spectrum[i];
    }
    alpha[sensor] = maxwave;// / alpha[sensor];
    maxwave = 0;
    
    for (int i=14; i<=30; i++) {
        maxwave = (maxwave > power_spectrum[i]) ? maxwave : power_spectrum[i];
        //beta[sensor] += power_spectrum[i];
    }
    beta[sensor] = maxwave;// / beta[sensor];
    maxwave = 0;
    
    delta[sensor] /= total[sensor]; theta[sensor] /= total[sensor];
    alpha[sensor] /= total[sensor]; beta[sensor] /= total[sensor];
    
}


//-------------


void ofApp::sendOSC() {
    ofxOscBundle b;
    ofxOscMessage m;
    
    // send EEG waves
    
    m.setAddress("/14/alpha");
    m.addFloatArg(alpha[14]);
    b.addMessage(m);
    m.clear();
    m.setAddress("/14/beta");
    m.addFloatArg(beta[14]);
    b.addMessage(m);
    m.clear();
    m.setAddress("/14/delta");
    m.addFloatArg(delta[14]);
    b.addMessage(m);
    m.clear();
    m.setAddress("/14/theta");
    m.addFloatArg(theta[14]);
    b.addMessage(m);
    m.clear();

    m.setAddress("/14/max");
    m.addFloatArg(maxframe[14]);
    b.addMessage(m);
    m.clear();
    
    m.setAddress("/1/alpha");
    m.addFloatArg(alpha[1]);
    b.addMessage(m);
    m.clear();
    m.setAddress("/1/beta");
    m.addFloatArg(beta[1]);
    b.addMessage(m);
    m.clear();
    m.setAddress("/1/delta");
    m.addFloatArg(delta[1]);
    b.addMessage(m);
    m.clear();
    m.setAddress("/1/theta");
    m.addFloatArg(theta[1]);
    b.addMessage(m);
    m.clear();
    
    m.setAddress("/1/max");
    m.addFloatArg(maxframe[1]);
    b.addMessage(m);
    m.clear();
    
    m.setAddress("/engBo");
    m.addFloatArg(engBo);
    b.addMessage(m);
    m.clear();
    m.setAddress("/excLT");
    m.addFloatArg(excLT);
    b.addMessage(m);
    m.clear();
    m.setAddress("/excST");
    m.addFloatArg(excST);
    b.addMessage(m);
    m.clear();
    m.setAddress("/frust");
    m.addFloatArg(frust);
    b.addMessage(m);
    m.clear();
    m.setAddress("/medit");
    m.addFloatArg(medit);
    b.addMessage(m);
    m.clear();
    
    clock.sender.sendBundle(b);
}


//--------------

void ofApp::setupEDF() {
    const char *cstr = edf_file_path.c_str(); // convert string to char* (file path)
    
    if(edfopen_file_readonly(cstr, &hdr, EDFLIB_READ_ALL_ANNOTATIONS))
    {
        switch(hdr.filetype)
        {
            case EDFLIB_MALLOC_ERROR                : errortext = "\nmalloc error\n\n";
                break;
            case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY   : errortext = "\ncan not open file, no such file or directory\n\n";
                break;
            case EDFLIB_FILE_CONTAINS_FORMAT_ERRORS : errortext = "\nthe file is not EDF(+) or BDF(+) compliant\n"
                "(it contains format errors)\n\n";
                break;
            case EDFLIB_MAXFILES_REACHED            : errortext = "\nto many files opened\n\n";
                break;
            case EDFLIB_FILE_READ_ERROR             : errortext = "\na read error occurred\n\n";
                break;
            case EDFLIB_FILE_ALREADY_OPENED         : errortext = "\nfile has already been opened\n\n";
                break;
            default                                 : errortext = "\nunknown error\n\n";
                break;
        }
    }
}


//-----------


ofApp::~ofApp(){
    
    clock.stop();
    edfclose_file(hdr.handle);
    
    //fftw_destroy_plan( plan_forward );
    //fftw_free( fft_input );
    //fftw_free( fft_result );
    //fftw_free( power_spectrum );
    
    delete fft_input;
    delete fft_result;
    delete split_data.imagp;
    delete split_data.realp;
    delete power_spectrum;
    delete window;
    
    vDSP_destroy_fftsetupD(fftSetup);

    EE_DataFree(hData);
    EE_EngineDisconnect();
    EE_EmoStateFree(eState);
    EE_EmoEngineEventFree(eEvent);
    
}
