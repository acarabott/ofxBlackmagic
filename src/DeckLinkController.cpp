
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
BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h) {

    if (w == 720 && h == 486) {				// NTSC
        return bmdModeNTSCp;
    } else if (w == 720 && h == 576) {		// PAL
        return bmdModePALp;
    } else if (w == 1280 && h == 720) {		// HD 720
        return bmdModeHD720p60;
    } else if (w == 1920 && h == 1080) {	// HD 1080
        return bmdModeHD1080p6000;
    } else if (w == 2048 && h == 1556) {	// 2k
        return bmdMode2k25;
    } else if (w == 2048 && h == 1080) {	// 2k DCI
        return bmdMode2kDCI25;
    } else if (w == 3840 && h == 2160) {	// 4K
        return bmdMode4K2160p30;
    } else if (w == 4096 && h == 2160) {	// 4k DCI
        return bmdMode4kDCI25;
    }
    
    return bmdModeUnknown;
}

BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h, float framerate) {
	if (w == 720 && h == 486) {									// NTSC
        float r[3] = { 23.98, 29.97, 59.94 };
        switch (getMatchingFramerateIndex(framerate, r, 3)) {
            case 0:
                return bmdModeNTSC2398;
                break;
            case 1:
                return bmdModeNTSC;
                break;
            case 2:
                return bmdModeNTSCp;
                break;
            default:
                break;
        }
	} else if (w == 720 && h == 576) {							// PAL
        float r[2] = { 25, 50 };
        switch (getMatchingFramerateIndex(framerate, r, 2)) {
            case 0:
                return bmdModePAL;
                break;
            case 1:
                return bmdModePALp;
                break;
            default:
                break;
        }
	} else if (w == 1280 && h == 720) {							// HD 720
        float r[3] = { 50, 59.94, 60 };
        switch (getMatchingFramerateIndex(framerate, r, 3)) {
            case 0:
                return bmdModeHD720p50;
                break;
            case 1:
                return bmdModeHD720p5994;
                break;
            case 2:
                return bmdModeHD720p60;
                break;
            default:
                break;
        }
	} else if (w == 1920 && h == 1080) {						// HD 1080
        float r[11] = { 23.98, 24, 25, 29.97, 30, 50, 59.94, 60, 50, 59.94, 60 };
        switch (getMatchingFramerateIndex(framerate, r, 11)) {
            case 0:
                return bmdModeHD1080p2398;
                break;
            case 1:
                return bmdModeHD1080p24;
                break;
            case 2:
                return bmdModeHD1080p25;
                break;
            case 3:
                return bmdModeHD1080p2997;
                break;
            case 4:
                return bmdModeHD1080p30;
                break;
            case 5:
                return bmdModeHD1080i50;
                break;
            case 6:
                return bmdModeHD1080i5994;
                break;
            case 7:
                return bmdModeHD1080i6000;
                break;
            case 8:
                return bmdModeHD1080p50;
                break;
            case 9:
                return bmdModeHD1080p5994;
                break;
            case 10:
                return bmdModeHD1080p6000;
                break;
            default:
                break;
        }
	} else if (w == 2048 && h == 1556) {						// 2k
        float r[3] = { 23.98, 24, 25 };
        switch (getMatchingFramerateIndex(framerate, r, 3)) {
            case 0:
                return bmdMode2k2398;
                break;
            case 1:
                return bmdMode2k24;
                break;
            case 2:
                return bmdMode2k25;
                break;
            default:
                break;
        }
	} else if (w == 2048 && h == 1080) {						// 2k DCI
        float r[3] = { 23.98, 24, 25 };
        switch (getMatchingFramerateIndex(framerate, r, 3)) {
            case 0:
                return bmdMode2kDCI2398;
                break;
            case 1:
                return bmdMode2kDCI24;
                break;
            case 2:
                return bmdMode2kDCI25;
                break;
            default:
                break;
        }
    } else if (w == 3840 && h == 2160) {                        // 4K
        float r[5] = { 23.98, 24, 25, 29.97, 30 };
        switch (getMatchingFramerateIndex(framerate, r, 5)) {
            case 0:
                return bmdMode4K2160p2398;
                break;
            case 1:
                return bmdMode4K2160p24;
                break;
            case 2:
                return bmdMode4K2160p25;
                break;
            case 3:
                return bmdMode4K2160p2997;
                break;
            case 4:
                return bmdMode4K2160p30;
                break;
            default:
            break;
        }
	} else if (w == 4096 && h == 2160) {						// 4k DCI
        float r[3] = { 23.98, 24, 25 };
        switch (getMatchingFramerateIndex(framerate, r, 3)) {
            case 0:
                return bmdMode4kDCI2398;
                break;
            case 1:
                return bmdMode4kDCI24;
                break;
            case 2:
                return bmdMode4kDCI25;
                break;
            default:
                break;
        }
	}

    ofLogError("DeckLinkController") << "resolution not supported";

	return bmdModeUnknown;
}
