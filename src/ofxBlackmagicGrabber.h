#pragma once

#include "DeckLinkController.h"
#include "DisplayModeInfo.h"
#include "ofTypes.h"

enum ofBmTextureFormat {
    OF_BM_TEX_FORMAT_YUV = 0,
    OF_BM_TEX_FORMAT_GRAY,
    OF_BM_TEX_FORMAT_RGB
};

class ofxBlackmagicGrabber : public ofBaseVideoGrabber {
public:
                                ofxBlackmagicGrabber();
    virtual                     ~ofxBlackmagicGrabber();

    const vector<ofVideoFormat> listDeviceFormats();
    vector<ofVideoDevice>       listDevices();
    bool                        setDisplayMode(BMDDisplayMode);
    bool                        initGrabber(int w, int h);
    bool                        initGrabber(int w, int h, float framerate);

    void                        setVerbose(bool bTalkToMe);
    void                        setDeviceID(int _deviceID);
    int                         getDeviceID();
    void                        setDesiredFrameRate(int _framerate);

    void                        update();
    bool                        isFrameNew();

    void                        close();
    void                        clearMemory();

    float                       getWidth();
    float                       getHeight();

    bool                        setPixelFormat(ofPixelFormat pixelFormat);
    ofPixelFormat               getPixelFormat();

    void                        setTextureFormat(ofBmTextureFormat texFormat);
    ofBmTextureFormat           getTextureFormat();

    vector<unsigned char>&      getRaw();           // fastest
    ofPixels&                   getGrayPixels();    // fast
    ofPixels&                   getRgbPixels();     // slow
    ofPixels&                   getCurrentPixels();
    ofPixels&                   getPixelsRef();
    unsigned char*              getPixels();

    ofTexture&                  getYuvTexture();    // fastest
    ofTexture&                  getGrayTexture();   // fast
    ofTexture&                  getRgbTexture();    // slower
    ofTexture&                  getCurrentTexture();
    ofTexture*                  getTexture();

    // void videoSettings(); // not implemented

protected:
    bool                        bIsFrameNew;
    bool                        bVerbose;
    bool                        bUseTexture;
    int                         deviceID;
    float                       width;
    float                       height;
    int                         framerate;

private:
    DeckLinkController          controller;

    ofPixelFormat               currentOfPixelFormat;
    ofBmTextureFormat           currentOfBmTexFormat;

    ofPixels                    grayPix,
                                rgbPix;

    ofTexture                   yuvTex,
                                grayTex,
                                rgbTex;

    bool                        grayPixOld,
                                rgbPixOld;

    bool                        yuvTexOld,
                                grayTexOld,
                                rgbTexOld;
};