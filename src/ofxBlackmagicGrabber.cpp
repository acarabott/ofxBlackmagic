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
    deviceID            = 0;
    width               = 0.f;
    height              = 0.f;
}

ofxBlackmagicGrabber::~ofxBlackmagicGrabber() {
    close();
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
    // ensure that we return to our original deviceID
    controller.selectDevice(deviceID);

    return devices;
}

bool ofxBlackmagicGrabber::setDisplayMode(BMDDisplayMode displayMode) {
    if (!controller.init()
        || displayMode == bmdModeUnknown
        || !controller.startCaptureWithMode(displayMode))
    {
        return false;
    }

    this->width = controller.getDisplayModeInfo(deviceID).width;
    this->height = controller.getDisplayModeInfo(deviceID).height;

    return true;
}

bool ofxBlackmagicGrabber::initGrabber(int w, int h) {
    ofLogNotice("ofxBlackmagicGrabber") << "Using display mode with matching"
        << "width and height." << endl
        << "To use a specific framerate or video mode one of:" << endl
        << " - setDisplayMode(BMDDisplayMode displayMode)" << endl
        << " - initGrabber(int width, int height, float framerate)";

    controller.selectDevice(deviceID);
    vector<string> displayModes = controller.getDisplayModeNames();
    ofLogVerbose("ofxBlackmagicGrabber") << "Availabile display modes: " << endl
        << ofToString(displayModes);

    return setDisplayMode(controller.getDisplayMode(w, h));
}

bool ofxBlackmagicGrabber::initGrabber(int w, int h, int framerate) {
    return setDisplayMode(controller.getDisplayMode(w, h, framerate));
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

void ofxBlackmagicGrabber::setVerbose(bool bTalkToMe) {
    bVerbose = bTalkToMe;
}

void ofxBlackmagicGrabber::setDeviceID(int _deviceID) {
    deviceID = _deviceID;
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