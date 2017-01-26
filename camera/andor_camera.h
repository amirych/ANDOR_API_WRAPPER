#ifndef ANDOR_CAMERA_H
#define ANDOR_CAMERA_H

#include "../export_decl.h"
#include "andorsdk_exception.h"

#include <atcore.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <utility>
#include <iostream>
#include <sstream>
#include <functional>
#include <thread>

// for MS compilers: disable multiple warnings about DLL-exports for the STL containers
// (and many others C++11 defined classes, e.g., thread)
#ifdef _MSC_VER
#pragma warning( disable: 4251 )
#endif

// actually, AT_WC is wchar_t according to declaration in atcore.h
// to follow strategy of generalized SDK wrapper (in sence of hidding actuall data-types used in
// SDK functions) one should declarate a string type to work with name of SDK feature and
// representation of string- and enumerated-type values

typedef std::basic_string<AT_WC> andor_string_t;

typedef int andor_enum_index_t; // type for ANDOR SDK enumerated feature index

#define ANDOR_CAMERA_LOG_IDENTATION 3

const int ANDOR_SDK_ENUM_FEATURE_STRLEN = 30; // maximal length of string for enumerated feature


struct ANDOR_CameraInfo;      // just forward declaration
class NonNumericFeatureValue;
struct ANDOR_StringFeature;
struct ANDOR_EnumFeature;
class ANDOR_EnumFeatureInfo;


                    /************************************/
                    /*  ANDOR CAMERA CLASS DECLARATION  */
                    /************************************/

class ANDOR_API_WRAPPER_EXPORT ANDOR_Camera
{
    friend class NonNumericFeatureValue;
    friend struct ANDOR_StringFeature;
    friend struct ANDOR_EnumFeature;
    friend class ANDOR_EnumFeatureInfo;

public:
    enum LOG_IDENTIFICATOR {CAMERA_INFO, SDK_ERROR, CAMERA_ERROR, BLANK};
    enum LOG_LEVEL {LOG_LEVEL_VERBOSE, LOG_LEVEL_ERROR, LOG_LEVEL_QUIET};
    enum AndorFeatureType {UnknownType = -1, BoolType, IntType, FloatType, StringType, EnumType};
    enum CAMERA_IDENT_TAG {CameraModel, CameraName, SerialNumber, ControllerID};

    typedef std::map<andor_string_t,AndorFeatureType> AndorFeatureNameMap;

    explicit ANDOR_Camera();

    virtual ~ANDOR_Camera();


protected:

    // function type for extra-logging facility
    // signature is the same as ANDOR_Camera::logToFile (the first overloaded variant)

    typedef std::function<void(const ANDOR_Camera::LOG_IDENTIFICATOR, const std::string &, const int)> log_func_t;

                /*   DECLARATION OF A PROXY CLASS TO ACCESS ANDOR SDK FEATURES  */

    class ANDOR_Feature {
        friend struct ANDOR_StringFeature;
        friend struct ANDOR_EnumFeature;
        friend class ANDOR_EnumFeatureInfo;
    public:
        ANDOR_Feature();
        ANDOR_Feature(const AT_H device_hndl, const andor_string_t &name = andor_string_t());
        ANDOR_Feature(const AT_H device_hndl, const AT_WC* name = nullptr);

        ANDOR_Feature(const andor_string_t &name);
        ANDOR_Feature(const AT_WC* name);

        ANDOR_Feature(ANDOR_Feature && other) = delete;
        ANDOR_Feature(const ANDOR_Feature &other) = delete;

        ANDOR_Feature & operator = (const ANDOR_Feature &other) = delete;
        ANDOR_Feature & operator = (ANDOR_Feature other) = delete;

//        ~ANDOR_Feature();

        void setName(const andor_string_t &name);
        void setName(const AT_WC* name);

        void setDeviceHndl(const AT_H hndl);
        AT_H getDeviceHndl() const;

        void setType(const AndorFeatureType &type);
        AndorFeatureType getType() const;

        std::string getLastLogMessage() const;

        // see declaration of 'log_func_t' type above
        void setLoggingFunc(const log_func_t &func = nullptr);


                 // get feature value operators

