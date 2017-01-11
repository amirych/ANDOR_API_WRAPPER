#include "andor_camera.h"


                            /*************************************
                             *                                   *
                             * ANDOR_Camera CLASS IMPLEMENTATION *
                             *                                   *
                             *************************************/
extern ANDOR_Camera::AndorFeatureNameMap  DEFAULT_ANDOR_SDK_FEATURES; // pre-defined SDK features "name-type" std::map

                /*  STATIC MEMBERS INITIALIZATION   */

std::list<ANDOR_CameraInfo> ANDOR_Camera::foundCameras = std::list<ANDOR_CameraInfo>();
std::list<int> ANDOR_Camera::openedCameraIndices = std::list<int>();
size_t ANDOR_Camera::numberOfCreatedObjects = 0;

ANDOR_Camera::AndorFeatureNameMap ANDOR_Camera::ANDOR_SDK_FEATURES = ANDOR_Camera::AndorFeatureNameMap(DEFAULT_ANDOR_SDK_FEATURES);

ANDOR_Camera::ANDOR_Feature ANDOR_Camera::DeviceCount(AT_HANDLE_SYSTEM,L"DeviceCount");
ANDOR_Camera::ANDOR_Feature ANDOR_Camera::SoftwareVersion(AT_HANDLE_SYSTEM,L"SoftwareVersion");


                /*  CONSTRUCTOR AND DESTRUCTOR  */

ANDOR_Camera::ANDOR_Camera():
    logLevel(LOG_LEVEL_ERROR),
    lastError(AT_SUCCESS), cameraLog(nullptr), cameraHndl(AT_HANDLE_SYSTEM),
    CameraPresent(L"CameraPresent"), CameraAcquiring(L"CameraAcquiring"), cameraFeature()
{
    // camera handler for "CameraPresent"
    // and "CameraAcquiring" features will be
    // set in connectToCamera method as at this point there is still no camera handle!!!

    if ( !numberOfCreatedObjects ) {
        AT_InitialiseLibrary();
        scanConnectedCameras();
    }

    ANDOR_StringFeature sf,sf1;

    sf1 = (*this)[L"SSS"] = sf = L"EEEE";
    int ind = (*this)[L"SSS"] = sf = L"EEEE";

    ANDOR_StringFeature ss = (*this)[L"SSSS"];
    sf = (*this)[L"SSSS"];

    std::wcout << sf;
}



ANDOR_Camera::~ANDOR_Camera()
{
}


                    /*  PUBLIC METHODS  */


void ANDOR_Camera::setLogLevel(const LOG_LEVEL level)
{
    logLevel = level;
}


ANDOR_Camera::LOG_LEVEL ANDOR_Camera::getLogLevel() const
{
    return logLevel;
}



                    /*  PROTECTED METHODS  */

int ANDOR_Camera::scanConnectedCameras()
{
    int N = ANDOR_Camera::DeviceCount;

    AT_H hndl;

    if ( foundCameras.size() ) foundCameras.clear();

    // suppose device indices are continuous sequence from 0 to (DeviceCount-1)
    for (int i = 0; i < N-1; ++i ) {
        int err = AT_Open(i,&hndl);
        if ( err == AT_SUCCESS ) {
            ANDOR_CameraInfo info;
            ANDOR_Feature feature(hndl,L"CameraModel");

//            info.cameraModel = (std::wstring)feature;
            info.cameraModel = feature;

            feature.setName(L"CameraName");
            info.cameraName = feature;

            feature.setName(L"SerialNumber");
            info.serialNumber = feature;

            feature.setName(L"ControllerID");
            info.controllerID = feature;

            feature.setName(L"SensorWidth");
            info.sensorWidth = feature;

            feature.setName(L"SensorHeight");
            info.sensorHeight = feature;

            feature.setName(L"PixelWidth");
            info.pixelWidth = feature;

            feature.setName(L"PixelHeight");
            info.pixelHeight = feature;

            feature.setName(L"InterfaceType");
            info.interfaceType = feature;

            info.device_index = i;

            foundCameras.push_back(info);
        }
        AT_Close(hndl);
    }

    return N;
}


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::operator [](const std::wstring &feature_name)
{
    auto it = ANDOR_SDK_FEATURES.find(feature_name); // check for valid feature name

    if ( it != ANDOR_SDK_FEATURES.end() ) {
        cameraFeature.setType(it->second);
        cameraFeature.setName(feature_name);
        return cameraFeature;
    }

    cameraFeature.setType(UnknownType);
    throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Unknown ANDOR SDK feature!");
}


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::operator [](const wchar_t* feature_name)
{
    return operator [](std::wstring(feature_name));
}


/*  STATIC PUBLIC METHODS  */

/*  STATIC PROTECTED METHODS  */



                    /*  ANDOR_CameraInfo constructor */

ANDOR_CameraInfo::ANDOR_CameraInfo():
    cameraModel(L"Unknown"), cameraName(L"Unknown"), serialNumber(L"Unknown"),
    controllerID(L"Unknown"), interfaceType(L"Unknown"), device_index(-1)
{
}



                /*  ANDOR SDK STRING AND ENUMERATED FEATURE CLASSES IMPLEMENTATION  */

ANDOR_StringFeature::ANDOR_StringFeature(): std::wstring()
{
}

ANDOR_StringFeature::ANDOR_StringFeature(ANDOR_Camera::ANDOR_Feature &feature):
    std::wstring(feature.operator std::wstring())
{

}





ANDOR_EnumFeature::ANDOR_EnumFeature():
    std::wstring(), _index(-1)
{

}


ANDOR_EnumFeature::ANDOR_EnumFeature(ANDOR_Camera::ANDOR_Feature &feature):
    std::wstring(feature.operator std::wstring())
{
    _index = feature.at_index;
}


