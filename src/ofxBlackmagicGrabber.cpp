#include "ofxBlackmagicGrabber.h"
#include "ColorConversion.h"

#define UNSET_FRAMERATE -1

ofxBlackmagicGrabber::ofxBlackmagicGrabber()
: currentPixels(bgraPix),
  currentTexture(bgraTex) {

    grayPixOld          = true;
    rgbPixOld           = true;
    rgbaPixOld          = true;
    bgraPixOld          = true;

    yuvTexOld           = true;
    grayTexOld          = true;
    rgbTexOld           = true;
    rgbaTexOld          = true;
    bgraTexOld          = true;

    // common
    bIsFrameNew         = false;
    bVerbose            = false;
    deviceID            = 0;
    width               = 0.f;
    height              = 0.f;
    framerate           = UNSET_FRAMERATE;
    bUseTexture         = true;
    bUsingDefaultTexMode = true;

    currentOfPixelFormat    = OF_PIXELS_BGRA;
    currentTexFormat        = OF_BLACKMAGIC_BGRA;
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

bool ofxBlackmagicGrabber::setDisplayMode(BMDDisplayMode displayMode,
                                          BMDPixelFormat pixelFormat) {

    if (displayMode == bmdModeUnknown || !controller.selectDevice(deviceID)) {
        return false;
    }

    if (!controller.startCaptureWithMode(displayMode, pixelFormat)) {
        return false;
    }

    this->width = controller.getDisplayModeInfo(deviceID).width;
    this->height = controller.getDisplayModeInfo(deviceID).height;

    return true;
}

BMDPixelFormat ofxBlackmagicGrabber::getBmPixelFormat(ofxBlackmagicTexFormat
                                                      texFormat) {
    switch (texFormat) {
        case OF_BLACKMAGIC_YUV:
            return bmdFormat8BitYUV;
            break;
        case OF_BLACKMAGIC_GRAY:
            return bmdFormat8BitYUV;
            break;
        case OF_BLACKMAGIC_RGB:
            // TODO: depends on quickest conversion: yuv or rgba to rgb
            return bmdFormat8BitYUV;
            break;
        case OF_BLACKMAGIC_RGBA:
            return bmdFormat8BitBGRA;
            break;
        case OF_BLACKMAGIC_BGRA:
            return bmdFormat8BitBGRA;
            break;
        default:
            break;
    }
}

bool ofxBlackmagicGrabber::initGrabber(int w, int h, int _framerate,
                                       ofxBlackmagicTexFormat texFormat) {
    if (!controller.init()) {
        return false;
    }

    setTextureFormat(texFormat);
    framerate = _framerate;
    BMDDisplayMode displayMode = controller.getDisplayMode(w, h, framerate);
    BMDPixelFormat pixelFormat = getBmPixelFormat(texFormat);

    return setDisplayMode(displayMode, pixelFormat);
}

bool ofxBlackmagicGrabber::initGrabber(int w, int h) {
    if (!controller.init()) {
        return false;
    }

    if (bUsingDefaultTexMode) {
        ofLogNotice("ofxBlackmagicGrabber") << "Using BGRA by default. If you "
            "want YUV, more efficient grayscale, or less efficient (but more "
            "convenient) RGB or RGBA, then call setTextureFormat beforehand";
    }

    vector<string> displayModes = controller.getDisplayModeNames();
    ofLogVerbose("ofxBlackmagicGrabber") << "Availabile display modes: " << endl
        << ofToString(displayModes);

    if (!controller.selectDevice(deviceID)) {
        return false;
    }

    if (framerate == UNSET_FRAMERATE) {
        ofLogNotice("ofxBlackmagicGrabber") << "Framerate not set, using the "
            "highest available for this width and height. Set explicitly with "
            "setDesiredFramerate. ";
        
        // get the displayMode with highest available framerate
        BMDDisplayMode displayMode = controller.getDisplayMode(w, h);
        BMDPixelFormat pixelFormat = getBmPixelFormat(currentTexFormat);

        return setDisplayMode(displayMode, pixelFormat);
    }

    return initGrabber(w, h, framerate, currentTexFormat);
}


void ofxBlackmagicGrabber::clearMemory() {
    yuvPix.clear();
    grayPix.clear();
    rgbPix.clear();
    rgbaPix.clear();
    bgraPix.clear();

    yuvTex.clear();
    grayTex.clear();
    rgbTex.clear();
    rgbaTex.clear();
    bgraTex.clear();
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
    ofLogVerbose("ofxBlackmagicGrabber") << "setDesiredFrameRate(): "
        "to change framerate initGrabber needs to be called now";

    framerate = _framerate;
}

bool ofxBlackmagicGrabber::setPixelFormat(ofPixelFormat pixelFormat) {
    if (pixelFormat != OF_PIXELS_MONO
        && pixelFormat != OF_PIXELS_RGB
        && pixelFormat != OF_PIXELS_RGBA
        && pixelFormat != OF_PIXELS_BGRA)
    {
        ofLogWarning("ofxBlackmagicGrabber") << "setPixelFormat(): "
            "requested pixel format " << pixelFormat << " not supported";
        return false;
    }

    switch (pixelFormat) {
        case OF_PIXELS_MONO:
            currentPixels = getGrayPixels();
            break;
        case OF_PIXELS_RGB:
            currentPixels = getRgbPixels();
            break;
        case OF_PIXELS_RGBA:
            currentPixels = getRgbaPixels();
            break;
        case OF_PIXELS_BGRA:
            currentPixels = getBgraPixels();
            break;
        default:
            return false;
            break;
    }


    currentOfPixelFormat = pixelFormat;
    return true;
}

ofPixelFormat ofxBlackmagicGrabber::getPixelFormat() {
    return currentOfPixelFormat;
}

vector<unsigned char>& ofxBlackmagicGrabber::getRaw() {
    return controller.buffer.getFront();
}

ofPixels& ofxBlackmagicGrabber::getGrayPixels() {
    if(grayPixOld) {
        // TODO if/else on current pixel mode
        grayPix.allocate(width, height, OF_IMAGE_GRAYSCALE);
        unsigned int n = width * height;
        cby0cry1_to_y(&(getRaw()[0]), grayPix.getPixels(), n);
        grayPixOld = false;
    }
    return grayPix;
}

ofPixels& ofxBlackmagicGrabber::getRgbPixels() {
    if(rgbPixOld) {
        // TODO if/else on currentPixelMode
        rgbPix.allocate(width, height, OF_IMAGE_COLOR);
        unsigned int n = width * height;
        cby0cry1_to_rgb(&(getRaw()[0]), rgbPix.getPixels(), n);
        rgbPixOld = false;
    }
    return rgbPix;
}

ofPixels& ofxBlackmagicGrabber::getRgbaPixels() {
    if(rgbaPixOld) {
        // TODO get from currentPixelMode
        rgbaPix.allocate(width, height, OF_IMAGE_COLOR);
        unsigned int n = width * height;
        cby0cry1_to_rgb(&(getRaw()[0]), rgbaPix.getPixels(), n);
        rgbaPixOld = false;
    }
    return rgbaPix;
}

ofPixels& ofxBlackmagicGrabber::getBgraPixels() {
    if(bgraPixOld) {
        // TODO get from currentPixelMode
        bgraPix.allocate(width, height, OF_IMAGE_COLOR);
        unsigned int n = width * height;
        cby0cry1_to_rgb(&(getRaw()[0]), bgraPix.getPixels(), n);
        bgraPixOld = false;
    }
    return bgraPix;
}

ofPixels& ofxBlackmagicGrabber::getCurrentPixels() {
    return currentPixels;
}

unsigned char* ofxBlackmagicGrabber::getPixels() {
    return getCurrentPixels().getPixels();
}

ofPixels& ofxBlackmagicGrabber::getPixelsRef() {
    return getCurrentPixels();
}

void ofxBlackmagicGrabber::setTextureFormat(ofxBlackmagicTexFormat texFormat) {
    currentTexFormat = texFormat;
    bUsingDefaultTexMode = false;

    switch (texFormat) {
        case OF_BLACKMAGIC_YUV:
            currentTexture = getYuvTexture();
            break;
        case OF_BLACKMAGIC_GRAY:
            currentTexture = getGrayTexture();
            break;
        case OF_BLACKMAGIC_RGB:
            currentTexture = getRgbTexture();
            break;
        case OF_BLACKMAGIC_RGBA:
            currentTexture = getRgbaTexture();
            break;
        case OF_BLACKMAGIC_BGRA:
            currentTexture = getBgraTexture();
            break;
        default:
            break;
    }
}

ofxBlackmagicTexFormat ofxBlackmagicGrabber::getTextureFormat() {
    return currentTexFormat;
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

ofTexture& ofxBlackmagicGrabber::getRgbaTexture() {
    if(rgbaTexOld) {
        rgbaTex.loadData(getRgbaPixels());
        rgbaTexOld = false;
    }
    return rgbaTex;
}

ofTexture& ofxBlackmagicGrabber::getBgraTexture() {
    if(bgraTexOld) {
        bgraTex.loadData(getBgraPixels());
        bgraTexOld = false;
    }
    return bgraTex;
}

ofTexture& ofxBlackmagicGrabber::getCurrentTexture() {
    return getCurrentTexture();
}

ofTexture* ofxBlackmagicGrabber::getTexture() {
    return &getCurrentTexture();
}

void ofxBlackmagicGrabber::draw(float x, float y) {
    getCurrentTexture().draw(x, y);
}

void ofxBlackmagicGrabber::draw(float x, float y, float w, float h) {
    getCurrentTexture().draw(x, y, w, h);
}

void ofxBlackmagicGrabber::setAnchorPercent(float xPct, float yPct) {
    getYuvTexture().setAnchorPercent(xPct, yPct);
    getGrayTexture().setAnchorPercent(xPct, yPct);
    getRgbTexture().setAnchorPercent(xPct, yPct);
    getRgbaTexture().setAnchorPercent(xPct, yPct);
    getBgraTexture().setAnchorPercent(xPct, yPct);
}

void ofxBlackmagicGrabber::setAnchorPoint(float x, float y) {
    getYuvTexture().setAnchorPoint(x, y);
    getGrayTexture().setAnchorPoint(x, y);
    getRgbTexture().setAnchorPoint(x, y);
    getRgbaTexture().setAnchorPoint(x, y);
    getBgraTexture().setAnchorPoint(x, y);
}

void ofxBlackmagicGrabber::resetAnchor() {
    getYuvTexture().resetAnchor();
    getGrayTexture().resetAnchor();
    getRgbTexture().resetAnchor();
    getRgbaTexture().resetAnchor();
    getBgraTexture().resetAnchor();
}

void ofxBlackmagicGrabber::setUseTexture(bool _bUseTexture) {
    bUseTexture = _bUseTexture;
}

ofTexture& ofxBlackmagicGrabber::getTextureReference() {
    return getCurrentTexture();
}