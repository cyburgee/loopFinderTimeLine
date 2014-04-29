 #include "ofApp.h"

using namespace cv;
using namespace ofxCv;

void ofApp::setup(){

    videoLoaded = false;
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    numLoopsInRow = 5;
    
    minMovementThresh = 1;
    minMovementBool = true;
    maxMovementThresh = 20;
    maxMovementBool = false;
    loopThresh = 80.0;
    minChangeRatio = MAXFLOAT;
    minPeriodSecs = 0.5;
    maxPeriodSecs = 2.0;
    
    loopSelected = -1;
    
    gifNum = 0;
    ofAddListener(ofxGifEncoder::OFX_GIF_SAVE_FINISHED, this, &ofApp::onGifSaved);
    
    //GUI STUFF
    hideGUI = false;
    setGuiMatch();
    
    
    //LOAD VIDEO FILE
    ofFileDialogResult result = ofSystemLoadDialog("Choose A Video File",false);
    if(result.bSuccess){
        loadVideo(result.getPath(),result.getName());
    }
    
    font.loadFont("GUI/NewMedia Fett.ttf", 15, true, true);
	font.setLineHeight(34.0f);
	font.setLetterSpacing(1.035);
    
    //ofSetWindowShape(guiMatch->getGlobalCanvasWidth() + vidPlayer.getWidth(), vidPlayer.getHeight()*2);
    ofSetBackgroundColor(127);
    cout<< "frameRate: " << ofGetFrameRate() << endl;
    //frameStart = 0;
    //initEnds();
    //pausePlayback = true;
    scrolling = false;
    dragPoint = ofPoint(0,0);
    
    ofAddListener(timeline.events().playbackEnded, this, &ofApp::playStopped);
    ofAddListener(timeline.events().playbackStarted, this, &ofApp::playStarted);
    
    //guiMatch->loadSettings("guiMatchSettings.xml");
    /*if (videoLoaded) {
        loopWidth = (ofGetWidth()-guiMatch->getGlobalCanvasWidth())/numLoopsInRow;
        loopHeight = vidPlayer.getHeight()*(loopWidth/vidPlayer.getWidth());
        drawLoopPoint = ofPoint(guiMatch->getGlobalCanvasWidth() ,timeline.getBottomLeft().y + vidPlayer.getHeight() + loopHeight);
    }*/
    //needToInitEnds = true;
}


//------------------------------------------------------------------------------------
void ofApp::initEnds(){
    if (potentialLoopEnds.size() > 0){
        potentialLoopEnds.clear();
        //frameNumsInEnds.clear();
    }

    //vidPlayer.play();
    //vidPlayer.idleMovie();
    vidPlayer.setPaused(true);
    vidPlayer.setFrame(frameStart);
    //cout << "Updating Vid Player" << endl;
    vidPlayer.update();
    
    for (int i = frameStart; i < vidPlayer.getTotalNumFrames() && i < frameStart + maxPeriod; i++) {
        //vidPlayer.setFrame(i);
        //vidPlayer.update();
        cout << "init ends frame: " << vidPlayer.getCurrentFrame() << endl;
        cv::Mat currGray;
        cv::cvtColor(toCv(vidPlayer), currGray, CV_RGB2GRAY);
        cv::Mat currFrame;
        cv::resize(currGray, currFrame, imResize);
        
        potentialLoopEnds.push_back(currFrame);
        //frameNumsInEnds.push_back(i);
        vidPlayer.nextFrame();
        //cout << "Updating Vid Player" << endl;
        vidPlayer.update();
    }
    vidPlayer.setFrame(frameStart);
    //cout << "Updating Vid Player" << endl;
    vidPlayer.update();
    needToInitEnds = false;
}


//------------------------------------------------------------------------------------
void ofApp::populateLoopEnds(){
    if (potentialLoopEnds.size() > 0){
        potentialLoopEnds.erase(potentialLoopEnds.begin());
        //frameNumsInEnds.erase(frameNumsInEnds.begin());
        
        int endIdx = frameStart + maxPeriod;
        //cout << "adding frame at: " << endIdx << endl;
        vidPlayer.idleMovie();
        if (endIdx < vidPlayer.getTotalNumFrames()){
            vidPlayer.setFrame(endIdx);
            //cout << "Updating Vid Player" << endl;
            vidPlayer.update();
            
            cv::Mat endGray;
            cv::cvtColor(toCv(vidPlayer), endGray, CV_RGB2GRAY);
            cv::Mat endFrame;
            cv::resize(endGray, endFrame, imResize);
            
            potentialLoopEnds.push_back(endFrame);
            //frameNumsInEnds.push_back(endIdx);
        }
    }
    else{
        initEnds();
    }
    
    //vidPlayer.setFrame(timeline.getCurrentFrame());
    vidPlayer.setFrame(frameStart);
    //cout << "Updating Vid Player" << endl;
    vidPlayer.update();
}

