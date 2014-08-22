#include "ofxBlackmagicGrabber.h"

#include "ColorConversion.h"

ofxBlackmagicGrabber::ofxBlackmagicGrabber()
:grayPixOld(true)
,colorPixOld(true)
,yuvTexOld(true)
,grayTexOld(true)
,colorTexOld(true) {

    bIsFrameNew = false;
}

ofxBlackmagicGrabber::~ofxBlackmagicGrabber() {
    close();
}

bool ofxBlackmagicGrabber::setup(int width, int height, float framerate) {
    if(!controller.init()) {
        return false;
    }
    controller.selectDevice(0);
    vector<string> displayModes = controller.getDisplayModeNames();
    ofLogVerbose("ofxBlackmagicGrabber") << "Available display modes: " << ofToString(displayModes);
    BMDDisplayMode displayMode = bmdModeUnknown;
    if(width == 3840 && height == 2160 && framerate == 30) {
        displayMode = bmdMode4K2160p2997;
    } else if(width == 1920 && height == 1080 && framerate == 30) {
        displayMode = bmdModeHD1080p30;
    }else{
        ofLogError("ofxBlackmagicGrabber") << "ofxBlackmagicGrabber needs to be updated to support that mode.";
        return false;
    }
    if(!controller.startCaptureWithMode(displayMode)) {
        return false;
    }
    this->width = width, this->height = height;
    return true;
}

void ofxBlackmagicGrabber::clearMemory() {
    yuvPix.clear();
    grayPix.clear();
    colorPix.clear();

    yuvTex.clear();
    grayTex.clear();
    colorTex.clear();
}

void ofxBlackmagicGrabber::close() {
    if(controller.isCapturing()) {
        controller.stopCapture();
    }

    clearMemory();
}

void ofxBlackmagicGrabber::update() {
    if(controller.buffer.swapFront()) {
        grayPixOld = true, colorPixOld = true;
        yuvTexOld = true, grayTexOld = true, colorTexOld = true;
        bIsFrameNew = true;
    } else {
        bIsFrameNew = false;
    }
}

bool isFrameNew() {
    return isFrameNew;
}

float ofxBlackmagicGrabber::getWidth() {
    return width;
}

float ofxBlackmagicGrabber::getHeight() {
    return height;
}

vector<unsigned char>& ofxBlackmagicGrabber::getYuvRaw() {
    return controller.buffer.getFront();
}

ofPixels& ofxBlackmagicGrabber::getGrayPixels() {
    if(grayPixOld) {
        grayPix.allocate(width, height, OF_IMAGE_GRAYSCALE);
        unsigned int n = width * height;
        cby0cry1_to_y(&(getYuvRaw()[0]), grayPix.getPixels(), n);
        grayPixOld = false;
    }
    return grayPix;
}

ofPixels& ofxBlackmagicGrabber::getColorPixels() {
    if(colorPixOld) {
        colorPix.allocate(width, height, OF_IMAGE_COLOR);
        unsigned int n = width * height;
        cby0cry1_to_rgb(&(getYuvRaw()[0]), colorPix.getPixels(), n);
        colorPixOld = false;
    }
    return colorPix;
}

ofTexture& ofxBlackmagicGrabber::getYuvTexture() {
    if(yuvTexOld) {
        yuvTex.loadData(&(getYuvRaw()[0]), width / 2, height, GL_RGBA);
        yuvTexOld = false;
    }
    return yuvTex;
}

ofTexture& ofxBlackmagicGrabber::getGrayTexture() {
    if(grayTexOld) {
        grayTex.loadData(getGrayPixels());
        grayTexOld = false;
    }
    return grayTex;
}

ofTexture& ofxBlackmagicGrabber::getColorTexture() {
    if(colorTexOld) {
        colorTex.loadData(getColorPixels());
        colorTexOld = false;
    }
    return colorTex;
}

void ofxBlackmagicGrabber::drawYuv(){
    getYuvTexture().draw(0, 0);
}

void ofxBlackmagicGrabber::drawGray() {
    getGrayTexture().draw(0, 0);
}

void ofxBlackmagicGrabber::drawColor() {
    getColorTexture().draw(0, 0);
}