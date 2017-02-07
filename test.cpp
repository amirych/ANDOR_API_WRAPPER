#include <iostream>

#include "camera/andor_camera.h"
#include "camera/andorsdk_exception.h"

int main()
{
    ANDOR_Camera cam;
    ANDOR_StringFeature ef;

    std::list<ANDOR_CameraInfo> ii = cam.getFoundCameras();
    if ( ii.size() ) {
        std::cout << "Found cameras: \n";
        for ( ANDOR_CameraInfo &inf: ii) {
            std::wcout << "  NAME: " << inf.cameraName.value() << "\n";
            std::wcout << "  MODEL: " << inf.cameraModel.value() << "\n";
            std::wcout << "  SERIAL: " << inf.serialNumber.value() << "\n";
        }
        std::cout << "\n";
    }

    try {
        ef = ANDOR_Camera::SoftwareVersion;
        std::cout << "Software version: " + ef.value_to_string() << std::endl;
        cam.setLogLevel(ANDOR_Camera::LOG_LEVEL_VERBOSE);


        bool ok = cam.connectToCamera(0,&std::cout);
        std::cout << "Is openned: " << ok << "\n";
        if ( ok ) {
            cam["FanSpeed"] = L"Off";
            ef = cam["InterfaceType"];

            std::cout << "InterfaceType = " << ef.value_to_string() << "\n";
        }

        ANDOR_EnumFeatureInfo fi = cam["PixelEncoding"];
        std::vector<andor_string_t> vals = fi.implementedValues();
        std::cout << "PixelEncoding: \n";
        for ( int i = 0; i < vals.size(); ++i ) {
            std::wcout << vals[i] << L"\n";
        }

        cam.disconnectFromCamera();
        return ok;
    } catch (AndorSDK_Exception &ex ) {
        std::cout << "ERROR: " << ex.what() << " ( " << ex.getError() << " )\n";
        cam.disconnectFromCamera();
        return ex.getError();
    }
}