//------------------------------------------------------------------------------------
bool ofApp::checkForLoop(){

    if (potentialLoopEnds.size() == 0)
        return false;
    
    cv::Mat start;
    potentialLoopEnds.at(0).copyTo(start);
    cv::Scalar startMean = cv::mean(start);
    cv::subtract(start, startMean, start);
    float startSum = cv::sum(start)[0] + 1; //add one to get rid of division by 0 if screen is black
    cv::Mat currEnd;
    for (int i = minPeriod; i < potentialLoopEnds.size(); i++) {
        cv::Mat currEnd;
        potentialLoopEnds.at(i).copyTo(currEnd);
        cv::subtract(currEnd, startMean, currEnd);
        cv::Mat diff;
        cv::absdiff(start, currEnd, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        float changeRatio = endDiff/startSum;
        
        
        if (changeRatio < minChangeRatio) {
            minChangeRatio = changeRatio;
            //cout << "min Ratio: " << minChangeRatio << endl;
        }
        
        cout << "frameStart: " << frameStart << endl;
        cout << "compare Frame: " << frameStart + i << endl;
        cout << "startSum: " << startSum << endl;
        cout << "endSum: " << cv::sum(currEnd)[0] + 1 << endl;
        cout << "endDiff: " << endDiff << endl;
        cout << "change Ratio: " << changeRatio*100 << endl;
        cout << "test Thresh: " << 100 - loopThresh << endl;
        cout << endl;
        
        if ((changeRatio*100) < (100 - loopThresh)){//set to change percentage
            potentialEndIdx = frameStart + i;
            //cout << "potentialEnd: " << potentialEndIdx << endl;
            if (minMovementBool || maxMovementBool){
                if  (hasMovement()){
                    matchIndeces.push_back(potentialEndIdx);
                    bestMatches.push_back(currEnd);
                }
                else{
                    //timeline.setCurrentFrame(potentialEndIdx);
                    //frameStart = potentialEndIdx;
                    //timeline.setCurrentFrame(frameStart);
                    //vidPlayer.setFrame(timeline.getCurrentFrame());
                    vidPlayer.setFrame(frameStart);
                    vidPlayer.update();
                    return false;
                }
            }
            else{
                matchIndeces.push_back(potentialEndIdx);
                bestMatches.push_back(currEnd);
            }
        }
    }
    
    if (bestMatches.size() > 0) {
        vidPlayer.setFrame(frameStart);
        vidPlayer.update();
        getBestLoop(start,startMean);
        vidPlayer.setFrame(frameStart);
        vidPlayer.update();
        return true;
    }
    
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    return false;
}

//------------------------------------------------------------------------------------
bool ofApp::hasMovement(){
    float sumDiff = 0;
    
    cv::Mat prevFrame;
    potentialLoopEnds.at(0).copyTo(prevFrame);
    cv::Scalar prevMean = cv::mean(prevFrame); //DO MEAN INSIDE LOOP?
    cv::subtract(prevFrame,prevMean,prevFrame);
    float prevSum = cv::sum(prevFrame)[0] + 1; //add one to get rid of division by 0 if screen is black

    cv::Mat currFrame;
    
    //for (int i = 1; i <= potentialEndIdx - timeline.getCurrentFrame(); i++) {
    for (int i = 1; i <= potentialEndIdx - frameStart; i++) {
        cv::Mat currFrame;
        potentialLoopEnds.at(i).copyTo(currFrame);
        cv::subtract(currFrame, prevMean, currFrame);
        cv::Mat diff;
        cv::absdiff(currFrame,prevFrame,diff);
        float endDiff = cv::sum(diff)[0] + 1;
        float changeRatio = endDiff;///prevSum;
        //cout << "movement Change Ratio: " << changeRatio << endl;
        sumDiff += changeRatio;
        prevFrame = currFrame;
        //prevSum = cv::sum(prevFrame)[0] + 1;
    }
    sumDiff /= prevSum;
    //cout << "sum Diff: " << sumDiff << endl;
    //cout << "minThreshold: " << minMovementThresh*(potentialEndIdx - frameStart)/100 << endl;
    if (minMovementBool) {
        //if(sumDiff < minMovementThresh*(potentialEndIdx - frameStart)/100 ){
        if(sumDiff < minMovementThresh/100 ){
            cout << "not enough movement" << endl;
            return false;
        }
    }
    else if (maxMovementBool){
        //if(sumDiff > maxMovementThresh*(potentialEndIdx - frameStart)/100 ){
        if(sumDiff > maxMovementThresh/100 ){
            cout << "too much movement" << endl;
            return false;
        }
    }
    else if (minMovementBool && maxMovementBool){
        //if(sumDiff > maxMovementThresh*(potentialEndIdx - frameStart)/100 && sumDiff < minMovementThresh*(potentialEndIdx - frameStart)/100){
        if(sumDiff > maxMovementThresh/100 && sumDiff < minMovementThresh/100){
            cout << "not enough movement or too much" << endl;
            return false;
        }
    }
    
    return true;
}

//------------------------------------------------------------------------------------
void ofApp::getBestLoop(cv::Mat start,cv::Scalar startMean){
    float minChange = MAXFLOAT;
    int bestEnd = -1;
    float startSum = cv::sum(start)[0] + 1; //add one to get rid of division by 0 if screen is black
    
    for (int i = 0; i < bestMatches.size(); i++) {
        cv::Mat currEnd;
        potentialLoopEnds.at(i).copyTo(currEnd);
        cv::subtract(currEnd,startMean,currEnd);
        cv::Mat diff;
        cv::absdiff(start, currEnd, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        float changeRatio = endDiff/startSum;
        
        //cout << "end of loop frame: " << matchIndeces.at(i) << endl;
        //add flow test here
        if (changeRatio <= minChange) {
            minChange = changeRatio;
            bestEnd = i;
        }
    }
    
    //ofQTKitDecodeMode decodeMode = OF_QTKIT_DECODE_PIXELS_ONLY;
    //ofQTKitPlayer loopFrameGrabber;
    //loopFrameGrabber.loadMovie(videoFilePath,decodeMode);
    if (bestEnd >= 0) {
        vector< ofImage> display;
        //vidPlayer.idleMovie();
        vidPlayer.setPaused(true);
        vidPlayer.setFrame(frameStart);
        cout << "Updating Vid Player" << endl;
        vidPlayer.update();
        
        for (int j = frameStart; j <= matchIndeces.at(bestEnd); j++) {
            //loopFrameGrabber.setFrame(j);
            //loopFrameGrabber.update();
            //vidPlayer.setFrame(j);
            //vidPlayer.update();
            ofImage loopee;
            loopee.setFromPixels(vidPlayer.getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
            //loopee.setFromPixels(loopFrameGrabber.getPixels(), loopFrameGrabber.getWidth(), loopFrameGrabber.getHeight(), OF_IMAGE_COLOR);
            loopee.update();
            ofImage displayIm;
            displayIm.clone(loopee);
            displayIm.resize(vidPlayer.getWidth()/numLoopsInRow, vidPlayer.getHeight()/numLoopsInRow);
            displayIm.update();
            //displayIm.resize(loopFrameGrabber.getWidth()/numLoopsInRow, loopFrameGrabber.getHeight()/numLoopsInRow);
            display.push_back(displayIm);
            vidPlayer.nextFrame();
            cout << "Updating Vid Player" << endl;
            vidPlayer.update();
        }
        vidPlayer.setFrame(frameStart);
        cout << "Updating Vid Player" << endl;
        vidPlayer.update();
        //loopFrameGrabber.close();
        
        //ofVec2f indices = ofVec2f(frameStart, matchIndeces.at(bestEnd));
        //int indices[2] = {frameStart,matchIndeces.at(bestEnd)};
        vector<int> indices;
        indices.push_back(frameStart);
        indices.push_back(matchIndeces.at(bestEnd));
        loopIndeces.push_back(indices);
        loopLengths.push_back(display.size());
        displayLoops.push_back(display);
        loopQuality.push_back(minChange);
        //loopQuality.push_back(1 - (minChange/100));
        loopPlayIdx.push_back(0);
        loopStartMats.push_back(start);
        
        if (!ditchSimilarLoop()) {
            if ( displayLoops.size() >= numLoopsInRow && displayLoops.size()%numLoopsInRow == 1) {
                ofRectangle aboveLoopRect = loopDrawRects.at(loopDrawRects.size() - numLoopsInRow);
                float rectBottom = aboveLoopRect.getBottom();
                float rectLeft = aboveLoopRect.getLeft();
                ofRectangle drawRect = ofRectangle(rectLeft, rectBottom, loopWidth, loopHeight);
                loopDrawRects.push_back(drawRect);
                
            }
            else if (loopDrawRects.size() > 0 && displayLoops.size()%numLoopsInRow != 1) {
                ofRectangle lastLoopRect = loopDrawRects.at(loopDrawRects.size()-1);
                float rectTop = lastLoopRect.getTop();
                float rectRight = lastLoopRect.getRight();
                ofRectangle drawRect = ofRectangle(rectRight, rectTop, loopWidth, loopHeight);
                loopDrawRects.push_back(drawRect);
            }
            else {
                int newHeight = ceil((double)displayLoops.size()/numLoopsInRow);
                ofRectangle drawRect = ofRectangle(drawLoopPoint, loopWidth, loopHeight);
                loopDrawRects.push_back(drawRect);
            }
        }
    }
    
    bestMatches.clear();
    matchIndeces.clear();
}


//------------------------------------------------------------------------------------
bool ofApp::ditchSimilarLoop(){
    if (displayLoops.size() == 1)
        return false;
    
    cv::Mat prevLoopMat;
    loopStartMats.at(loopStartMats.size()-2).copyTo(prevLoopMat);
    cv::Mat newLoopMat;
    loopStartMats.at(loopStartMats.size()-1).copyTo(newLoopMat);
    //cv::cvtColor(toCv(displayLoops.at(displayLoops.size()-2).at(0)), prevLoopGray, CV_RGB2GRAY);
    //cv::cvtColor(toCv(displayLoops.at(displayLoops.size()-1).at(0)), newLoopGray, CV_RGB2GRAY);
    //cv::Mat prevLoopFrame;
    //cv::Mat newLoopFrame;
    //cv::resize(prevLoopGray, prevLoopFrame, imResize);
    //cv::resize(newLoopGray, newLoopFrame, imResize);

    float prevLoopSum = cv::sum(prevLoopMat)[0] + 1;
    
    cv::Mat diff;
    cv::absdiff(prevLoopMat, newLoopMat, diff);
    float endDiff = cv::sum(diff)[0] + 1;
    
    float changeRatio = endDiff/prevLoopSum;
    cout << "changeRatio: " << changeRatio << endl;
    float changePercent = (changeRatio*100);
    cout << "changePercent: " << changePercent << endl;
    if((changeRatio*100) <= (100-loopThresh)){ // the first frames are very similar
        
        if (loopQuality.at(loopQuality.size() - 2) < loopQuality.at(loopQuality.size() -1)) {
            cout << "erasing last one" << endl;
            loopQuality.erase(loopQuality.end() - 1);
            loopLengths.erase(loopLengths.end() - 1);
            loopPlayIdx.erase(loopPlayIdx.end() - 1);
            displayLoops.erase(displayLoops.end() - 1);
            loopIndeces.erase(loopIndeces.end() - 1);
            loopStartMats.erase(loopStartMats.end() - 1);
        }
        else{
            cout << "erasing second to last" << endl;
            loopQuality.erase(loopQuality.end() - 2 );
            loopLengths.erase(loopLengths.end() - 2);
            loopPlayIdx.erase(loopPlayIdx.end() - 2);
            displayLoops.erase(displayLoops.end() - 2);
            loopIndeces.erase(loopIndeces.end() - 2);
            loopStartMats.erase(loopStartMats.end() - 2);
        }
        return true;
        
    }
    cout << "keeping loop" << endl;
    return false;

}

//------------------------------------------------------------------------------------
void ofApp::update(){
    if (!refiningLoop){
        frameStart = timeVid.getCurrentFrame();
        if (videoLoaded && timeline.getIsPlaying() && !needToInitEnds) {
            if (frameStart >= vidPlayer.getTotalNumFrames()){
                //frameStart = 0;
                cout << "restarting?"<<endl;
                initEnds();
                return;
            }

            if(checkForLoop()){
                //cout << "Loop Found" << endl;
            }
            
            populateLoopEnds();
            timeVid.nextFrame();
            //cout << "Updating Time Vid" << endl;
            timeVid.update();
        }
        else if (videoLoaded && needToInitEnds)
            initEnds();
        
        for (int i = 0; i < loopPlayIdx.size(); i++) {
            loopPlayIdx.at(i)++;
            if(loopPlayIdx.at(i) >= loopLengths.at(i))
                loopPlayIdx.at(i) = 0;
        }
        //mostReceIndex ++;
        //if (mostReceIndex >= mostRecentSaved.size())
        //    mostReceIndex = 0;
    }
    else{
        needToInitEnds = false;
        if(timeline.getIsPlaying()){
            timeVid.nextFrame();
            //cout << "Updating Time Vid" << endl;
            timeVid.update();
        }
    }
    if (videoLoaded) {
        drawLoopPoint = ofPoint(guiMatch->getGlobalCanvasWidth() ,timeline.getBottomLeft().y + vidPlayer.getHeight() + loopHeight);
        loopThumbBounds = ofRectangle(drawLoopPoint, ofGetWidth()-guiMatch->getGlobalCanvasWidth(), ofGetHeight()-timeline.getBottomLeft().y-timeVid.getHeight()-loopHeight);
    }
    //cout << "running Update" << endl;
}

//------------------------------------------------------------------------------------
void ofApp::draw(){
    
    ofBackground(127);
    
    if(videoLoaded){
        
        for (int i = 0; i < displayLoops.size(); i++) {
            ofImage loopFrame = displayLoops.at(i).at(loopPlayIdx.at(i));
            loopFrame.draw(loopDrawRects.at(i));
        }
        
        ofPushStyle();
        ofSetColor(64, 64 , 64, 200);
        ofPushMatrix();
        ofRect(guiMatch->getGlobalCanvasWidth(),0,ofGetWidth(),timeline.getBottomLeft().y + vidPlayer.getHeight()+ loopHeight);
        ofRect(0, 0, guiMatch->getGlobalCanvasWidth(), ofGetHeight());
        ofPopMatrix();
        ofPopStyle();
        
        if (loopSelected >= 0){
            ofPushStyle();
            ofNoFill();
            ofSetColor(0, 255, 255);
            ofSetLineWidth(2);
            ofRect(loopDrawRects.at(loopSelected));
            ofPopStyle();
        }
        if (!savingGif){
            ofPushMatrix();
            ofTranslate(0, timeline.getBottomLeft().y);
            //if (mostRecentSaved.size() > 0)
            //    mostRecentSaved.at(mostReceIndex).draw(0, 0);
            timeVid.draw(vidRect);
            ofPopMatrix();
        }
    }
    else{
        string loadVidInstruct = "No video loaded. Please click 'Load Video' and choose a valid video file.";
        float width = font.getStringBoundingBox(loadVidInstruct, 0, 0).width;
        font.drawString(loadVidInstruct, ofGetWidth()/2 - width/2, 3*ofGetHeight()/4);
    }
    timeline.draw();
    guiMatch->setPosition(0, timeline.getBottomLeft().y);
}

//------------------------------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
    if (scrolling && !refiningLoop){
        for (int i = 0; i < loopDrawRects.size(); i++) {
            dragPoint = ofPoint(0, (y - yStart)*0.5);
            loopDrawRects.at(i).translate(dragPoint);
        }
        xStart = x;
        yStart = y;
    }
}

//------------------------------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if (!refiningLoop) {
        for (int i = 0; i < loopDrawRects.size(); i++) {
            if (loopDrawRects.at(i).inside(x, y)) {
                loopSelected = i;
                string changeString;
                instructions = "Press 's' to save loop as a GIF. Press 'r' to refine the loop";
                setGuiInstructions();
            }
        }
        if (loopDrawRects.size() > 0 && loopThumbBounds.inside(x, y)) {
            scrolling = true;
            xStart = x;
            yStart = y;
        }
    }
}

