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


static inline AT_U8* allocate_aligned(const size_t buff_size)
{
#ifdef _MSC_VER
#if _MSC_VER > 1800
        alignas(8) AT_U8* pbuff = new unsigned char[buff_size];
#else // compile with VS 2013
        __declspec(align(8))  AT_U8* pbuff = new unsigned char[buff_size];
#endif
#else
        alignas(8) AT_U8* pbuff = new unsigned char[buff_size];
#endif
    return pbuff;
}


static inline std::string pointer_to_str(void* ptr)
{
    char addr[20];
#ifdef _MSC_VER
    int n = _snprintf_s(addr,20,"%p",ptr);
#else
    int n = snprintf(addr,20,"%p",ptr);
#endif
    return std::string(addr);
}


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


            /* feature callback function  */

static int AT_EXP_CONV feature_callback(AT_H hndl, const AT_WC* name, void* context)
{
    if ( context == nullptr ) return AT_ERR_INVALIDHANDLE; // just check (should not be NULL!!!)

    CallbackContext* _context = (CallbackContext*)context;

    int ret_code = _context->func(andor_string_t(name),_context->user_context);

    return ret_code;
}


                /*  STATIC MEMBERS INITIALIZATION   */

std::list<ANDOR_CameraInfo> ANDOR_Camera::foundCameras = std::list<ANDOR_CameraInfo>();
std::list<int> ANDOR_Camera::openedCameraIndices = std::list<int>();
size_t ANDOR_Camera::numberOfCreatedObjects = 0;

//ANDOR_Camera::AndorFeatureNameMap ANDOR_Camera::ANDOR_SDK_FEATURES = ANDOR_Camera::AndorFeatureNameMap(DEFAULT_ANDOR_SDK_FEATURES);

ANDOR_Camera::ANDOR_Feature ANDOR_Camera::DeviceCount(AT_HANDLE_SYSTEM,L"DeviceCount");
ANDOR_Camera::ANDOR_Feature ANDOR_Camera::SoftwareVersion(AT_HANDLE_SYSTEM,L"SoftwareVersion");


                /*  CONSTRUCTOR AND DESTRUCTOR  */

ANDOR_Camera::ANDOR_Camera():
    logLevel(LOG_LEVEL_ERROR),
    lastError(AT_SUCCESS), cameraLog(nullptr), cameraHndl(AT_HANDLE_UNINITIALISED),
    cameraFeature(),
    waitBufferThread(),
//    imageBufferAddr(nullptr), imageBuffersNumber(0),
    imageBufferAddr(),
    maxBuffersNumber(ANDOR_CAMERA_DEFAULT_MAX_BUFFERS_NUMBER), requestedBuffersNumber(0),
    callbackContextPtr(),
//    DeviceCount(), SoftwareVersion(),
    ANDOR_SDK_FEATURES(DEFAULT_ANDOR_SDK_FEATURES)
{
    // camera handler for "CameraPresent"
    // and "CameraAcquiring" features will be
    // set in connectToCamera method as at this point there is still no camera handle!!!

    if ( !numberOfCreatedObjects ) {
        AT_InitialiseLibrary();
//        scanConnectedCameras();
    }

    setLogLevel(logLevel); // needed to initialize or disable extra logging facility

//    DeviceCount.setDeviceHndl(AT_HANDLE_SYSTEM);
//    DeviceCount.setName(L"DeviceCount");

//    SoftwareVersion.setDeviceHndl(AT_HANDLE_SYSTEM);
//    SoftwareVersion.setName(L"Softwareversion");

    ANDOR_Camera::DeviceCount.setType(ANDOR_Camera::IntType);
    ANDOR_Camera::SoftwareVersion.setType(ANDOR_Camera::StringType);

    ++numberOfCreatedObjects;
}



