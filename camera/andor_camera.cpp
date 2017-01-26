#include "andor_camera.h"

#include <locale>
#include <codecvt>
#include <chrono>
#include <ctime>


                            /*************************************
                             *                                   *
                             * ANDOR_Camera CLASS IMPLEMENTATION *
                             *                                   *
                             *************************************/

extern ANDOR_Camera::AndorFeatureNameMap  DEFAULT_ANDOR_SDK_FEATURES; // pre-defined SDK features "name-type" std::map


                /*  AUXILIARY NON-MEMBER FUNCTIONS  */

static std::string time_stamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    char time_stamp[100];

    struct std::tm buff;
    buff = *std::localtime(&now_c);

    std::strftime(time_stamp,sizeof(time_stamp),"%c",&buff);

    return std::string(time_stamp);
}


            /* CameraPresent feature callback function  */

static int AT_EXP_CONV disconnection_callback(AT_H hndl, AT_WC* name, void* context)
{
    if ( context == nullptr ) return AT_ERR_INVALIDHANDLE; // just check (should not be NULL!!!)

    ANDOR_Camera *camera = (ANDOR_Camera*)context;
    if ( camera == nullptr ) return AT_ERR_INVALIDHANDLE; // just check

    camera->disconnectFromCamera();
}



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
    CameraPresent(L"CameraPresent"), CameraAcquiring(L"CameraAcquiring"), cameraFeature(),
    waitBufferThread()
{
    // camera handler for "CameraPresent"
    // and "CameraAcquiring" features will be
    // set in connectToCamera method as at this point there is still no camera handle!!!

    if ( !numberOfCreatedObjects ) {
        AT_InitialiseLibrary();
        scanConnectedCameras();
    }

    setLogLevel(logLevel); // needed to initialize or disable extra logging facility

    ++numberOfCreatedObjects;
}



ANDOR_Camera::~ANDOR_Camera()
{
    --numberOfCreatedObjects;

    if ( !numberOfCreatedObjects ) {
        AT_FinaliseLibrary();
    }
}


                    /*  PUBLIC METHODS  */


void ANDOR_Camera::setLogLevel(const LOG_LEVEL level)
{
    if ( logLevel == LOG_LEVEL_VERBOSE ) { // set extra logging function (logging from SDK function calling)
        log_func_t log_func = std::bind(
           static_cast<void(ANDOR_Camera::*)(const ANDOR_Camera::LOG_IDENTIFICATOR, const std::string&, const int)>
           (&ANDOR_Camera::logToFile), this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
                                    );
        cameraFeature.setLoggingFunc(log_func);
        CameraPresent.setLoggingFunc(log_func);
        CameraAcquiring.setLoggingFunc(log_func);
    } else { // reset logging function to null (no logging from SDK function calling)
        cameraFeature.setLoggingFunc();
        CameraPresent.setLoggingFunc();
        CameraAcquiring.setLoggingFunc();
    }

    std::string log_str = "Set logging level to ";

    switch ( logLevel ) {
        case LOG_LEVEL_VERBOSE:
            log_str += "'VERBOSE'";
            break;
        case LOG_LEVEL_ERROR:
            log_str += "'ERROR'";
            break;
        case LOG_LEVEL_QUIET:
            log_str += "'QUIET'";
            break;
        default:
            log_str += "'UNKNOWN'";
            break;
    }
    log_str += " state!";

    if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str);

    logLevel = level;
}


ANDOR_Camera::LOG_LEVEL ANDOR_Camera::getLogLevel() const
{
    return logLevel;
}


