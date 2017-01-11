                        /*******************************************
                         *                                         *
                         *  IMPLEMENTATION OF ANDOR_Feature CLASS  *
                         *                                         *
                         *******************************************/


#include "andor_camera.h"

//#include <typeinfo>
#include <locale>
#include <codecvt>


static std::wstring trim_str(const std::wstring &str)
{
    size_t first = str.find_first_not_of(' ');
    if ( first == std::string::npos ) return std::wstring();

    size_t last = str.find_last_not_of(' ');

    return str.substr(first, last-first+1);
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature():
    deviceHndl(AT_HANDLE_SYSTEM), featureName(nullptr), featureNameStr(std::wstring()),
    featureType(UnknownType), logMessageStream()
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const std::wstring &name):
    ANDOR_Feature()
{
    setDeviceHndl(device_hndl);
    setName(name);
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const wchar_t *name):
    ANDOR_Feature(device_hndl,std::wstring(name))
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const std::wstring &name):
    ANDOR_Feature(AT_HANDLE_SYSTEM, name)
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const wchar_t *name):
    ANDOR_Feature(std::wstring(name))
{

}

void ANDOR_Camera::ANDOR_Feature::setName(const std::wstring &name)
{
//    if ( trim_str(name).empty() ) { // !!!!! TO DO: throw an exception!!
//        return;
//    }

    // suppose the 'name' is already valid name of SDK feature!!! (see, e.g., ANDOR_Camera::operator[])
    // If the 'name' is not valid name then an error will occur after SDK function calling
    featureNameStr = name;
    featureName = (AT_WC*) featureNameStr.c_str();
}


void ANDOR_Camera::ANDOR_Feature::setName(const wchar_t *name)
{
    setName(std::wstring(name));
}


void ANDOR_Camera::ANDOR_Feature::setDeviceHndl(const AT_H hndl)
{
    deviceHndl = hndl;
}


AT_H ANDOR_Camera::ANDOR_Feature::getDeviceHndl() const
{
    return deviceHndl;
}


void ANDOR_Camera::ANDOR_Feature::setType(const AndorFeatureType &type)
{
    featureType = type;
}


ANDOR_Camera::AndorFeatureType ANDOR_Camera::ANDOR_Feature::getType() const
{
    return featureType;
}


ANDOR_Camera::ANDOR_Feature::operator ANDOR_StringFeature()
{
    if ( featureType != ANDOR_Camera::StringType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    getString();

    ANDOR_StringFeature f(at_string.c_str());

    return f;
}


ANDOR_Camera::ANDOR_Feature::operator ANDOR_EnumFeature()
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    getEnumString();

    ANDOR_EnumFeature f(at_string.c_str());
    f._index = at_index;

    return f;
}



                        /*  PRIVATE METHODS  */


                        /*  'get value' methods  */


