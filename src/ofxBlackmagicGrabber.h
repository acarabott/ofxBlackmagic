#pragma once

#include "DeckLinkController.h"
#include "DisplayModeInfo.h"
#include "ofTypes.h"

class ofxBlackmagicGrabber : public ofBaseVideoGrabber, public ofBaseVideoDraws{
private:
    DeckLinkController controller;

    bool bIsFrameNew;
    bool grayPixOld, colorPixOld;
    ofPixels yuvPix, grayPix, colorPix;
    bool yuvTexOld, grayTexOld, colorTexOld;
    ofTexture yuvTex, grayTex, colorTex;

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

    vector<unsigned char>& getYuvRaw(); // fastest
    ofPixels& getGrayPixels(); // fast
    ofPixels& getColorPixels(); // slow
    ofPixels& getCurrentPixels();
    unsigned char* getPixels();

    ofTexture& getYuvTexture(); // fastest
    ofTexture& getGrayTexture(); // fast
    ofTexture& getColorTexture(); // slower
    ofTexture& getCurrentTexture();
    ofTexture* getTexture();

    // void videoSettings(); // not implemented

    void drawYuv(); // fastest
    void drawGray(); // fast
    void drawColor(); // slower

    void draw(float x, float y);
    void draw(float x, float y, float w, float h);

    void setAnchorPercent(float xPct, float yPct);
    void setAnchorPoint(float x, float y);
    void resetAnchor();

    void setUseTexture(bool _bUseTexture); // for compatibility, unused.
    ofTexture& getTextureReference();

protected:

    bool                            bVerbose;
    bool                            bUseTexture;
    int                             deviceID;
    float                           width;
    float                           height;
    int                             framerate;
};