bool ANDOR_Camera::connectToCamera(const int device_index, std::ostream *log_file)
{
    disconnectFromCamera();

    cameraLog = log_file;

    // header of log
    for (int i = 0; i < 5; ++i) logToFile(ANDOR_Camera::BLANK,"");
    std::string log_str;
    log_str.resize(80,'*');
    logToFile(ANDOR_Camera::BLANK,log_str);
    logToFile(ANDOR_Camera::BLANK,"                 " + time_stamp());
    logToFile(ANDOR_Camera::BLANK,log_str);

    logToFile(ANDOR_Camera::CAMERA_INFO,"Try to open a connection to Andor camera ... ");

    bool ok = true;
    lastError = AT_SUCCESS;

    try {
        int dev_num = ANDOR_Camera::DeviceCount;
        if ( logLevel == ANDOR_Camera::LOG_LEVEL_VERBOSE ) {

        }

        log_str = "Number of found cameras: " + std::to_string(dev_num);
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str,1);

        if ( !dev_num ) {
            logToFile(ANDOR_Camera::CAMERA_ERROR,"No one camera was detected!!! Can not initiate the connection!!!");
            lastError = AT_ERR_CONNECTION;
            return false;
        }

        log_str = "initiate connection to device with index " + std::to_string(device_index) + " ...";
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str,1);

        log_str = "AT_Open(" + std::to_string(device_index) + ", &cameraHndl)";
        if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str);

        andor_sdk_assert( AT_Open(device_index,&cameraHndl), log_str);

        CameraPresent.setDeviceHndl(cameraHndl);
        ok = CameraPresent;
        if ( !ok ) { // it is very strange!!!
            throw AndorSDK_Exception(AT_ERR_CONNECTION,"Connection lost!");
        }

        log_str = "Connection established! Camera handler is " + std::to_string(cameraHndl);
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str);

        // initialize features (set working camera handler)
        CameraAcquiring.setDeviceHndl(cameraHndl);
        cameraFeature.setDeviceHndl(cameraHndl);

        logToFile(ANDOR_Camera::CAMERA_INFO,"Try to register 'CameraPresent'-feature callback function ...");

        log_str = "AT_RegisterFeatureCallback(" + std::to_string(cameraHndl) + ", L'CameraPresent', " +
                  "(FeatureCallback)disconnection_callback,(void*)this";
        if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str);

        andor_sdk_assert( AT_RegisterFeatureCallback(cameraHndl,L"CameraPresent",(FeatureCallback)disconnection_callback,(void*)this),
                          log_str);

        logToFile(ANDOR_Camera::CAMERA_INFO, "The callback function was registered successfully!");


        return true;

    } catch ( AndorSDK_Exception &ex) {
        logToFile(ex);
        lastError = ex.getError();
        return false;
    }
}


bool ANDOR_Camera::connectToCamera(const ANDOR_Camera::CAMERA_IDENT_TAG ident_tag,
                                   const andor_string_t &tag_str, std::ostream *log_file)
{

    std::string log_str = "Ask connection to camera by ";
    switch (ident_tag) {
        case ANDOR_Camera::CameraModel:
            log_str += "'CameraModel'";
            break;
        case ANDOR_Camera::CameraName:
            log_str += "'CameraName'";
            break;
        case ANDOR_Camera::SerialNumber:
            log_str += "'SerialNumber'";
            break;
        case ANDOR_Camera::ControllerID:
            log_str += "'ControllerID'";
            break;
        default:
            log_str += "'unknown identifier' tag!!! Cannot open a camera!!!";
            logToFile(ANDOR_Camera::CAMERA_ERROR,log_str);
            return false; // unknown type!
    }

    log_str += " tag ...";
    logToFile(ANDOR_Camera::CAMERA_INFO,log_str);

    int ok;

    for ( ANDOR_CameraInfo info: foundCameras ) {
        switch (ident_tag) {
            case ANDOR_Camera::CameraModel:
//                ok = info.cameraModel.compare(tag_str);
                ok = info.cameraModel.value().compare(tag_str);
                break;
            case ANDOR_Camera::CameraName:
                ok = info.cameraName.value().compare(tag_str);
                break;
            case ANDOR_Camera::SerialNumber:
                ok = info.serialNumber.value().compare(tag_str);
                break;
            case ANDOR_Camera::ControllerID:
                ok = info.controllerID.value().compare(tag_str);
                break;
        }

        if ( !ok ) { // coincidence was found
            log_str = "The identificator was found! The camera device index is " + std::to_string(info.device_index);
            logToFile(ANDOR_Camera::CAMERA_INFO,log_str);
            return connectToCamera(info.device_index, log_file);
        }
    }

    // coincidence was not found
    std::wstring_convert<std::codecvt_utf8<AT_WC>> cvt;

    log_str = "The camera with indentificator '" +  cvt.to_bytes(tag_str) +
              "' was not found! Can not open a camera connection!";
    logToFile(ANDOR_Camera::CAMERA_ERROR,log_str);

    return false;
}


