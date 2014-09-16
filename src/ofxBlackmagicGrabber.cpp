#include "ofxBlackmagicGrabber.h"
#include "ColorConversion.h"

#define UNSET_FRAMERATE -1

ofxBlackmagicGrabber::ofxBlackmagicGrabber() {
    currentOfPixelFormat    = OF_PIXELS_RGB;
    currentOfBmTexFormat    = OF_BM_TEX_FORMAT_RGB;

    grayPixOld              = true;
    rgbPixOld               = true;

    yuvTexOld               = true;
    grayTexOld              = true;
    rgbTexOld               = true;

    // common
    bIsFrameNew             = false;
    bVerbose                = false;
    deviceID                = 0;
    width                   = 0.f;
    height                  = 0.f;
    framerate               = UNSET_FRAMERATE;
    bUseTexture             = true;
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
    if (displayMode == bmdModeUnknown || !controller.selectDevice(deviceID)) {
        ofLogError("ofxBlackmagicGrabber::setDisplayMode") << "display mode "
            "unknown: " << bmdModeUnknown;
        return false;
    }

    int modeIndex;
    if (controller.getDisplayModeIndex(displayMode, modeIndex)) {
        const DisplayModeInfo info = controller.getDisplayModeInfo(modeIndex);
        this->width = info.width;
        this->height = info.height;
    } else {
        ofLogError("ofxBlackmagicGrabber::setDisplayMode") << "display mode "
            "not supported by this device";
    }

    if (!controller.startCaptureWithMode(displayMode)) {
        return false;
    }

    return true;
}

bool ofxBlackmagicGrabber::initGrabber(int w, int h, float _framerate) {
    if (!controller.init()) {
        return false;
    }

    BMDDisplayMode mode = controller.getDisplayMode(w, h, _framerate,
                                                    &framerate);

    return setDisplayMode(mode);
}

bool ofxBlackmagicGrabber::initGrabber(int w, int h) {
    if (!controller.init()) {
        return false;
    }

    if (!controller.selectDevice(deviceID)) {
        return false;
    }

    vector<string> displayModes = controller.getDisplayModeNames();
    ofLogVerbose("ofxBlackmagicGrabber") << "Availabile display modes: " << endl
        << ofToString(displayModes);

    if (framerate == UNSET_FRAMERATE) {
        ofLogNotice("ofxBlackmagicGrabber") << "Framerate not set, using the "
            "highest available for this width and height. Set explicitly with "
            "setDesiredFramerate. ";

        // get the displayMode with highest available framerate
        BMDDisplayMode mode = controller.getDisplayMode(w, h, &framerate);

        return setDisplayMode(mode);
    }

    return initGrabber(w, h, framerate);
}


void ofxBlackmagicGrabber::clearMemory() {
    grayPix.clear();
    rgbPix.clear();

    yuvTex.clear();
    grayTex.clear();
    rgbTex.clear();
}

void ofxBlackmagicGrabber::close() {
    if(controller.isCapturing()) {
        controller.stopCapture();
    }

    clearMemory();
}

void ofxBlackmagicGrabber::update() {
    if(controller.buffer.swapFront()) {
        grayPixOld = true, rgbPixOld = true;
        yuvTexOld = true, grayTexOld = true, rgbTexOld = true;
        bIsFrameNew = true;
    } else {
        bIsFrameNew = false;
    }
}

bool ofxBlackmagicGrabber::isFrameNew() {
    return bIsFrameNew;
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

int ofxBlackmagicGrabber::getDeviceID() {
    return deviceID;
}

void ofxBlackmagicGrabber::setDesiredFrameRate(int _framerate) {
    ofBaseVideoGrabber::setDesiredFrameRate((float)_framerate);
}

void ofxBlackmagicGrabber::setDesiredFrameRate(float _framerate) {
    if (_framerate != framerate) {
        ofLogVerbose("ofxBlackmagicGrabber") << "setDesiredFrameRate(): "
            "to change framerate initGrabber needs to be called now";
    }

    framerate = _framerate;
}

bool ofxBlackmagicGrabber::setPixelFormat(ofPixelFormat pixelFormat) {
    if (pixelFormat != OF_PIXELS_MONO && pixelFormat != OF_PIXELS_RGB) {
        ofLogWarning("ofxBlackmagicGrabber") << "setPixelFormat(): "
            "requested pixel format " << pixelFormat << " not supported";
        return false;
    }

    currentOfPixelFormat = pixelFormat;
    return true;
}

ofPixelFormat ofxBlackmagicGrabber::getPixelFormat() {
    return currentOfPixelFormat;
}

void ofxBlackmagicGrabber::setTextureFormat(ofBmTextureFormat texFormat) {
    currentOfBmTexFormat = texFormat;
}

ofBmTextureFormat ofxBlackmagicGrabber::getTextureFormat() {
    return currentOfBmTexFormat;
}

vector<unsigned char>& ofxBlackmagicGrabber::getRaw() {
    return controller.buffer.getFront();
}

ofPixels& ofxBlackmagicGrabber::getGrayPixels() {
    if(grayPixOld) {
        grayPix.allocate(width, height, OF_IMAGE_GRAYSCALE);
        unsigned int n = width * height;
        cby0cry1_to_y(&(getRaw()[0]), grayPix.getPixels(), n);
        grayPixOld = false;
    }
    return grayPix;
}

ofPixels& ofxBlackmagicGrabber::getRgbPixels() {
    if(rgbPixOld) {
        rgbPix.allocate(width, height, OF_IMAGE_COLOR);
        unsigned int n = width * height;
        cby0cry1_to_rgb(&(getRaw()[0]), rgbPix.getPixels(), n);
        rgbPixOld = false;
    }
    return rgbPix;
}

ofPixels& ofxBlackmagicGrabber::getCurrentPixels() {
    if (currentOfPixelFormat == OF_PIXELS_MONO) {
        return getGrayPixels();
    }

    return getRgbPixels();
}

unsigned char* ofxBlackmagicGrabber::getPixels() {
    return getCurrentPixels().getPixels();
}

ofPixels& ofxBlackmagicGrabber::getPixelsRef() {
    return getCurrentPixels();
}

ofTexture& ofxBlackmagicGrabber::getYuvTexture() {
    if(yuvTexOld) {
        yuvTex.loadData(&(getRaw()[0]), width / 2, height, GL_RGBA);
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

ofTexture& ofxBlackmagicGrabber::getRgbTexture() {
    if(rgbTexOld) {
        rgbTex.loadData(getRgbPixels());
        rgbTexOld = false;
    }
    return rgbTex;
}

ofTexture& ofxBlackmagicGrabber::getCurrentTexture() {
    switch (currentOfBmTexFormat) {
        case OF_BM_TEX_FORMAT_YUV:
            return getYuvTexture();
            break;
        case OF_BM_TEX_FORMAT_GRAY:
            return getGrayTexture();
            break;
        case OF_BM_TEX_FORMAT_RGB:
            return getRgbTexture();
            break;
        default:
            return getRgbTexture();
            break;
    }
}

ofTexture* ofxBlackmagicGrabber::getTexture() {
    return &getCurrentTexture();
}