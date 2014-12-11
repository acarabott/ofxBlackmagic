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
    void                        cacheDevices();
    vector<ofVideoDevice>       listDevices();
    bool                        setDisplayMode(BMDDisplayMode);
    bool                        initGrabber(int w, int h);
    bool                        initGrabber(int w, int h, float framerate);

    void                        setVerbose(bool bTalkToMe);
    void                        setDeviceID(int _deviceID);
    int                         getDeviceID();
    void                        setDesiredFrameRate(int _framerate); // for compatibility
    void                        setDesiredFrameRate(float _framerate);

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

    string                      getDeviceName(int w, int h, float _framerate);

    // void videoSettings(); // not implemented

protected:
    vector<ofVideoDevice>       videoDevices;
    bool                        bIsFrameNew;
    bool                        bVerbose;
    bool                        bUseTexture;
    int                         deviceID;
    float                       width;
    float                       height;
    float                       framerate;

private:
    DeckLinkController          controller;

    BMDDisplayMode              currentDisplayMode;
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