//------------------------------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    scrolling = false;
}


//------------------------------------------------------------------------------------
void ofApp::saveGif(int i){
    savingGif = true;
    bool timeWasPlaying = timeline.getIsPlaying();
    if (timeWasPlaying) {
        timeline.stop();
    }
    ofFileDialogResult result = ofSystemSaveDialog(outputPrefix + ".gif", "Save Loop as Gif");
    if(result.bSuccess){
        
        ofxGifEncoder *giffy = new ofxGifEncoder();
        (*giffy).setup(vidPlayer.getWidth(), vidPlayer.getHeight(), 1/fps, 256);
        int currFrame = vidPlayer.getCurrentFrame();
        //int saveStartFrame = (int)(loopIndeces.at(i).x);
        int saveStartFrame = (loopIndeces.at(i))[0];
        cout << "saveStartFrame: " << saveStartFrame << endl;
        //int saveEndFrame = (int)(loopIndeces.at(i).y);
        int saveEndFrame = (loopIndeces.at(i))[1];
        cout << "saveEndFrame: " << saveEndFrame << endl;
        
        ofQTKitDecodeMode decodeMode = OF_QTKIT_DECODE_PIXELS_ONLY;
        ofQTKitPlayer frameSaver;
        frameSaver.loadMovie(videoFilePath,decodeMode);
        //frameSaver.idleMovie();
        frameSaver.setSynchronousSeeking(true);
//FIX THIS!!-----------------------------------------------------
        //int currFrameNum = vidPlayer.getCurrentFrame();
        frameSaver.play();
        frameSaver.setPaused(true);
        frameSaver.setFrame(saveStartFrame-1);
        //cout << "Updating FrameSaver" << endl;
        frameSaver.update();
        frameSaver.nextFrame();
        //cout << "Updating FrameSaver" << endl;
        frameSaver.update();
        //mostRecentSaved.clear();
        //mostReceIndex = 0;
        
        while (frameSaver.getCurrentFrame() <= saveEndFrame) {
            (*giffy).addFrame(frameSaver.getPixels(), frameSaver.getWidth(),frameSaver.getHeight());
            frameSaver.nextFrame();
            //cout << "Updating FrameSaver" << endl;
            frameSaver.update();
        }
        frameSaver.closeMovie();
        frameSaver.close();
        string gifFileName = result.getPath();
        if (gifFileName.size() >= 3  && gifFileName.substr(gifFileName.size() - 4) != ".gif")
            gifFileName += ".gif";
        else if (gifFileName.size() <= 3)
            gifFileName += ".gif";
        cout << "gifFileName: " << gifFileName << endl;
        (*giffy).save(gifFileName);
        gifNum++;
        gifses.push_back(giffy);
    }
    
    if (timeWasPlaying) {
        timeline.play();
    }
    savingGif = false;
}


