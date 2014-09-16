
#include "DeckLinkController.h"

DeckLinkController::DeckLinkController()
: selectedDevice(NULL), deckLinkInput(NULL), supportFormatDetection(false), currentlyCapturing(false)  {}

DeckLinkController::~DeckLinkController()  {
	vector<IDeckLink*>::iterator it;

	// Release the IDeckLink list
	for(it = deviceList.begin(); it != deviceList.end(); it++) {
		(*it)->Release();
	}
}

bool DeckLinkController::init()  {
	IDeckLinkIterator* deckLinkIterator = NULL;
	IDeckLink* deckLink = NULL;
	bool result = false;

	// Create an iterator
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL) {
		ofLogError("DeckLinkController") << "Please install the Blackmagic Desktop Video drivers to use the features of this application.";
		goto bail;
	}

	// List all DeckLink devices
	while (deckLinkIterator->Next(&deckLink) == S_OK) {
		// Add device to the device list
		deviceList.push_back(deckLink);
	}

	if (deviceList.size() == 0) {
		ofLogError("DeckLinkController") << "You will not be able to use the features of this application until a Blackmagic device is installed.";
		goto bail;
	}

	result = true;

bail:
	if (deckLinkIterator != NULL) {
		deckLinkIterator->Release();
		deckLinkIterator = NULL;
	}

	return result;
}


int DeckLinkController::getDeviceCount()  {
	return deviceList.size();
}

vector<string> DeckLinkController::getDeviceNameList()  {
	vector<string> nameList;
	int deviceIndex = 0;

	while (deviceIndex < deviceList.size()) {
		CFStringRef cfStrName;

		// Get the name of this device
		if (deviceList[deviceIndex]->GetDisplayName(&cfStrName) == S_OK) {
			nameList.push_back(string(CFStringGetCStringPtr(cfStrName, kCFStringEncodingMacRoman)));
			CFRelease(cfStrName);
		}
		else {
			nameList.push_back("DeckLink");
		}

		deviceIndex++;
	}

	return nameList;
}


bool DeckLinkController::selectDevice(int index)  {
	IDeckLinkAttributes* deckLinkAttributes = NULL;
	IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
	IDeckLinkDisplayMode* displayMode = NULL;
	bool result = false;

	// Check index
	if (index >= deviceList.size()) {
		ofLogError("DeckLinkController") << "This application was unable to select the device.";
		goto bail;
	}

	// A new device has been selected.
	// Release the previous selected device and mode list
	if (deckLinkInput != NULL)
		deckLinkInput->Release();

	while(modeList.size() > 0) {
		modeList.back()->Release();
		modeList.pop_back();
	}


	// Get the IDeckLinkInput for the selected device
	if ((deviceList[index]->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)) {
		ofLogError("DeckLinkController") << "This application was unable to obtain IDeckLinkInput for the selected device.";
		deckLinkInput = NULL;
		goto bail;
	}

	//
	// Retrieve and cache mode list
	if (deckLinkInput->GetDisplayModeIterator(&displayModeIterator) == S_OK) {
		while (displayModeIterator->Next(&displayMode) == S_OK)
			modeList.push_back(displayMode);

		displayModeIterator->Release();
	}

	//
	// Check if input mode detection format is supported.

	supportFormatDetection = false; // assume unsupported until told otherwise
	if (deviceList[index]->QueryInterface(IID_IDeckLinkAttributes, (void**) &deckLinkAttributes) == S_OK) {
		if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportFormatDetection) != S_OK)
			supportFormatDetection = false;

		deckLinkAttributes->Release();
	}

	result = true;

bail:
	return result;
}

bool DeckLinkController::getDisplayModeIndex(BMDDisplayMode displayMode, int& result) {
	int i = 0;
	bool found = false;

	while (i < modeList.size() && !found) {
	    if (modeList[i]->GetDisplayMode() == displayMode) {
	        result = i;
	        found = true;
	    }
        ++i;
	}

	return found;
}

const DisplayModeInfo DeckLinkController::getDisplayModeInfo(int modeIndex) {
	DisplayModeInfo info;

	// name
	CFStringRef modeName;
	if (modeList[modeIndex]->GetName(&modeName) == S_OK) {
		info.name = string(CFStringGetCStringPtr(modeName, kCFStringEncodingMacRoman));
		CFRelease(modeName);
	} else {
		info.name = "Unknown mode";
	}

	// dimensions
	info.width = modeList[modeIndex]->GetWidth();
	info.height = modeList[modeIndex]->GetHeight();

	// FPS
	BMDTimeValue numerator;
	BMDTimeScale denominator;

	if (modeList[modeIndex]->GetFrameRate(&numerator, &denominator) == S_OK) {
		info.framerate = numerator / denominator;
	} else {
		ofLogError("DeckLinkController") << "Couldn't read frame rate from"
		<< " it may still work but has been set to 0";
		info.framerate = 0;
	}

	return info;
}