ANDOR_Camera::ANDOR_Feature::operator std::wstring()
{
    switch ( featureType ) {
        case ANDOR_Camera::StringType:
            getString();
            break;
        case ANDOR_Camera::EnumType:
            getEnumString();
            break;
        default:
            throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    return at_string;
}


void ANDOR_Camera::ANDOR_Feature::getInt()
{
    if ( featureType != ANDOR_Camera::IntType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_GetInt","&at64_val");

    andor_sdk_assert( AT_GetInt(deviceHndl,featureName,&at64_val), logMessageStream.str());
}


void ANDOR_Camera::ANDOR_Feature::getFloat()
{
    if ( featureType != ANDOR_Camera::FloatType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_GetFloat","&at_float");

    andor_sdk_assert( AT_GetFloat(deviceHndl,featureName,&at_float), logMessageStream.str());
}


void ANDOR_Camera::ANDOR_Feature::getBool()
{
    if ( featureType != ANDOR_Camera::BoolType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    AT_BOOL flag;

    formatLogMessage("AT_GetBool","&flag");

    andor_sdk_assert( AT_GetBool(deviceHndl,featureName,&flag), logMessageStream.str());

    at_bool = flag == AT_TRUE ? true : false;
}


void ANDOR_Camera::ANDOR_Feature::getString()
{
    if ( featureType != ANDOR_Camera::StringType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    int len;

    formatLogMessage("AT_GetStringMaxLength","&len");

    andor_sdk_assert( AT_GetStringMaxLength(deviceHndl,featureName,&len), logMessageStream.str());

    if ( len ) {
        AT_WC str[len];

        formatLogMessage("AT_GetString","str",len);

        andor_sdk_assert( AT_GetString(deviceHndl,featureName,str,len), logMessageStream.str());

        at_string = str;
    } else {
        throw AndorSDK_Exception(AT_ERR_NULL_MAXSTRINGLENGTH,"Length of string feature value is 0!");
    }
}


void ANDOR_Camera::ANDOR_Feature::getEnumIndex()
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_GetEnumIndex","&at_index");

    andor_sdk_assert( AT_GetEnumIndex(deviceHndl,featureName,&at_index), logMessageStream.str());
}


void ANDOR_Camera::ANDOR_Feature::getEnumString()
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    getEnumIndex();

    AT_WC str[ANDOR_SDK_ENUM_FEATURE_STRLEN];

    formatLogMessage("AT_GetEnumStringByIndex",at_index,"str",ANDOR_SDK_ENUM_FEATURE_STRLEN);

    andor_sdk_assert( AT_GetEnumStringByIndex(deviceHndl,featureName,at_index,str,ANDOR_SDK_ENUM_FEATURE_STRLEN),
                      logMessageStream.str());

    at_string = str;
}



                                /*  'set value' methods  */

void ANDOR_Camera::ANDOR_Feature::setInt(const AT_64 val)
{
    if ( featureType != ANDOR_Camera::IntType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetInt",val);

    andor_sdk_assert( AT_SetInt(deviceHndl,featureName,val), logMessageStream.str());

    at64_val = val;
}


void ANDOR_Camera::ANDOR_Feature::setFloat(const double val)
{
    if ( featureType != ANDOR_Camera::FloatType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetFloat",val);

    andor_sdk_assert( AT_SetFloat(deviceHndl,featureName,val), logMessageStream.str());

    at_float = val;
}


void ANDOR_Camera::ANDOR_Feature::setBool(const bool val)
{
    if ( featureType != ANDOR_Camera::BoolType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    AT_BOOL flag = val ? AT_TRUE : AT_FALSE;

    formatLogMessage("AT_SetBool",flag);

    andor_sdk_assert( AT_SetBool(deviceHndl,featureName,flag), logMessageStream.str());

    at_bool = val;
}


void ANDOR_Camera::ANDOR_Feature::setString(const std::wstring &val)
{
    if ( featureType != ANDOR_Camera::StringType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetString",val);

    andor_sdk_assert( AT_SetString(deviceHndl,featureName,val.c_str()), logMessageStream.str());

    at_string = val;
}

void ANDOR_Camera::ANDOR_Feature::setString(const wchar_t *val)
{
    setString(std::wstring(val));
}



void ANDOR_Camera::ANDOR_Feature::setEnumString(const std::wstring &val)
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetEnumString",val);

    andor_sdk_assert( AT_SetEnumString(deviceHndl,featureName,val.c_str()), logMessageStream.str());

    at_string = val;
}

void ANDOR_Camera::ANDOR_Feature::setEnumString(const wchar_t *val)
{
    setEnumString(std::wstring(val));
}


void ANDOR_Camera::ANDOR_Feature::setEnumIndex(const andor_enum_index_t val)
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetEnumIndex",val);

    andor_sdk_assert( AT_SetEnumIndex(deviceHndl,featureName,val), logMessageStream.str());

    at_index = val;
}


//ANDOR_StringFeature & ANDOR_Camera::ANDOR_Feature::operator = (const ANDOR_StringFeature &val)
//{
//    setString(val);

//    static ANDOR_StringFeature f(at_string.c_str());

//    return f;
//}
ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const ANDOR_StringFeature &val)
{
    setString(val);

    return *this;
}


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const ANDOR_EnumFeature &val)
{
    setEnumString(val);

    return *this;
}


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const wchar_t* val)
{
    switch ( featureType ) {
        case ANDOR_Camera::StringType:
            return operator = (ANDOR_StringFeature(val));
        case ANDOR_Camera::EnumType:
            return operator = (ANDOR_EnumFeature(val));
        default:
            throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

}

ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const std::wstring &val)
{
    return operator = (val.c_str());
}

                                /*  auxiliary methods  */

template<typename... T>
void ANDOR_Camera::ANDOR_Feature::formatLogMessage(const std::string &sdk_func, T... args)
{
    logMessageStream.str("");  // clear string stream
    logMessageStream << sdk_func << "(";  // print SDK function name
    logHelper(featureNameStr); // print feature name
    logHelper(deviceHndl);     // print device handler
    logHelper(args ...);       // print function arguments
    logMessageStream << ")";
}


template<typename... T>
void ANDOR_Camera::ANDOR_Feature::formatLogMessage(const char* sdk_func, T... args)
{
    formatLogMessage(std::string(sdk_func), args...);
}


template<typename T1, typename... T2>
void ANDOR_Camera::ANDOR_Feature::logHelper(T1 first, T2... last)
{
    logHelper(first);
    logMessageStream << ", ";
    logHelper(last ...);
}

template<typename T>
void ANDOR_Camera::ANDOR_Feature::logHelper(T arg)
{
    logMessageStream << arg;
}

void ANDOR_Camera::ANDOR_Feature::logHelper(const std::string &str)
{
    logMessageStream << "'" << str << "'";
}

void ANDOR_Camera::ANDOR_Feature::logHelper(const char *str)
{
    logHelper(std::string(str));
}

void ANDOR_Camera::ANDOR_Feature::logHelper(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> str;
    logMessageStream << "'" << str.to_bytes(wstr) << "'";
}

void ANDOR_Camera::ANDOR_Feature::logHelper(const wchar_t *wstr)
{
    logHelper(std::wstring(wstr));
}