        template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
        operator T()
        {
//            static T ret_val;
            T ret_val;
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


                 // get range of numeric feature and list of enumarated feature values

        template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
        operator std::pair<T,T>()
        {
//            static std::pair<T,T> ret_val;
            std::pair<T,T> ret_val;

            switch ( featureType ) {
                case ANDOR_Camera::IntType: {
                    std::pair<AT_64,AT_64> v = getIntMinMax();
                    ret_val.first = T(v.first);
                    ret_val.second = T(v.second);
                }
                case ANDOR_Camera::FloatType: {
                    std::pair<double,double> v = getFloatMinMax();
                    ret_val.first = T(v.first);
                    ret_val.second = T(v.second);
                }
                case ANDOR_Camera::BoolType: {
                    std::pair<bool,bool> v(false,true);
                    ret_val.first = T(v.first);
                    ret_val.second = T(v.second);
                }
                default: {
                    throw AndorSDK_Exception(AT_ERR_INDEXNOTIMPLEMENTED,"Invalid feature type to get a range of its value!");
                }
            }

            return ret_val;
        }

        explicit operator ANDOR_EnumFeatureInfo();


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

//        ANDOR_Feature & operator = (const ANDOR_StringFeature &val);
//        ANDOR_Feature & operator = (const ANDOR_EnumFeature &val);
        ANDOR_Feature &  operator = (const andor_string_t &val);
        ANDOR_Feature &  operator = (const AT_WC *val);


    private:
        operator andor_string_t();
        AT_H deviceHndl;
        AT_WC* featureName;
        andor_string_t featureNameStr;
        AndorFeatureType featureType;
        union {
            AT_64 at64_val;  // integer feature
            AT_BOOL at_bool; // boolean feature
            double at_float; // floating-point feature
            andor_enum_index_t at_index; // index of enumerated feature
        };
        andor_string_t at_string; // enumerated or string feature

        std::stringstream logMessageStream;

        log_func_t loggingFunc;


        void setInt(const AT_64 val);
        void setFloat(const double val);
        void setBool(const bool val);
        void setString(const wchar_t *val);
        void setString(const andor_string_t &val);
        void setEnumString(const wchar_t *val);
        void setEnumString(const andor_string_t &val);
        void setEnumIndex(const andor_enum_index_t val);

        std::pair<AT_64,AT_64> getIntMinMax();
        std::pair<double,double> getFloatMinMax();
        std::vector<andor_string_t> getEnumInfo(std::vector<andor_enum_index_t> &imIdx, std::vector<andor_enum_index_t> &aIdx);

        void getInt();
        void getFloat();
        void getBool();
        void getString();
        void getEnumString();
        void getEnumIndex();


        // format log message for ANDOR SDK function calling
        // (the first argument is the name of SDK function, the others are its arguments)
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
        inline void logHelper(const andor_string_t &wstr);
        inline void logHelper(const AT_WC* wstr);
    };

                /*  END OF ANDOR_Feature CLASS DECLARATION  */


public:

    void setLogLevel(const LOG_LEVEL level);
    LOG_LEVEL getLogLevel() const;

    bool connectToCamera(const int device_index, std::ostream *log_file);
    bool connectToCamera(const ANDOR_Camera::CAMERA_IDENT_TAG ident_tag, const andor_string_t &tag_str, std::ostream *log_file);
    void disconnectFromCamera();


            /* operator[] for accessing Andor SDK features (const and non-const versions) */

    ANDOR_Feature& operator[](const andor_string_t &feature_name);
    ANDOR_Feature& operator[](const AT_WC* feature_name);
    ANDOR_Feature& operator[](const std::string &feature_name);
    ANDOR_Feature& operator[](const char *feature_name);


            /* Andor SDK global features */

    static ANDOR_Feature DeviceCount;
    static ANDOR_Feature SoftwareVersion;


            /* "CameraPresent" and "CameraAcquiring" feature declarations */

    ANDOR_Feature CameraPresent;
    ANDOR_Feature CameraAcquiring;


           /*  logging methods */

    void logToFile(const LOG_IDENTIFICATOR ident, const std::string &log_str, const int identation = 0); // general logging
    void logToFile(const LOG_IDENTIFICATOR ident, const char *log_str, const int identation = 0); // general logging
    void logToFile(const AndorSDK_Exception &ex, const int identation = 0); // SDK error logging


protected:
    LOG_LEVEL logLevel;

    AT_H cameraHndl;
    int cameraIndex;

    int lastError;

    std::ostream *cameraLog;

    ANDOR_Feature cameraFeature;

    void logToFile(const ANDOR_Feature &feature, const int identation = 0); // SDK function calling logging


    std::thread waitBufferThread;