const vector<DisplayModeInfo> DeckLinkController::getDisplayModeInfoList() {
	vector<DisplayModeInfo> modeInfos;

	typedef vector<IDeckLinkDisplayMode*>::size_type vec_mode_sz;
	for (vec_mode_sz modeIndex = 0; modeIndex < modeList.size(); modeIndex++) {
		modeInfos.push_back(getDisplayModeInfo(modeIndex));
	}

    return modeInfos;
}

const vector<string> DeckLinkController::getDisplayModeNames()  {
	vector<string> modeNames;
	vector<DisplayModeInfo> modeInfos = getDisplayModeInfoList();

    typedef vector<DisplayModeInfo>::size_type vec_dm_sz;
	for (vec_dm_sz modeIndex = 0; modeIndex < modeInfos.size(); modeIndex++) {
		modeNames.push_back(modeInfos[modeIndex].name);
	}

	return modeNames;
}

bool DeckLinkController::isFormatDetectionEnabled()  {
	return supportFormatDetection;
}

bool DeckLinkController::isCapturing()  {
	return currentlyCapturing;
}

unsigned long DeckLinkController::getDisplayModeBufferSize(BMDDisplayMode mode) {

	if(mode == bmdModeNTSC2398
			|| mode == bmdModeNTSC
			|| mode == bmdModeNTSCp) {
		return 720 * 486 * 2;
	} else if( mode == bmdModePAL
			|| mode == bmdModePALp) {
		return 720 * 576 * 2;
	} else if( mode == bmdModeHD720p50
			|| mode == bmdModeHD720p5994
			|| mode == bmdModeHD720p60) {
		return 1280 * 720 * 2;
	} else if( mode == bmdModeHD1080p2398
			|| mode == bmdModeHD1080p24
			|| mode == bmdModeHD1080p25
			|| mode == bmdModeHD1080p2997
			|| mode == bmdModeHD1080p30
			|| mode == bmdModeHD1080i50
			|| mode == bmdModeHD1080i5994
			|| mode == bmdModeHD1080i6000
			|| mode == bmdModeHD1080p50
			|| mode == bmdModeHD1080p5994
			|| mode == bmdModeHD1080p6000) {
		return 1920 * 1080 * 2;
	} else if( mode == bmdMode2k2398
			|| mode == bmdMode2k24
			|| mode == bmdMode2k25) {
		return 2048 * 1556 * 2;
	} else if( mode == bmdMode2kDCI2398
			|| mode == bmdMode2kDCI24
			|| mode == bmdMode2kDCI25) {
		return 2048 * 1080 * 2;
	} else if( mode == bmdMode4K2160p2398
			|| mode == bmdMode4K2160p24
			|| mode == bmdMode4K2160p25
			|| mode == bmdMode4K2160p2997
			|| mode == bmdMode4K2160p30) {
		return 3840 * 2160 * 2;
	} else if( mode == bmdMode4kDCI2398
			|| mode == bmdMode4kDCI24
			|| mode == bmdMode4kDCI25) {
		return 4096 * 2160 * 2;
	}

	return 0;
}

bool DeckLinkController::startCaptureWithIndex(int videoModeIndex)  {
	// Get the IDeckLinkDisplayMode from the given index
	if ((videoModeIndex < 0) || (videoModeIndex >= modeList.size())) {
		ofLogError("DeckLinkController") << "An invalid display mode was selected.";
		return false;
	}

	return startCaptureWithMode(modeList[videoModeIndex]->GetDisplayMode());
}

bool DeckLinkController::startCaptureWithMode(BMDDisplayMode displayMode) {
	unsigned long bufferSize = getDisplayModeBufferSize(displayMode);
	if (bufferSize == 0) {
		ofLogError("DeckLinkController") << "Invalid display mode";
	    return false;
	}
	vector<unsigned char> prototype(bufferSize);
	buffer.setup(prototype);

	BMDVideoInputFlags videoInputFlags;

	// Enable input video mode detection if the device supports it
	videoInputFlags = supportFormatDetection ? bmdVideoInputEnableFormatDetection : bmdVideoInputFlagDefault;

	// Set capture callback
	deckLinkInput->SetCallback(this);

	// Set the video input mode
	if (deckLinkInput->EnableVideoInput(displayMode, bmdFormat8BitYUV, videoInputFlags) != S_OK) {
		ofLogError("DeckLinkController") << "This application was unable to select the chosen video mode. Perhaps, the selected device is currently in-use.";
		return false;
	}

	// Start the capture
	if (deckLinkInput->StartStreams() != S_OK) {
		ofLogError("DeckLinkController") << "This application was unable to start the capture. Perhaps, the selected device is currently in-use.";
		return false;
	}

	currentlyCapturing = true;

	return true;
}

