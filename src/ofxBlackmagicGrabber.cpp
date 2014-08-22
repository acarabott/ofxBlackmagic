#include "ofxBlackmagicGrabber.h"

#include "ColorConversion.h"

ofxBlackmagicGrabber::ofxBlackmagicGrabber()
:grayPixOld(true)
,colorPixOld(true)
,yuvTexOld(true)
,grayTexOld(true)
,colorTexOld(true) {

    // common
    bIsFrameNew         = false;
    bVerbose            = false;
}

ofxBlackmagicGrabber::~ofxBlackmagicGrabber() {
    close();
}

void ofxBlackmagicGrabber::setVerbose(bool bTalkToMe) {
    bVerbose = bTalkToMe;
}

const vector<ofVideoFormat> ofxBlackmagicGrabber::listDeviceFormats() {
    vector<ofVideoFormat> formats;
    vector<DisplayModeInfo> infoList = controller.getDisplayModeInfoList();

    for (int modeIndex = 0; modeIndex < infoList.size(); modeIndex++) {
        ofVideoFormat format;

        format.pixelFormat = OF_PIXELS_UNKNOWN;
        format.width = infoList[modeIndex].width;
        format.height = infoList[modeIndex].height;

        vector<float> framerates;
        framerates.push_back(infoList[modeIndex].framerate);
        format.framerates = framerates;

        formats.push_back(format);
    }
}

vector<ofVideoDevice> ofxBlackmagicGrabber::listDevices() {
    vector<ofVideoDevice> devices;
    vector<string> deviceNames = controller.getDeviceNameList();

    for (int i = 0; i < controller.getDeviceCount(); ++i) {
        ofVideoDevice device;
        device.id           = i;
        device.deviceName   = deviceNames[i];
        device.hardwareName = deviceNames[i];
        device.bAvailable   = controller.selectDevice(i);
        device.formats      = listDeviceFormats();

        devices.push_back(device);

    }

    return devices;
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