ANDOR_Camera::~ANDOR_Camera()
{
    --numberOfCreatedObjects;

    if ( !numberOfCreatedObjects ) {
        AT_FinaliseLibrary();
    }

//    deleteImageBuffers();

    // delete callback helper structures if it exist
    if ( callbackContextPtr.size() ) {
        for ( auto it = callbackContextPtr.begin(); it != callbackContextPtr.end(); ++it ) delete *it;
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
    } else { // reset logging function to null (no logging from SDK function calling)
        cameraFeature.setLoggingFunc();
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
            logToFile(ANDOR_Camera::CAMERA_INFO, DeviceCount.getLastLogMessage());
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


        log_str = "Connection established! Camera handler is " + std::to_string(cameraHndl);
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str);

        // initialize feature (set working camera handler)
        cameraFeature.setDeviceHndl(cameraHndl);

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

    // here, I ignore possible errors from access to SDK features!!!
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

    if ( cameraHndl == AT_HANDLE_UNINITIALISED ) return;

    logToFile(ANDOR_Camera::CAMERA_INFO, "Try to disconnect from camera ...");


    if ( logLevel == LOG_LEVEL_VERBOSE ) {
        log_str = "AT_Close(" + std::to_string(cameraHndl) + ")";
        logToFile(ANDOR_Camera::CAMERA_INFO,log_str);
    }

    lastError = AT_Close(cameraHndl);

    logToFile(ANDOR_Camera::CAMERA_INFO,"Camera disconnected");

    cameraHndl = AT_HANDLE_UNINITIALISED;
}



void ANDOR_Camera::setMaxBuffersNumber(const size_t num)
{
    maxBuffersNumber = num;
}


size_t ANDOR_Camera::getMaxBuffersNumber() const
{
    return maxBuffersNumber;
}


void ANDOR_Camera::registerFeatureCallback(andor_string_t feature_name, const callback_func_t &func, void *context)
{
    std::string log_str;

    if ( cameraHndl == AT_HANDLE_UNINITIALISED ) {
        throw AndorSDK_Exception(AT_ERR_CONNECTION, "Cannot register feature callback function! No connection to device!");
    }

    CallbackContext *_context = new CallbackContext();
    callbackContextPtr.push_back(_context);

    _context->func = func;
    _context->user_context = context;

    std::wstring_convert<std::codecvt_utf8<AT_WC>> cvt;
    log_str = "Try to register '" + cvt.to_bytes(feature_name) +  "'-feature callback function ...";
    logToFile(ANDOR_Camera::CAMERA_INFO, log_str);

    // get string presentation of user callback function address
    auto ff = *func.target<int (*)(andor_string_t, void*)>();
    std::string sptr = pointer_to_str((void*)ff);
//    char addr[20];
//#ifdef _MSC_VER
//    int n = _snprintf_s(addr,20,"%p",ff);
//#else
//    int n = snprintf(addr,20,"%p",ff);
//#endif

    log_str = "AT_RegisterFeatureCallback(" + std::to_string(cameraHndl) + ", L'" + cvt.to_bytes(feature_name).c_str() +
            "', " + sptr;
//            "', " + addr;

//#ifdef _MSC_VER
//    n = _snprintf_s(addr,20,"%p",context);
//#else
//    n = snprintf(addr,20,"%p",context);
//#endif
//    log_str += std::string(", ") + addr + ")";

    log_str += std::string(", ") + pointer_to_str(context) + ")";

    if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str);

    andor_sdk_assert( lastError = AT_RegisterFeatureCallback(cameraHndl,feature_name.c_str(),feature_callback,(void*)_context),
                      log_str);

    logToFile(ANDOR_Camera::CAMERA_INFO, "The callback function was registered successfully!");

}


void ANDOR_Camera::unregisterFeatureCallback(andor_string_t feature_name, const callback_func_t &func, void *context)
{
    std::string log_str;

    if ( cameraHndl == AT_HANDLE_UNINITIALISED ) { // not sure it is a good idea!!!
        return;
    }

    std::wstring_convert<std::codecvt_utf8<AT_WC>> cvt;
    log_str = "Unregistering '" + cvt.to_bytes(feature_name) +  "'-feature callback function ...";
    logToFile(ANDOR_Camera::CAMERA_INFO, log_str);

    // get string presentation of user callback function address
    auto ff = *func.target<int (*)(andor_string_t, void*)>();
    std::string sptr = pointer_to_str((void*)ff);
//    char addr[20];
//#ifdef _MSC_VER
//    int n = _snprintf_s(addr,20,"%p",ff);
//#else
//    int n = snprintf(addr,20,"%p",ff);
//#endif

    log_str = "AT_UnregisterFeatureCallback(" + std::to_string(cameraHndl) + ", L'" + cvt.to_bytes(feature_name).c_str() +
            "', " + sptr;
//            "', " + addr;

//#ifdef _MSC_VER
//    n = _snprintf_s(addr,20,"%p",context);
//#else
//    n = snprintf(addr,20,"%p",context);
//#endif
//    log_str += std::string(", ") + addr + ")";
    log_str += std::string(", ") + pointer_to_str(context) + ")";

    if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str);

    andor_sdk_assert( lastError = AT_UnregisterFeatureCallback(cameraHndl,feature_name.c_str(),feature_callback,context), log_str);

    logToFile(ANDOR_Camera::CAMERA_INFO, "The callback function was unregistered successfully!");
}


