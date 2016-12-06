#ifndef ANDOR_CAMERA_H
#define ANDOR_CAMERA_H

#include "../export_decl.h"
#include "andorsdk_exception.h"

#include <atcore.h>

#include <string>
#include <list>
#include <iostream>

using namespace std;

typedef int andor_enum_index_t; // type for ANDOR SDK enumerated feature index


        /*  ANDOR camera info structure  */

struct ANDOR_API_WRAPPER_EXPORT ANDOR_CameraInfo
{
    ANDOR_CameraInfo();

    std::wstring cameraModel;
    std::wstring cameraName;
    std::wstring serialNumber;
    std::wstring controllerID;
    AT_64 sensorWidth;
    AT_64 sensorHeight;
    double pixelWidth; // in micrometers
    double pixelHeight;
    std::wstring interfaceType;
    int device_index;

    enum ANDOR_CameraInfoType {CameraModel, CameraName, SerialNumber, ControllerID};
};


                    /*  ANDOR CAMERA CLASS DECLARATION  */

class ANDOR_API_WRAPPER_EXPORT ANDOR_Camera
{
public:
    enum LOG_IDENTIFICATOR {CAMERA_INFO, SDK_ERROR, CAMERA_ERROR};
    enum LOG_LEVEL {LOG_LEVEL_VERBOSE, LOG_LEVEL_ERROR, LOG_LEVEL_QUIET};

    explicit ANDOR_Camera();
    virtual ~ANDOR_Camera();


protected:

                /*   DECLARATION OF A PROXY CLASS TO ACCESS ANDOR SDK FEATURES  */

    class ANDOR_Feature {
    public:
        ANDOR_Feature();
        ANDOR_Feature(const AT_H device_hndl, const std::wstring &name = std::wstring());
        ANDOR_Feature(const AT_H device_hndl, const wchar_t* name = nullptr);

        ANDOR_Feature(const std::wstring &name);
        ANDOR_Feature(const wchar_t* name);

//        ~ANDOR_Feature();

        void setName(const std::wstring &name);
        void setName(const wchar_t* name);

        void setDeviceHndl(const AT_H hndl);
        AT_H getDeviceHndl() const;

        std::string getLastLogMessage() const;

                // get feature value operators

//        template<typename T> operator T();

        template<typename T, typename =
        typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
//        typename std::enable_if<(std::is_integral<T>::value && !std::is_same<T,bool>::value ), T>::type>
        operator T();

        explicit operator std::wstring();
//        operator std::wstring();
//        operator AT_64();

                // set feature value operators

        template<typename T>
        typename std::enable_if<std::is_floating_point<T>::value, T&>::type
        operator=(const T &value);

        template<typename T>
        typename std::enable_if<(std::is_integral<T>::value && !std::is_same<T,bool>::value ), T&>::type
        operator=(const T &value);

        bool & operator=(const bool &value);

        std::wstring & operator=(const std::wstring & value);

    private:
        AT_H deviceHndl;
        AT_WC* featureName;
        std::wstring featureNameStr;
        union {
            AT_64 at_val64;  // integer feature
            AT_BOOL at_bool; // boolean feature
            double at_float; // floating-point feature
            andor_enum_index_t at_index; // index of enumerated feature
        };
        std::wstring at_string; // enumerated or string feature

        std::string logMessage;
    };

                /*  END OF ANDOR_Feature CLASS DECLARATION  */


public:

    void setLogLevel(const LOG_LEVEL level);
    LOG_LEVEL getLogLevel() const;

            /* operator[] for accessing Andor SDK features (const and non-const versions) */

    ANDOR_Feature& operator[](const std::wstring &feature_name);
    ANDOR_Feature& operator[](const wchar_t* feature_name);

            /* Andor SDK global features */

    static ANDOR_Feature DeviceCount;
    static ANDOR_Feature SoftwareVersion;


            /* "CameraPresent" and "CameraAcquiring" feature declarations */

    ANDOR_Feature CameraPresent;
    ANDOR_Feature CameraAcquiring;


protected:
    LOG_LEVEL logLevel;

    static list<ANDOR_CameraInfo> foundCameras;
    static list<int> openedCameraIndices;
    static size_t numberOfCreatedObjects;

    AT_H cameraHndl;
    int cameraIndex;

    int lastError;

    std::ostream *cameraLog;

    ANDOR_Feature cameraFeature;

    void logToFile(LOG_IDENTIFICATOR ident, std::string &log_str, int identation = 0);
    void logToFile(const AndorSDK_Exception &ex, int identation = 0);

    static int scanConnectedCameras(); // scan connected cameras when the first object will be created
};

#endif // ANDOR_CAMERA_H
