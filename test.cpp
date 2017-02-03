#include <iostream>

#include "camera/andor_camera.h"
#include "camera/andorsdk_exception.h"

int main()
{
    ANDOR_Camera cam;
    try {
        cam.setLogLevel(ANDOR_Camera::LOG_LEVEL_VERBOSE);


        bool ok = cam.connectToCamera(0,&std::cout);
        std::cout << "Is openned: " << ok << "\n";
        if ( ok ) {
            cam["FanSpeed"] = L"Off";
            ANDOR_StringFeature ef = cam["InterfaceType"];

            std::cout << "InterfaceType = " << ef.value_to_string() << "\n";
        }

        cam.disconnectFromCamera();
        return ok;
    } catch (AndorSDK_Exception &ex ) {
        std::cout << "ERROR: " << ex.what() << " ( " << ex.getError() << " )\n";
        cam.disconnectFromCamera();
        return ex.getError();
    }
}