void ANDOR_Camera::disconnectFromCamera() // here there is no SDK error processing!!!
{
    std::string log_str;

    logToFile(ANDOR_Camera::CAMERA_INFO, "Try to disconnect from camera ...");

    if ( CameraAcquiring ) {

    }

    logToFile(ANDOR_Camera::CAMERA_INFO, "Unregistering 'CameraPresent'-feature callback function",1);

    if ( logLevel == LOG_LEVEL_VERBOSE ) {
        log_str = "AT_UnregisterFeatureCallback(" + std::to_string(cameraHndl) + ", L'CameraPresent', " +
                  "(FeatureCallback)disconnection_callback), nullptr)";
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str);
    }

    AT_UnregisterFeatureCallback(cameraHndl,L"CameraPresent",(FeatureCallback)disconnection_callback,nullptr);


    if ( logLevel == LOG_LEVEL_VERBOSE ) {
        log_str = "AT_Close(" + std::to_string(cameraHndl) + ")";
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str);
    }

    AT_Close(cameraHndl);

    logToFile(ANDOR_Camera::CAMERA_INFO,"Camera disconnected");
}



void ANDOR_Camera::logToFile(const LOG_IDENTIFICATOR ident, const std::string &log_str, const int identation)
{
    if ( !cameraLog ) return;
    if ( logLevel == ANDOR_Camera::LOG_LEVEL_QUIET ) return;


    std::stringstream str;

    str << "[" << time_stamp() << "] ";

    switch (ident) {
    case ANDOR_Camera::CAMERA_INFO:
        str << "[CAMERA INFO";
        if ( cameraHndl != AT_HANDLE_SYSTEM ) { // camera already connected, add device handler identificator
            str << ", DEVICE HANDLER " << cameraHndl << "]";
        } else {
            str << "]";
        }
        break;
    case ANDOR_Camera::SDK_ERROR:
        str << "[ANDOR SDK ERROR, DEVICE HANDLER " << cameraHndl << "]";
        break;
    case ANDOR_Camera::CAMERA_ERROR:
        str << "[CAMERA ERROR";
        if ( cameraHndl != AT_HANDLE_SYSTEM ) { // camera already connected, add device handler identificator
            str << ", DEVICE HANDLER " << cameraHndl << "]";
        } else {
            str << "]";
        }
        break;
    default: // just ignore
        break;
    }

    std::string tab;
    if ( identation > 0 ) {
        tab.resize(ANDOR_CAMERA_LOG_IDENTATION*identation,' ');
    }

    str << " " << tab << log_str;

    *cameraLog << str.str() << std::flush;
}


void ANDOR_Camera::logToFile(const LOG_IDENTIFICATOR ident, const char *log_str, const int identation)
{
    logToFile(ident,std::string(log_str),identation);
}


void ANDOR_Camera::logToFile(const AndorSDK_Exception &ex, const int identation)
{
    std::stringstream str;
    str << ex.what() << " [ANDOR SDK ERROR CODE: " << ex.getError() << "]";

    logToFile(ANDOR_Camera::SDK_ERROR, str.str(), identation);
}



                    /*  PUBLIC OPERATORS[]  */

ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::operator [](const andor_string_t &feature_name)
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


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::operator [](const AT_WC* feature_name)
{
    return operator [](andor_string_t(feature_name));
}



ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::operator [](const std::string & feature_name)
{
    std::wstring_convert<std::codecvt_utf8<AT_WC>> cvt;

    return operator [](cvt.from_bytes(feature_name));
}


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::operator [](const char* feature_name)
{
    std::wstring_convert<std::codecvt_utf8<AT_WC>> cvt;

    return operator [](cvt.from_bytes(feature_name));
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




void ANDOR_Camera::logToFile(const ANDOR_Feature &feature, const int identation)
{
    logToFile(ANDOR_Camera::CAMERA_INFO, feature.getLastLogMessage(), identation);
}




                            /*  STATIC PUBLIC METHODS  */

                            /*  STATIC PROTECTED METHODS  */



                    /*  ANDOR_CameraInfo constructor */

ANDOR_CameraInfo::ANDOR_CameraInfo():
//  cameraModel(L"Unknown"), cameraName(L"Unknown"), serialNumber(L"Unknown"),
//  controllerID(L"Unknown"), interfaceType(L"Unknown")
    cameraModel(), cameraName(), serialNumber(),
    controllerID(), interfaceType(), device_index(-1),
    sensorWidth(0), sensorHeight(0), pixelWidth(0), pixelHeight(0)
{
}