//-------------------------------------------------------------------
void ofApp::onGifSaved(string &fileName) {
    cout << "gif saved as " << fileName << endl;
    (*gifses.at(gifses.size() -1 )).exit();
}


//-------------------------------------------------------------------
void ofApp::exit() {
    for (int i = 0; i < gifses.size(); i++) {
        (*gifses.at(i)).exit();
    }
    
    if(videoLoaded){
        //guiMatch->saveSettings("guiMatchSettings.xml");
        delete guiMatch;
    }
}


//-------------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(guiMatch->hasKeyboardFocus())
    {
        return;
    }
	switch (key)
	{
		//case 'h':
        //    guiMatch->toggleVisible();
		//	break;
        case 's':
            if (loopSelected >=0 && !refiningLoop){
                //timeline.stop();
                saveGif(loopSelected);
            }
            break;
        case 'r':
            if (loopSelected >=0 && !refiningLoop) {
                //timeline.stop();
                refineLoop();
            }
            else if(refiningLoop){
                //timeline.stop();
                ofRange inOutTotal = ofRange(0.0,1.0);
                timeline.setInOutRange(inOutTotal);
                timeline.getZoomer()->setViewRange(oldRange);
                refiningLoop = false;
                //needToInitEnds = true;
                updateRefinedLoop();
                needToInitEnds = true;
            }
            break;
        case 'n':
            if (refiningLoop){
                ofRange inOutTotal = ofRange(0.0,1.0);
                timeline.setInOutRange(inOutTotal);
                timeline.getZoomer()->setViewRange(oldRange);
                //timeline.stop();
                //needToInitEnds = true;
                instructions = "Press 's' to save loop as a GIF. Press 'r' to keep your changes to the loop.";
                setGuiInstructions();
                needToInitEnds = true;
                refiningLoop = false;
            }
            break;
        case ' ':
            if (timeline.getIsPlaying()){
                pauseInstruct = "Press the spacebar to run. ";
            }
            else{
                pauseInstruct = "Press the spacebar to pause. ";
            }
            setGuiInstructions();
            break;
        case 'o':
            if(refiningLoop){
                tempLoopIndeces[1] --;
                int end = tempLoopIndeces[1];
                timeline.setOutPointAtFrame(end);
                timeline.getZoomer()->setViewRange(timeline.getInOutRange());
            }
            break;
        case 'p':
            if(refiningLoop){
                tempLoopIndeces[1] ++;
                int end = tempLoopIndeces[1];
                timeline.setOutPointAtFrame(end);
                timeline.getZoomer()->setViewRange(timeline.getInOutRange());
            }
            break;
        case 'k':
            if(refiningLoop){
                tempLoopIndeces[0] --;
                int start = tempLoopIndeces[0];
                timeline.setInPointAtFrame(start);
                timeline.getZoomer()->setViewRange(timeline.getInOutRange());
            }
            break;
        case 'l':
            if(refiningLoop){
                tempLoopIndeces[0] ++;
                int start = tempLoopIndeces[0];
                timeline.setInPointAtFrame(start);
                timeline.getZoomer()->setViewRange(timeline.getInOutRange());
            }
            break;
		default:
			break;
	}
}


