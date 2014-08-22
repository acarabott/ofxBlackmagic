#pragma once

#include "DeckLinkController.h"

class ofxBlackmagicGrabber : ofBaseVideoGrabber {
private:
    DeckLinkController controller;

    bool grayPixOld, colorPixOld;
    ofPixels yuvPix, grayPix, colorPix;
    bool yuvTexOld, grayTexOld, colorTexOld;
    ofTexture yuvTex, grayTex, colorTex;

    int width, height;

protected:

    bool                    bIsFrameNew;

public:
    ofxBlackmagicGrabber();
    virtual ~ofxBlackmagicGrabber();

    vector<ofVideoDevice>   listDevices();

    bool setup(int width, int height, float framerate);

    void update();
    bool isFrameNew();

    void close();
    void clearMemory();

    float getWidth();
    float getHeight();

    vector<unsigned char>& getYuvRaw(); // fastest
    ofPixels& getGrayPixels(); // fast
    ofPixels& getColorPixels(); // slow

    ofTexture& getYuvTexture(); // fastest
    ofTexture& getGrayTexture(); // fast
    ofTexture& getColorTexture(); // slower

    void drawYuv(); // fastest
    void drawGray(); // fast
    void drawColor(); // slower
};