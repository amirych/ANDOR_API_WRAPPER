#ifndef ANDOR_CAMERA_H
#define ANDOR_CAMERA_H

#include "../export_decl.h"
#include "andorsdk_exception.h"

#include <atcore.h>

#include <string>
#include <list>
#include <map>
#include <iostream>
#include <sstream>

//using namespace std;

typedef int andor_enum_index_t; // type for ANDOR SDK enumerated feature index

const int ANDOR_SDK_ENUM_FEATURE_STRLEN = 30;


class ANDOR_CameraInfo;      // just forward declaration
class ANDOR_StringFeature;
class ANDOR_EnumFeature;


                    /************************************/
                    /*  ANDOR CAMERA CLASS DECLARATION  */
                    /************************************/

class ANDOR_API_WRAPPER_EXPORT ANDOR_Camera
{
    friend class ANDOR_StringFeature;
    friend class ANDOR_EnumFeature;
public:
    enum LOG_IDENTIFICATOR {CAMERA_INFO, SDK_ERROR, CAMERA_ERROR};
    enum LOG_LEVEL {LOG_LEVEL_VERBOSE, LOG_LEVEL_ERROR, LOG_LEVEL_QUIET};
    enum AndorFeatureType {UnknownType = -1, BoolType, IntType, FloatType, StringType, EnumType};

    typedef std::map<std::wstring,AndorFeatureType> AndorFeatureNameMap;

    explicit ANDOR_Camera();
    virtual ~ANDOR_Camera();


protected:

                /*   DECLARATION OF A PROXY CLASS TO ACCESS ANDOR SDK FEATURES  */

    class ANDOR_Feature {
        friend class ANDOR_StringFeature;
        friend class ANDOR_EnumFeature;
    public:
        ANDOR_Feature();
        ANDOR_Feature(const AT_H device_hndl, const std::wstring &name = std::wstring());
        ANDOR_Feature(const AT_H device_hndl, const wchar_t* name = nullptr);

        ANDOR_Feature(const std::wstring &name);
        ANDOR_Feature(const wchar_t* name);

        ANDOR_Feature(ANDOR_Feature && other); // move ctor

//        ~ANDOR_Feature();

        void setName(const std::wstring &name);
        void setName(const wchar_t* name);

        void setDeviceHndl(const AT_H hndl);
        AT_H getDeviceHndl() const;

        void setType(const AndorFeatureType &type);
        AndorFeatureType getType() const;

        std::string getLastLogMessage() const;


                 // get feature value operators

        template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
        operator T()
        {
            static T ret_val;
            switch ( featureType ) {
                case ANDOR_Camera::IntType: {
                    getInt();
                    ret_val = T(at64_val);
                    break;
                }
                case ANDOR_Camera::FloatType: {
                    getFloat();
                    ret_val = T(at_float);
                    break;
                }
                case ANDOR_Camera::BoolType: {
                    getBool();
                    ret_val = T(at_bool);
                    break;
                }
                case ANDOR_Camera::EnumType: { // get index of enumerated feature
                    getEnumIndex();
                    ret_val = T(at_index);
                    break;
                }
            }
            return ret_val;
        }

        explicit operator ANDOR_StringFeature();
        explicit operator ANDOR_EnumFeature();


                 // set feature value operators

        template<typename T>
        using delRef = typename std::remove_reference<T>::type;


        template<typename T, typename = typename std::enable_if<std::is_arithmetic<delRef<T>>::value, delRef<T>>::type>
        ANDOR_Feature & operator=(T &&val)
        {
            switch ( featureType ) {
                case ANDOR_Camera::IntType: {
                    setInt(val);
                    break;
                }
                case ANDOR_Camera::FloatType: {
                    setFloat(val);
                    break;
                }
                case ANDOR_Camera::BoolType: {
                    setBool(val);
                    break;
                }
                case ANDOR_Camera::EnumType: {
                    setEnumIndex(val);
                    break;
                }
            }

            return *this;
        }

        ANDOR_Feature & operator = (const ANDOR_StringFeature &val);
        ANDOR_Feature & operator = (const ANDOR_EnumFeature &val);
        ANDOR_Feature &  operator = (const std::wstring &val);
        ANDOR_Feature &  operator = (const wchar_t *val);


