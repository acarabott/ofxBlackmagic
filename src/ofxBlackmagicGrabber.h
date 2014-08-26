#pragma once

#include "DeckLinkController.h"
#include "DisplayModeInfo.h"
#include "ofTypes.h"

enum ofxBlackmagicTexFormat {
    OF_BLACKMAGIC_YUV = 0,
    OF_BLACKMAGIC_GRAY,
    OF_BLACKMAGIC_RGB,
    OF_BLACKMAGIC_RGBA,
    OF_BLACKMAGIC_BGRA
};

class ofxBlackmagicGrabber : public ofBaseVideoGrabber, public ofBaseVideoDraws{
public:
                                    ofxBlackmagicGrabber();
    virtual                         ~ofxBlackmagicGrabber();

    const vector<ofVideoFormat>     listDeviceFormats();
    vector<ofVideoDevice>           listDevices();
    bool                            setDisplayMode(BMDDisplayMode);
    bool                            initGrabber(int w, int h);
    bool                            initGrabber(int w, int h, int framerate);
                                        // e.g. 2997 for 29.97fps
                                        // 30 or 3000 for 30fps

    void                            setVerbose(bool bTalkToMe);
    void                            setDeviceID(int _deviceID);
    void                            setDesiredFrameRate(int _framerate);

    void update();
    bool isFrameNew();

    void close();
    void clearMemory();

    float getWidth();
    float getHeight();

    bool                            setPixelFormat(ofPixelFormat pixelFormat);
    ofPixelFormat                   getPixelFormat();

    vector<unsigned char>& getYuvRaw(); // fastest
    ofPixels& getGrayPixels(); // fast
    ofPixels& getRgbPixels(); // slow
    ofPixels& getRgbaPixels();
    ofPixels& getBgraPixels();
    ofPixels& getCurrentPixels();
    ofPixels&                       getPixelsRef();
    unsigned char* getPixels();

    void                            setTextureFormat(ofxBlackmagicTexFormat tf);
    ofxBlackmagicTexFormat          getTextureFormat();

    ofTexture& getYuvTexture(); // fastest
    ofTexture& getGrayTexture(); // fast
    ofTexture& getRgbTexture(); // slower
    ofTexture& getRgbaTexture();
    ofTexture& getBgraTexture();
    ofTexture& getCurrentTexture();
    ofTexture* getTexture();

    // void videoSettings(); // not implemented

    void draw(float x, float y);
    void draw(float x, float y, float w, float h);

    void setAnchorPercent(float xPct, float yPct);
    void setAnchorPoint(float x, float y);
    void resetAnchor();

    void setUseTexture(bool _bUseTexture); // for compatibility, unused.
    ofTexture& getTextureReference();

protected:
    bool                            bIsFrameNew;
    bool                            bVerbose;
    bool                            bUseTexture;
    int                             deviceID;
    float                           width;
    float                           height;
    int                             framerate;

private:
    DeckLinkController              controller;

    ofPixelFormat                   currentPixelFormat;
    ofPixels&                       currentPixels;

    ofPixels                        yuvPix,
                                    grayPix,
                                    rgbPix,
                                    rgbaPix,
                                    bgraPix;

    ofxBlackmagicTexFormat          currentTexFormat;
    ofTexture&                      currentTexture;

    ofTexture                       yuvTex,
                                    grayTex,
                                    rgbTex,
                                    rgbaTex,
                                    bgraTex;

    bool                            grayPixOld,
                                    rgbPixOld,
                                    rgbaPixOld,
                                    bgraPixOld;

    bool                            yuvTexOld,
                                    grayTexOld,
                                    rgbTexOld,
                                    rgbaTexOld,
                                    bgraTexOld;

};