//------------------------------------------------------------------------------------
void ofApp::refineLoop(){
    oldRange = timeline.getZoomer()->getViewRange();
    refiningLoop = true;
    int start = (loopIndeces.at(loopSelected))[0];
    int end = (loopIndeces.at(loopSelected))[1];
    tempLoopIndeces[0] = start;
    tempLoopIndeces[1] = end;
    timeVid.setFrame(start);
    timeVid.update();
    timeline.stop();
    timeline.setInPointAtFrame(start);
    timeline.setOutPointAtFrame(end);
    timeline.getZoomer()->setViewRange(timeline.getInOutRange());
    instructions = "Press 'r' to keep your changes to the loop. Press 'n' to cancel and go back. Press 'o' and 'p' to move the end one frame back or forward. Press 'k' and 'l' to move the beginning one frame back or forward.";
    setGuiInstructions();
}


//------------------------------------------------------------------------------------
void ofApp::updateRefinedLoop(){
    int prevFrame = vidPlayer.getCurrentFrame();
    vector<ofImage> display;
    //vidPlayer.idleMovie();
    vidPlayer.setPaused(true);
    //std::copy(tempLoopIndeces, tempLoopIndeces + 2, loopIndeces.at(loopSelected));
    vector<int> indices;
    indices.push_back(tempLoopIndeces[0]);
    indices.push_back(tempLoopIndeces[1]);
    //loopIndeces.at(loopSelected) = indices;
    vector< vector<int> >::iterator itIn = loopIndeces.begin() + loopSelected;
    itIn = loopIndeces.erase(itIn);
    loopIndeces.insert(itIn,indices);
    
    int startIndx = (loopIndeces.at(loopSelected))[0];
    if (startIndx > 0) {
        vidPlayer.setFrame(startIndx - 1);
        vidPlayer.update();
        vidPlayer.nextFrame();
    }
    else
        vidPlayer.setFrame(startIndx);

    vidPlayer.update();
    cv::Mat startGray;
    cv::cvtColor(toCv(vidPlayer), startGray, CV_RGB2GRAY);
    cv::Mat start;
    cv::resize(startGray, start, imResize);
    cv::Scalar startMean = cv::mean(startMean);
    cv::subtract(start,startMean,start);
    float startSum = cv::sum(start)[0] + 1;
    
    //while(vidPlayer.getCurrentFrame() <= (int)loopIndeces.at(loopSelected).y){
    while(vidPlayer.getCurrentFrame() <= (loopIndeces.at(loopSelected))[1]){
        ofImage loopee;
        loopee.setFromPixels(vidPlayer.getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
        loopee.update();
        ofImage displayIm;
        displayIm.clone(loopee);
        displayIm.resize(vidPlayer.getWidth()/numLoopsInRow, vidPlayer.getHeight()/numLoopsInRow);
        displayIm.update();
        display.push_back(displayIm);
        vidPlayer.nextFrame();
        //cout << "Updating Vid Player" << endl;
        vidPlayer.update();
        //potentialLoopEnds.at(i).copyTo(currEnd);
    }
    vidPlayer.previousFrame();
    vidPlayer.update();
    cv::Mat endGray;
    cv::cvtColor(toCv(vidPlayer),endGray,CV_RGB2GRAY);
    cv::Mat end;
    cv::resize(endGray, end, imResize);
    cv::subtract(end, startMean, end);
    cv::Mat diff;
    cv::absdiff(start, end, diff);
    float endDiff = cv::sum(diff)[0] + 1;
    float changeRatio = endDiff/startSum;
    
    vidPlayer.setFrame(prevFrame);
    //cout << "Updating Vid Player" << endl;
    vidPlayer.update();
    
    vector<int>::iterator it = loopLengths.begin()+loopSelected;
    it = loopLengths.erase(it);
    loopLengths.insert(it, display.size());
    vector<vector<ofImage> >::iterator it2 = displayLoops.begin()+loopSelected;
    it2 = displayLoops.erase(displayLoops.begin()+loopSelected);
    displayLoops.insert(it2, display);
    vector<int>::iterator it3 = loopPlayIdx.begin()+loopSelected;
    it3 = loopPlayIdx.erase(it3);
    loopPlayIdx.insert(it3, 0);
    vector<float>::iterator it4 = loopQuality.begin()+loopSelected;
    it4 = loopQuality.erase(it4);
    loopQuality.insert(it4, changeRatio);
    vector<cv::Mat>::iterator it5 = loopStartMats.begin()+loopSelected;
    it5 = loopStartMats.erase(it5);
    loopStartMats.insert(it5, start);
    //loopLengths.at(loopSelected) = display.size();
    //displayLoops.at(loopSelected) = display;
    //loopPlayIdx.at(loopSelected) = 0;
    //loopQuality.at(loopSelected) = changeRatio;
    //loopStartMats.at(loopSelected) = start;
}


//------------------------------------------------------------------------------------
void ofApp::keyReleased(int key){
}


//------------------------------------------------------------------------------------
//GUI STUFF
void ofApp::guiEvent(ofxUIEventArgs &e)
{
	string name = e.getName();
    if(name == "Load Video"){
        videoLoaded = false;
		ofxUIButton *button = (ofxUIButton *) e.getButton();
        if (button->getValue()){
            timeline.stop();
            //pausePlayback = true;
            ofFileDialogResult result = ofSystemLoadDialog("Choose a Video File",false);
            if(result.bSuccess){
                //need to clear old data
                //timeline.removeTrack("Video");
                loadVideo(result.getPath(),result.getName());
            }
        }
	}
    else if (name == "Toggle Min Movement"){
        minMovementBool = e.getToggle()->getValue();
        minMove->setVisible(minMovementBool);
    }
    else if (name == "Toggle Max Movement"){
        maxMovementBool = e.getToggle()->getValue();
        maxMove->setVisible(maxMovementBool);
    }
    else if(name == "Min Movement") {
        minMovementThresh = e.getSlider()->getValue();
    }
    else if(name == "Max Movement") {
        maxMovementThresh = e.getSlider()->getValue();
    }
    else if(name == "Length Range"){
        if(loopLengthSlider->getState() != OFX_UI_STATE_DOWN){
            timeline.stop();
            minPeriod = (int)(fps*minPeriodSecs);
            maxPeriod = (int)(fps*maxPeriodSecs);
        }
    }
}


//------------------------------------------------------------------------------------
void ofApp::setGuiInstructions(){
    instructLabel->setTextString(pauseInstruct + instructions);
    instructLabel->stateChange();
    instructLabel->update();
    instructLabel->setVisible(true);
    guiMatch->update();
    guiMatch->autoSizeToFitWidgets();
}

//------------------------------------------------------------------------------------
void ofApp::setGuiMatch(){
	
	guiMatch = new ofxUISuperCanvas("Loop Settings");
    
    guiMatch->addSpacer();
    guiMatch->addLabel("Load Video", OFX_UI_FONT_SMALL);
    guiMatch->addLabelButton("Load Video", false);

    guiMatch->addSpacer();
    guiMatch->addLabel("Valid Loop Lengths",OFX_UI_FONT_SMALL);
    loopLengthSlider = guiMatch->addRangeSlider("Length Range", 0.25, 8.0, &minPeriodSecs, &maxPeriodSecs);
    loopLengthSlider->setTriggerType(OFX_UI_TRIGGER_ALL);
    loopLengthSlider->setValueHigh(maxPeriodSecs);
    loopLengthSlider->setValueLow(minPeriodSecs);
    
    guiMatch->addSpacer();
	guiMatch->addLabel("Match Slider (percent)",OFX_UI_FONT_SMALL);
	ofxUISlider *match = guiMatch->addSlider("Accuracy", 70.0, 100.0, &loopThresh);
    match->setValue(loopThresh);
    match->setTriggerType(OFX_UI_TRIGGER_ALL);
    
	guiMatch->addSpacer();
    guiMatch->addLabel("Min Movement (% change)",OFX_UI_FONT_SMALL);
    guiMatch->addToggle( "Toggle Min Movement", &minMovementBool);
    minMove = guiMatch->addSlider("Min Movement", 0.0, 100.0, &minMovementThresh);
    minMove->setValue(minMovementThresh);
    minMove->setTriggerType(OFX_UI_TRIGGER_ALL);
    minMove->setVisible(minMovementBool);
    
    guiMatch->addSpacer();
    guiMatch->addLabel("Max Movement (% change)",OFX_UI_FONT_SMALL);
    guiMatch->addToggle("Toggle Max Movement", &maxMovementBool);
	maxMove = guiMatch->addSlider("Max Movement", 0.0, 100.0, &maxMovementThresh);
    maxMove->setValue(maxMovementThresh);
    maxMove->setTriggerType(OFX_UI_TRIGGER_ALL);
    
    guiMatch->addSpacer();
    guiMatch->addLabel("Instructions");
    pauseInstruct = "Press the spacebar to run. ";
    instructions = "";
    instructLabel = guiMatch->addTextArea("Instructions", pauseInstruct + instructions, OFX_UI_FONT_SMALL);
    
    guiMatch->autoSizeToFitWidgets();
    maxMove->setVisible(maxMovementBool);
	ofAddListener(guiMatch->newGUIEvent,this,&ofApp::guiEvent);
}


//--------------------------------------------------------------
void ofApp::loadVideo(string videoPath, string videoName){
    
    int lastindex = videoName.find_last_of(".");
    outputPrefix = videoName.substr(0, lastindex);
    
    //videoTrack = timeline.getVideoTrack("Video");
    
    //TIMELINE STUFF
    timeline.clear();
    timeline.setup();
    timeline.removeFromThread();
    timeline.setAutosave(false);
    timeline.setShowTicker(false);
    timeline.setShowBPMGrid(false);
    ofRange inOut = ofRange(0,1.0);
    timeline.setInOutRange(inOut);
    timeline.getZoomer()->setViewExponent(1);
    //set big initial duration, longer than the video needs to be
	timeline.setDurationInFrames(20000);
	timeline.setLoopType(OF_LOOP_NORMAL);
    
    loopSelected = -1;
    
    cout << "vidPath: " << videoPath << endl;
    if(videoTrack == NULL){
	    videoTrack = timeline.addVideoTrack(videoName, videoPath);
        videoLoaded = (videoTrack != NULL);
        videoTrack = timeline.getVideoTrack(videoName);
    }
    else{
        videoLoaded = videoTrack->load(videoPath);
    }
    
    if(videoLoaded){
        int vidWidth = videoTrack->getPlayer()->getWidth();
        int vidHeight = videoTrack->getPlayer()->getHeight();
        vidRect = ofRectangle(guiMatch->getGlobalCanvasWidth() + (ofGetWidth() - guiMatch->getGlobalCanvasWidth())/2 - vidWidth/2, 0, vidWidth, videoTrack->getPlayer()->getHeight());
        frameBuffer.allocate(vidRect.getWidth(),vidRect.getHeight(),GL_RGB);
        
        timeline.setDurationInFrames(videoTrack->getPlayer()->getTotalNumFrames());
        cout << "totes frames: " << videoTrack->getPlayer()->getTotalNumFrames() << endl;
        cout << timeline.getDurationInFrames() << endl;
        timeline.setTimecontrolTrack(videoTrack); //video playback will control the time
		timeline.bringTrackToTop(videoTrack);
        cout << "outFrame: " << timeline.getOutFrame() << endl;
        cout << "outFrame: " << timeline.getOutFrame() << endl;
        videoTrack->getPlayer()->setVolume(0);
        videoTrack->getPlayer()->stop();
        timeVid = *(videoTrack->getPlayer());
        timeline.setFrameBased(true);
        videoTrack->setPlayAlongToTimeline(true);
        
        scale = 10;
        imResize = cv::Size(videoTrack->getPlayer()->getWidth()/scale,videoTrack->getPlayer()->getHeight()/scale);
        guiMatch->setPosition(0, timeline.getBottomLeft().y);
        ofQTKitDecodeMode decodeMode = OF_QTKIT_DECODE_PIXELS_ONLY;
        
        vidPlayer.loadMovie(videoPath,decodeMode);
        vidPlayer.setSynchronousSeeking(true);
        //vidPlayer.idleMovie();
        vidPlayer.play();
        vidPlayer.setPaused(true);
        videoFilePath = videoPath;
        //pausePlayback = true;
        fps = (float)vidPlayer.getTotalNumFrames()/vidPlayer.getDuration();
        minPeriod = (int)(fps*minPeriodSecs);
        maxPeriod = (int)(fps*maxPeriodSecs);
        loopWidth = (ofGetWidth()-guiMatch->getGlobalCanvasWidth())/numLoopsInRow;
        loopHeight = vidPlayer.getHeight()*(loopWidth/vidPlayer.getWidth());
        drawLoopPoint = ofPoint(guiMatch->getGlobalCanvasWidth() ,timeline.getBottomLeft().y + vidPlayer.getHeight() + loopHeight);
        loopThumbBounds = ofRectangle(guiMatch->getGlobalCanvasWidth(), timeline.getBottomLeft().y + vidPlayer.getHeight() + loopHeight, ofGetWidth()-guiMatch->getGlobalCanvasWidth(), ofGetHeight()-vidPlayer.getHeight() - timeline.getBottomLeft().y - loopHeight);
        ofSetFrameRate(fps);
        refiningLoop = false;
        needToInitEnds = true;
    }
    else{
        videoPath = "";
    }
    //settings.setValue("videoPath", videoPath);
    //settings.saveFile();
    
}

void ofApp::playStopped(ofxTLPlaybackEventArgs& bang){
    pauseFrameNum = timeline.getCurrentFrame();
    cout << "paused" << endl;
}

void ofApp::playStarted(ofxTLPlaybackEventArgs& bang){
    if (timeline.getCurrentFrame() != pauseFrameNum){
        cout << "playhead changed" << endl;
        needToInitEnds = true;
    }
}