#ifdef _MSC_VER
#if _MSC_VER > 1800
int ANDOR_Camera::waitBuffer(AT_U8 **ptr, int *ptr_size, unsigned int timeout) noexcept
#else
int ANDOR_Camera::waitBuffer(AT_U8 **ptr, int *ptr_size, unsigned int timeout)
#endif
#else
int ANDOR_Camera::waitBuffer(AT_U8 **ptr, int *ptr_size, unsigned int timeout) noexcept
#endif
{
    std::string log_str;

    log_str = "AT_WaitBuffer(" + std::to_string(cameraHndl) + ", **ptr, *ptr_size, " + std::to_string(timeout);
    if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str);

    int ret_code = AT_WaitBuffer(cameraHndl, ptr, ptr_size, timeout);
//    andor_sdk_assert( ret_code, log_str);

    log_str = "returns: *ptr = " + pointer_to_str(*ptr) + ", ptr_size = " + std::to_string(*ptr_size);
    if ( logLevel == LOG_LEVEL_VERBOSE ) logToFile(CAMERA_INFO,log_str,1);

    return ret_code;
}


void ANDOR_Camera::queueBuffer(AT_U8 *ptr, int ptr_size)
{
    std::string log_msg = "AT_QueueBuffer(" + std::to_string(cameraHndl) + ", " + pointer_to_str(ptr) +
                          ", " + std::to_string(ptr_size) + ")";
    if ( logLevel == ANDOR_Camera::LOG_LEVEL_VERBOSE ) logToFile(ANDOR_Camera::CAMERA_INFO, log_msg);

    andor_sdk_assert( lastError =  AT_QueueBuffer(cameraHndl, ptr, ptr_size), log_msg);
}


void ANDOR_Camera::flush()
{
    std::string log_str = "AT_Flush(" + std::to_string(cameraHndl) + ")";
    if ( logLevel == ANDOR_Camera::LOG_LEVEL_VERBOSE ) logToFile(ANDOR_Camera::CAMERA_INFO,log_str);

    andor_sdk_assert( lastError = AT_Flush(cameraHndl), log_str );
}