    private:
        operator std::wstring();
        AT_H deviceHndl;
        AT_WC* featureName;
        std::wstring featureNameStr;
        AndorFeatureType featureType;
        union {
            AT_64 at64_val;  // integer feature
            AT_BOOL at_bool; // boolean feature
            double at_float; // floating-point feature
            andor_enum_index_t at_index; // index of enumerated feature
        };
        std::wstring at_string; // enumerated or string feature

        std::stringstream logMessageStream;

        void setInt(const AT_64 val);
        void setFloat(const double val);
        void setBool(const bool val);
        void setString(const wchar_t *val);
        void setString(const std::wstring &val);
        void setEnumString(const wchar_t *val);
        void setEnumString(const std::wstring &val);
        void setEnumIndex(const andor_enum_index_t val);

        void getInt();
        void getFloat();
        void getBool();
        void getString();
        void getEnumString();
        void getEnumIndex();


        // format log message for ANDOR SDK function calling
        // (the first argument is the name of SDk function, the others are its arguments)
        template<typename... T>
        void formatLogMessage(const std::string &sdk_func, T... args);

        template<typename... T>
        void formatLogMessage(const char* sdk_func, T... args);

        // helper methods for logging
        template<typename T1, typename... T2>
        inline void logHelper(T1 first, T2... last);

        template<typename T>
        inline void logHelper(T arg);

        inline void logHelper(const std::string &str);
        inline void logHelper(const char* str);
        inline void logHelper(const std::wstring &wstr);
        inline void logHelper(const wchar_t* wstr);
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

    AT_H cameraHndl;
    int cameraIndex;

    int lastError;

    std::ostream *cameraLog;

    ANDOR_Feature cameraFeature;

    void logToFile(LOG_IDENTIFICATOR ident, std::string &log_str, int identation = 0);
    void logToFile(const AndorSDK_Exception &ex, int identation = 0);


                /*  static class members and methods  */

    static std::list<ANDOR_CameraInfo> foundCameras;
    static std::list<int> openedCameraIndices;
    static size_t numberOfCreatedObjects;

    static AndorFeatureNameMap ANDOR_SDK_FEATURES;

    static int scanConnectedCameras(); // scan connected cameras when the first object will be created

}; // end of ANDOR_Camera class declaration




                /*   DECLARATION OF CLASS FOR ANDOR SDK STRING-TYPE FEATURE PRESENTATION  */

struct ANDOR_API_WRAPPER_EXPORT ANDOR_StringFeature: public std::wstring
{
    using std::wstring::wstring;
    ANDOR_StringFeature();
    ANDOR_StringFeature(ANDOR_Camera::ANDOR_Feature &feature);
};



                /*   DECLARATION OF CLASS FOR ANDOR SDK ENUMERATED-TYPE FEATURE PRESENTATION  */

struct ANDOR_API_WRAPPER_EXPORT ANDOR_EnumFeature: public std::wstring
{
    friend class ANDOR_Camera::ANDOR_Feature;

    using std::wstring::wstring;

    ANDOR_EnumFeature(); // one must init _index member to some invalid value at this point! do not inherite
                         // default ctor from wstring!
    ANDOR_EnumFeature(ANDOR_Camera::ANDOR_Feature &feature);

    andor_enum_index_t index() const;

private:
    andor_enum_index_t _index;
};



                /*  ANDOR CAMERA INFO STRUCTURE  */

struct ANDOR_API_WRAPPER_EXPORT ANDOR_CameraInfo
{
    ANDOR_CameraInfo();

    ANDOR_StringFeature cameraModel;
    ANDOR_StringFeature cameraName;
    ANDOR_StringFeature serialNumber;
    ANDOR_StringFeature controllerID;
    //    std::wstring cameraModel;
    //    std::wstring cameraName;
    //    std::wstring serialNumber;
    //    std::wstring controllerID;
    AT_64 sensorWidth;
    AT_64 sensorHeight;
    double pixelWidth; // in micrometers
    double pixelHeight;
//    std::wstring interfaceType;
    ANDOR_StringFeature interfaceType;
    int device_index;

    enum ANDOR_CameraInfoType {CameraModel, CameraName, SerialNumber, ControllerID};
};



#endif // ANDOR_CAMERA_H