void DeckLinkController::stopCapture()  {
	// Stop the capture
	deckLinkInput->StopStreams();

	// Delete capture callback
	deckLinkInput->SetCallback(NULL);

	currentlyCapturing = false;
}


HRESULT DeckLinkController::VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags)  {
	bool shouldRestartCaptureWithNewVideoMode = true;

	// Restart capture with the new video mode if told to
	if (shouldRestartCaptureWithNewVideoMode) {
		// Stop the capture
		deckLinkInput->StopStreams();

		// Set the video input mode
		if (deckLinkInput->EnableVideoInput(newMode->GetDisplayMode(), bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection) != S_OK) {
			ofLogError("DeckLinkController") << "This application was unable to select the new video mode.";
			goto bail;
		}

		// Start the capture
		if (deckLinkInput->StartStreams() != S_OK) {
			ofLogError("DeckLinkController") << "This application was unable to start the capture on the selected device.";
			goto bail;
		}
	}

bail:
	return S_OK;
}

typedef struct {
	// VITC timecodes and user bits for field 1 & 2
	string vitcF1Timecode;
	string vitcF1UserBits;
	string vitcF2Timecode;
	string vitcF2UserBits;

	// RP188 timecodes and user bits (VITC1, VITC2 and LTC)
	string rp188vitc1Timecode;
	string rp188vitc1UserBits;
	string rp188vitc2Timecode;
	string rp188vitc2UserBits;
	string rp188ltcTimecode;
	string rp188ltcUserBits;
} AncillaryDataStruct;

HRESULT DeckLinkController::VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket)  {
//	bool hasValidInputSource = (videoFrame->GetFlags() & bmdFrameHasNoInputSource) != 0;

//	AncillaryDataStruct ancillaryData;

	// Get the various timecodes and userbits for this frame
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITC, ancillaryData.vitcF1Timecode, ancillaryData.vitcF1UserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITCField2, ancillaryData.vitcF2Timecode, ancillaryData.vitcF2UserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC1, ancillaryData.rp188vitc1Timecode, ancillaryData.rp188vitc1UserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188LTC, ancillaryData.rp188ltcTimecode, ancillaryData.rp188ltcUserBits);
//	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC2, ancillaryData.rp188vitc2Timecode, ancillaryData.rp188vitc2UserBits);

	void* bytes;
	videoFrame->GetBytes(&bytes);
	unsigned char* raw = (unsigned char*) bytes;
	vector<unsigned char>& back = buffer.getBack();
	back.assign(raw, raw + back.size());
	buffer.swapBack();

	return S_OK;
}

void DeckLinkController::getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat, string& timecodeString, string& userBitsString)  {
	IDeckLinkTimecode* timecode = NULL;
	CFStringRef timecodeCFString;
	BMDTimecodeUserBits userBits = 0;

	if ((videoFrame != NULL)
		&& (videoFrame->GetTimecode(timecodeFormat, &timecode) == S_OK)) {
		if (timecode->GetString(&timecodeCFString) == S_OK) {
			timecodeString = string(CFStringGetCStringPtr(timecodeCFString, kCFStringEncodingMacRoman));
			CFRelease(timecodeCFString);
		}
		else {
			timecodeString = "";
		}

		timecode->GetTimecodeUserBits(&userBits);
		userBitsString = "0x" + ofToHex(userBits);

		timecode->Release();
	}
	else {
		timecodeString = "";
		userBitsString = "";
	}


}

// returns the *index* of the closest matching framerate from an array of rates
// returning the closest match (as a float) doesn't allow trustworthy comparison
int DeckLinkController::getMatchingFramerateIndex(float input, float* rates, int n) {
    bool found = false;
    float bestDiff = 0.f;
    int bestRate = 0;

    int i = 0;
    while (!found && i < n) {
        // this accounts for inputs that may have suffered from integer truncation
        // e.g. 23.98 becoming 23
        if (input == floor(rates[i])) {
            bestRate = i;
            found = true;
        } else {
            float value = rates[i];
            float diff = abs(value - input);

            if (i == 0 || diff < bestDiff) {
                bestDiff = diff;
                bestRate = i;
            }
        }
        ++i;
    }

    return bestRate;
}