void ANDOR_Camera::logToFile(const LOG_IDENTIFICATOR ident, const std::string &log_str, const int identation)
{
    if ( !cameraLog ) return;
    if ( logLevel == ANDOR_Camera::LOG_LEVEL_QUIET ) return;


    std::stringstream str;

    if ( ident == ANDOR_Camera::BLANK ) {
        *cameraLog << log_str << std::endl << std::flush;
        return;
    }

    str << "[" << time_stamp() << "] ";

    switch (ident) {
    case ANDOR_Camera::CAMERA_INFO:
        str << "[CAMERA INFO";
        if ( cameraHndl != AT_HANDLE_UNINITIALISED ) { // camera already connected, add device handler identificator
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
        if ( cameraHndl != AT_HANDLE_UNINITIALISED ) { // camera already connected, add device handler identificator
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

    *cameraLog << str.str() << std::endl << std::flush;
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



void ANDOR_Camera::operator ()(const andor_string_t & command_name)
{
    andor_string_t str = L"AT_Command(" + std::to_wstring(cameraHndl) +
                         L", " + command_name + L")";

    std::wstring_convert<std::codecvt_utf8<AT_WC>> cnv;
    std::string log_str = cnv.to_bytes(str);

    if ( logLevel == ANDOR_Camera::LOG_LEVEL_VERBOSE ) logToFile(ANDOR_Camera::CAMERA_INFO,log_str);
    andor_sdk_assert( AT_Command(cameraHndl,command_name.c_str()), log_str);
}


void ANDOR_Camera::operator ()(const AT_WC* command_name)
{
    operator ()(andor_string_t(command_name));
}


void ANDOR_Camera::operator ()(const std::string & command_name)
{
    std::wstring_convert<std::codecvt_utf8<AT_WC>> cvt;

    operator ()(cvt.from_bytes(command_name));
}


void ANDOR_Camera::operator ()(const char* command_name)
{
    operator ()(std::string(command_name));
}




                    /*  PROTECTED METHODS  */

int ANDOR_Camera::scanConnectedCameras()
{
    int N = ANDOR_Camera::DeviceCount;

    AT_H hndl;

    ANDOR_FeatureInfo f_info;

    std::list<andor_string_t> f_list = {L"CameraModel", L"CameraName", L"ControllerID",
                                        L"SensorWidth",L"SensorHeight",
                                        L"PixelWidth", L"PixelHeight",
                                        L"InterfaceType"};

    if ( foundCameras.size() ) foundCameras.clear();

    // suppose device indices are continuous sequence from 0 to (DeviceCount-1)
    for (int i = 0; i < N-1; ++i ) {
        int err = AT_Open(i,&hndl);
        if ( err == AT_SUCCESS ) {
            ANDOR_CameraInfo info;
            ANDOR_Feature feature(hndl,L"CameraModel");

            f_info = feature;
            if ( f_info.isImplemented() ) info.cameraModel = feature;

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


void ANDOR_Camera::allocateImageBuffers(int imageSizeBytes)
{
    std::string log_msg;

    if ( requestedBuffersNumber == 0 ) {
        log_msg = "Cannot allocate image buffers! The requested number of images is 0!";
        throw AndorSDK_Exception(AT_ERR_NOMEMORY, log_msg);
    }

//    imageBuffersNumber = ( requestedBuffersNumber > maxBuffersNumber ) ? maxBuffersNumber : requestedBuffersNumber;
    size_t imageBuffersNumber = ( requestedBuffersNumber > maxBuffersNumber ) ? maxBuffersNumber : requestedBuffersNumber;

//    imageBufferAddr = std::unique_ptr<unsigned char*>(new unsigned char*[imageBuffersNumber]);

    // try to optimize allocation mechanism: real allocation only if it is needed
    if ( imageBufferAddr.size() != imageBuffersNumber ) {
        size_t old_size = imageBufferAddr.size();
        imageBufferAddr.resize(imageBuffersNumber);
        if ( imageBufferSize != imageSizeBytes ) {
            for ( size_t i = 0; i < imageBufferAddr.size(); ++i ) {
                imageBufferAddr[i] = std::unique_ptr<AT_U8[]>(allocate_aligned(imageSizeBytes));
            }
        } else {
            for ( size_t i = old_size; i < imageBufferAddr.size(); ++i ) {
                imageBufferAddr[i] = std::unique_ptr<AT_U8[]>(allocate_aligned(imageSizeBytes));
            }
        }
    } else {
        if ( imageBufferSize != imageSizeBytes ) {
            for ( size_t i = 0; i < imageBufferAddr.size(); ++i ) {
                imageBufferAddr[i] = std::unique_ptr<AT_U8[]>(allocate_aligned(imageSizeBytes));
            }
        }
    }

    imageBufferSize = imageSizeBytes;

//    unsigned char** buff_ptr = imageBufferAddr.get();

//    // init to nullptr
//    for ( size_t i = 0; i < imageBuffersNumber; ++i ) {
//        buff_ptr[i] = nullptr;
//    }

//    // allocate memory for image buffers (use of alignment required by SDK)
////    char addr[20];
//    for ( size_t i = 0; i < imageBuffersNumber; ++i ) {
//#ifdef _MSC_VER
//#if _MSC_VER > 1800
//        alignas(8) unsigned char* pbuff = new unsigned char[imageSizeBytes];
//#else // compile with VS 2013
//        __declspec(align(8))  unsigned char* pbuff = new unsigned char[imageSizeBytes];
//#endif
//#else
//        alignas(8) unsigned char* pbuff = new unsigned char[imageSizeBytes];
//#endif

//        buff_ptr[i] = pbuff;

//        queueBuffer(buff_ptr[i], imageSizeBytes);
//    }
}


//void ANDOR_Camera::deleteImageBuffers()
//{
//    if ( !imageBufferAddr ) return;

//    std::string log_str;

//    flush();

//    unsigned char** buff_ptr = imageBufferAddr.get();

//    for ( size_t i = 0; i < imageBuffersNumber; ++i ) {
//        if ( logLevel == ANDOR_Camera::LOG_LEVEL_VERBOSE ) {
//            log_str = std::string("Delete image buffer at address: ") + pointer_to_str(buff_ptr[i]);
//            logToFile(ANDOR_Camera::CAMERA_INFO,log_str,1);
//        }
//        delete[] buff_ptr[i];
//    }
//}


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