    void waitBufferFunc();

                /*  static class members and methods  */

    static std::list<ANDOR_CameraInfo> foundCameras;
    static std::list<int> openedCameraIndices;
    static size_t numberOfCreatedObjects;

    static AndorFeatureNameMap ANDOR_SDK_FEATURES;

    static int scanConnectedCameras(); // scan connected cameras when the first object will be created

}; // end of ANDOR_Camera class declaration




            /*   DECLARATION OF BASE CLASS FOR ANDOR SDK NON-NUMERIC TYPE FEATURES PRESENTATION  */


class NonNumericFeatureValue {
    friend class ANDOR_Camera::ANDOR_Feature;
protected:
    andor_string_t _value;
    explicit NonNumericFeatureValue();
    NonNumericFeatureValue( const NonNumericFeatureValue &other);
    NonNumericFeatureValue( NonNumericFeatureValue &&other);

public:
    andor_string_t value() const;

    std::string value_to_string();
};




                /*   DECLARATION OF CLASS FOR ANDOR SDK STRING-TYPE FEATURE PRESENTATION  */


struct ANDOR_API_WRAPPER_EXPORT ANDOR_StringFeature: public NonNumericFeatureValue
{
    explicit ANDOR_StringFeature();
    ANDOR_StringFeature(ANDOR_Camera::ANDOR_Feature &feature);
    ANDOR_StringFeature(const ANDOR_StringFeature &other);
    ANDOR_StringFeature(ANDOR_StringFeature &&other);
    ANDOR_StringFeature & operator = (ANDOR_StringFeature other);
};



                /*   DECLARATION OF CLASSES FOR ANDOR SDK ENUMERATED-TYPE FEATURE PRESENTATION  */

struct ANDOR_API_WRAPPER_EXPORT ANDOR_EnumFeature: public NonNumericFeatureValue
{
    friend class ANDOR_Camera::ANDOR_Feature;

    explicit ANDOR_EnumFeature();
    ANDOR_EnumFeature(ANDOR_Camera::ANDOR_Feature &feature);
    ANDOR_EnumFeature(const ANDOR_EnumFeature &other);
    ANDOR_EnumFeature(ANDOR_EnumFeature &&other);
    ANDOR_EnumFeature & operator = (ANDOR_EnumFeature other);

    andor_enum_index_t index() const;

private:
    andor_enum_index_t _index;
};



class ANDOR_API_WRAPPER_EXPORT ANDOR_EnumFeatureInfo
{
    friend class ANDOR_Camera::ANDOR_Feature;

    std::vector<andor_string_t> valueList;
    std::vector<andor_enum_index_t> availableIndex;
    std::vector<andor_enum_index_t> implementedIndex;

    andor_enum_index_t currentIndex;

    void swap(ANDOR_EnumFeatureInfo &v1, ANDOR_EnumFeatureInfo &v2);

public:
    ANDOR_EnumFeatureInfo();
    ANDOR_EnumFeatureInfo(const ANDOR_EnumFeatureInfo &other);
    ANDOR_EnumFeatureInfo(ANDOR_EnumFeatureInfo &&other);
    ANDOR_EnumFeatureInfo(ANDOR_Camera::ANDOR_Feature &feature);

    ANDOR_EnumFeatureInfo & operator =(ANDOR_EnumFeatureInfo other);

    std::vector<andor_string_t> getValues() const;
    std::vector<andor_string_t> getAvailableValues();
    std::vector<andor_string_t> getImplementedValues();

    std::vector<andor_enum_index_t> getAvailableIndices() const;
    std::vector<andor_enum_index_t> getImplementedIndices() const;

    andor_enum_index_t index() const;
};



                /*  ANDOR CAMERA INFO STRUCTURE  */

struct ANDOR_API_WRAPPER_EXPORT ANDOR_CameraInfo
{
    ANDOR_CameraInfo();

    ANDOR_StringFeature cameraModel;
    ANDOR_StringFeature cameraName;
    ANDOR_StringFeature serialNumber;
    ANDOR_StringFeature controllerID;
    AT_64 sensorWidth;
    AT_64 sensorHeight;
    double pixelWidth; // in micrometers
    double pixelHeight;
//    andor_string_t interfaceType;
    ANDOR_StringFeature interfaceType;
    int device_index;

    enum ANDOR_CameraInfoType {CameraModel, CameraName, SerialNumber, ControllerID};
};



#endif // ANDOR_CAMERA_H
