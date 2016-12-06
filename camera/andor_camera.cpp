#include "andor_camera.h"

                            /*************************************
                             *                                   *
                             * ANDOR_Camera CLASS IMPLEMENTATION *
                             *                                   *
                             *************************************/


                /*  STATIC MEMBERS INITIALIZATION   */

list<ANDOR_CameraInfo> ANDOR_Camera::foundCameras = list<ANDOR_CameraInfo>();
list<int> ANDOR_Camera::openedCameraIndices = list<int>();
size_t ANDOR_Camera::numberOfCreatedObjects = 0;

ANDOR_Camera::ANDOR_Feature ANDOR_Camera::DeviceCount(AT_HANDLE_SYSTEM,L"DeviceCount");
ANDOR_Camera::ANDOR_Feature ANDOR_Camera::SoftwareVersion(AT_HANDLE_SYSTEM,L"SoftwareVersion");


                /*  CONSTRUCTOR AND DESTRUCTOR  */

ANDOR_Camera::ANDOR_Camera():
    logLevel(LOG_LEVEL_ERROR),
    lastError(AT_SUCCESS), cameraLog(nullptr), cameraHndl(AT_HANDLE_SYSTEM),
    CameraPresent(L"CameraPresent"), CameraAcquiring(L"CameraAcquiring"), cameraFeature()
{
//    cameraFeature = 100;
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




                    /*  ANDOR_CameraInfo constructor */

ANDOR_CameraInfo::ANDOR_CameraInfo():
    cameraModel(L"Unknown"), cameraName(L"Unknown"), serialNumber(L"Unknown"),
    controllerID(L"Unknown"), interfaceType(L"Unknown"), device_index(-1)
{
}
