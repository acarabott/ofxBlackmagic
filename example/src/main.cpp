#include "ofMain.h"
#include "ofxBlackmagicGrabber.h"

class RateTimer {
protected:
	float lastTick, averagePeriod, smoothing;
	bool secondTick;
public:
	RateTimer() :
	smoothing(.9) {
		reset();
	}
	void reset() {
		lastTick = 0, averagePeriod = 0, secondTick = false;
	}
	void setSmoothing(float smoothing) {
		this->smoothing = smoothing;
	}
	float getFramerate() {
		return averagePeriod == 0 ? 0 : 1 / averagePeriod;
	}
	void tick() {
		float curTick = ofGetElapsedTimef();
		if(lastTick == 0) {
			secondTick = true;
		} else {
			float curDiff = curTick - lastTick;;
			if(secondTick) {
				averagePeriod = curDiff;
				secondTick = false;
			} else {
				averagePeriod = ofLerp(curDiff, averagePeriod, smoothing);
			}
		}
		lastTick = curTick;
	}
};

class ofApp : public ofBaseApp {
public:
    ofVideoGrabber cam;
	RateTimer timer;

	void setup() {
		ofSetLogLevel(OF_LOG_VERBOSE);
        ofPtr<ofxBlackmagicGrabber> blackmagicGrabber(new ofxBlackmagicGrabber());
        // setTextureFormat not in ofVideoGrabber API, so we call directly
        blackmagicGrabber->setTextureFormat(OF_BM_TEX_FORMAT_RGB);
        cam.setGrabber(blackmagicGrabber);
        cam.setDeviceID(0);
        cam.setDesiredFrameRate(30);
        cam.initGrabber(1920, 1080);
	}
	void exit() {
		cam.close();
	}
	void update() {
		cam.update();
        if (cam.isFrameNew()) {
            timer.tick();
        }

	}
	void draw() {
		cam.draw(0, 0, 1920, 1080);
		ofDrawBitmapStringHighlight(ofToString((int) timer.getFramerate()), 10, 20);
	}
	void keyPressed(int key) {
		if(key == 'f') {
			ofToggleFullscreen();
		}
	}
};

int main() {
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp());
}