// picks the mode with matching resolution, with highest available framerate
// and a preference for progressive over interlaced
BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h,
                                                  float* framerateResult)
{
    BMDDisplayMode result = bmdModeUnknown;
    float framerate = -1;
    if (w == 720 && h == 486) {				// NTSC
        framerate = 59.94;
        result = bmdModeNTSCp;
    } else if (w == 720 && h == 576) {		// PAL
        framerate = 50.f;
        result = bmdModePALp;
    } else if (w == 1280 && h == 720) {		// HD 720
        framerate = 59.94;
        result = bmdModeHD720p60;
    } else if (w == 1920 && h == 1080) {	// HD 1080
        framerate = 60.f;
        result = bmdModeHD1080p6000;
    } else if (w == 2048 && h == 1556) {	// 2k
        framerate = 25.f;
        result = bmdMode2k25;
    } else if (w == 2048 && h == 1080) {	// 2k DCI
        framerate = 25.f;
        result = bmdMode2kDCI25;
    } else if (w == 3840 && h == 2160) {	// 4K
        framerate = 30.f;
        result = bmdMode4K2160p30;
    } else if (w == 4096 && h == 2160) {	// 4k DCI
        framerate = 25.f;
        result = bmdMode4kDCI25;
    }

    if (framerateResult != NULL) {
        *framerateResult = framerate;
    }

    if (result == bmdModeUnknown) {
        ofLogError("DeckLinkController") << "resolution not supported";
    }

    return result;
}

BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h, float framerate,
                                                  float* framerateResult)
{
    BMDDisplayMode result = bmdModeUnknown;
    float fr = framerate;

	if (w == 720 && h == 486) {									// NTSC
        static const int n = 3;
        float r[n] = { 23.98, 29.97, 59.94 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdModeNTSC2398;
                break;
            case 1:
                result = bmdModeNTSC;
                break;
            case 2:
                result = bmdModeNTSCp;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
	} else if (w == 720 && h == 576) {							// PAL
        static const int n = 2;
        float r[n] = { 25, 50 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdModePAL;
                break;
            case 1:
                result = bmdModePALp;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
    } else if (w == 1280 && h == 720) {							// HD 720
        static const int n = 3;
        float r[n] = { 50, 59.94, 60 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdModeHD720p50;
                break;
            case 1:
                result = bmdModeHD720p5994;
                break;
            case 2:
                result = bmdModeHD720p60;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
    } else if (w == 1920 && h == 1080) {						// HD 1080
        static const int n = 11;
        float r[n] = { 23.98, 24, 25, 29.97, 30, 50, 59.94, 60, 50, 59.94, 60 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdModeHD1080p2398;
                break;
            case 1:
                result = bmdModeHD1080p24;
                break;
            case 2:
                result = bmdModeHD1080p25;
                break;
            case 3:
                result = bmdModeHD1080p2997;
                break;
            case 4:
                result = bmdModeHD1080p30;
                break;
            case 5:
                result = bmdModeHD1080i50;
                break;
            case 6:
                result = bmdModeHD1080i5994;
                break;
            case 7:
                result = bmdModeHD1080i6000;
                break;
            case 8:
                result = bmdModeHD1080p50;
                break;
            case 9:
                result = bmdModeHD1080p5994;
                break;
            case 10:
                result = bmdModeHD1080p6000;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
    } else if (w == 2048 && h == 1556) {						// 2k
        static const int n = 3;
        float r[n] = { 23.98, 24, 25 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdMode2k2398;
                break;
            case 1:
                result = bmdMode2k24;
                break;
            case 2:
                result = bmdMode2k25;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
    } else if (w == 2048 && h == 1080) {						// 2k DCI
        static const int n = 3;
        float r[n] = { 23.98, 24, 25 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdMode2kDCI2398;
                break;
            case 1:
                result = bmdMode2kDCI24;
                break;
            case 2:
                result = bmdMode2kDCI25;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
    } else if (w == 3840 && h == 2160) {                        // 4K
        static const int n = 5;
        float r[n] = { 23.98, 24, 25, 29.97, 30 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdMode4K2160p2398;
                break;
            case 1:
                result = bmdMode4K2160p24;
                break;
            case 2:
                result = bmdMode4K2160p25;
                break;
            case 3:
                result = bmdMode4K2160p2997;
                break;
            case 4:
                result = bmdMode4K2160p30;
                break;
            default:
            break;
        }
	} else if (w == 4096 && h == 2160) {						// 4k DCI
        static const int n = 3;
        float r[n] = { 23.98, 24, 25 };
        int i = getMatchingFramerateIndex(framerate, r, n);
        switch (i) {
            case 0:
                result = bmdMode4kDCI2398;
                break;
            case 1:
                result = bmdMode4kDCI24;
                break;
            case 2:
                result = bmdMode4kDCI25;
                break;
            default:
                result = bmdModeUnknown;
                break;
        }
        fr = r[i];
    }

    if (framerateResult != NULL) {
        *framerateResult = fr;
    }

    if (result == bmdModeUnknown) {
        ofLogError("DeckLinkController") << "resolution not supported";
    }

	return result;
}
