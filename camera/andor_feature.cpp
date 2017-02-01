                        /*******************************************
                         *                                         *
                         *  IMPLEMENTATION OF ANDOR_Feature CLASS  *
                         *                                         *
                         *******************************************/


#include "andor_camera.h"

//#include <typeinfo>
#include <locale>
#include <codecvt>


static andor_string_t trim_str(const andor_string_t &str)
{
    size_t first = str.find_first_not_of(' ');
    if ( first == std::string::npos ) return andor_string_t();

    size_t last = str.find_last_not_of(' ');

    return str.substr(first, last-first+1);
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature():
    deviceHndl(AT_HANDLE_SYSTEM), featureName(nullptr), featureNameStr(andor_string_t()),
    featureType(UnknownType), logMessageStream(), loggingFunc(nullptr), at_string()
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const andor_string_t &name):
    ANDOR_Feature()
{
    setDeviceHndl(device_hndl);
    setName(name);
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const AT_WC *name):
    ANDOR_Feature(device_hndl,andor_string_t(name))
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const andor_string_t &name):
    ANDOR_Feature(AT_HANDLE_SYSTEM, name)
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_WC *name):
    ANDOR_Feature(andor_string_t(name))
{

}

void ANDOR_Camera::ANDOR_Feature::setName(const andor_string_t &name)
{
//    if ( trim_str(name).empty() ) { // !!!!! TO DO: throw an exception!!
//        return;
//    }

    // suppose the 'name' is already valid name of SDK feature!!! (see, e.g., ANDOR_Camera::operator[])
    // If the 'name' is not valid name then an error will occur after SDK function calling
    featureNameStr = name;
    featureName = (AT_WC*) featureNameStr.c_str();
}


void ANDOR_Camera::ANDOR_Feature::setName(const AT_WC *name)
{
    setName(andor_string_t(name));
}


void ANDOR_Camera::ANDOR_Feature::setDeviceHndl(const AT_H hndl)
{
    deviceHndl = hndl;
}


void ANDOR_Camera::ANDOR_Feature::setLoggingFunc(const log_func_t &func)
{
    loggingFunc = func;
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


std::string ANDOR_Camera::ANDOR_Feature::getLastLogMessage() const
{
    return logMessageStream.str();
}



ANDOR_Camera::ANDOR_Feature::operator ANDOR_StringFeature()
{
    if ( featureType != ANDOR_Camera::StringType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    getString();

    ANDOR_StringFeature f;

    f._name = featureNameStr;
    f._value = at_string;

    return f;
}


ANDOR_Camera::ANDOR_Feature::operator ANDOR_EnumFeature()
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    getEnumString();

    ANDOR_EnumFeature f;
    f._name = featureNameStr;
    f._value = at_string;
    f._index = at_index;

    return f;
}



                        /*  PRIVATE METHODS  */


                        /*  'get value' methods  */


//ANDOR_Camera::ANDOR_Feature::operator andor_string_t()
//{
//    switch ( featureType ) {
//        case ANDOR_Camera::StringType:
//            getString();
//            break;
//        case ANDOR_Camera::EnumType:
//            getEnumString();
//            break;
//        default:
//            throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
//    }

//    return at_string;
//}


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
        AT_WC* str;
        try {
//            AT_WC str[len]; // not allowed in VS, only GCC
            str = new AT_WC[len];
        } catch (std::bad_alloc &ex ) {
            throw AndorSDK_Exception(AT_ERR_NOMEMORY, "Can not allocate memory for AT_WC* string!");
        }

        formatLogMessage("AT_GetString","str",len);

        andor_sdk_assert( AT_GetString(deviceHndl,featureName,str,len), logMessageStream.str());

        at_string = str;

        delete[] str;
    } else {
        at_string = L"";
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


                                /*  'get range' methods  */

std::pair<AT_64,AT_64> ANDOR_Camera::ANDOR_Feature::getIntMinMax()
{
    if ( featureType != ANDOR_Camera::IntType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    std::pair<AT_64,AT_64> val(0,0);

    formatLogMessage("AT_GetIntMin","&min_val");
    andor_sdk_assert( AT_GetIntMin(deviceHndl,featureName,&val.first), logMessageStream.str());

    formatLogMessage("AT_GetIntMax","&max_val");
    andor_sdk_assert( AT_GetIntMax(deviceHndl,featureName,&val.second), logMessageStream.str());

    return val;
}


std::pair<double,double> ANDOR_Camera::ANDOR_Feature::getFloatMinMax()
{
    if ( featureType != ANDOR_Camera::FloatType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    std::pair<double,double> val(0,0);

    formatLogMessage("AT_GetFloatMin","&min_val");
    andor_sdk_assert( AT_GetFloatMin(deviceHndl,featureName,&val.first), logMessageStream.str());

    formatLogMessage("AT_GetFloatMax","&max_val");
    andor_sdk_assert( AT_GetFloatMax(deviceHndl,featureName,&val.second), logMessageStream.str());

    return val;
}


std::vector<andor_string_t> ANDOR_Camera::ANDOR_Feature::getEnumInfo(std::vector<andor_enum_index_t> &imIdx,
                                                                     std::vector<andor_enum_index_t> &aIdx)
{
    int count;
    AT_WC str[ANDOR_SDK_ENUM_FEATURE_STRLEN];
    AT_BOOL flag;

    std::vector<andor_string_t> vals;

    formatLogMessage("AT_GetEnumCount","&count");
    andor_sdk_assert( AT_GetEnumCount(deviceHndl, featureName, &count), logMessageStream.str() );

    if ( !count ) return vals;

    aIdx.clear();
    imIdx.clear();

    for ( int i = 0; i < count; ++i ) {
        formatLogMessage("AT_GetEnumStringByIndex",i,"str",ANDOR_SDK_ENUM_FEATURE_STRLEN);

        andor_sdk_assert( AT_GetEnumStringByIndex(deviceHndl,featureName,i,str,ANDOR_SDK_ENUM_FEATURE_STRLEN),
                          logMessageStream.str());

        vals.push_back(str);

        formatLogMessage("AT_IsEnumIndexImplemented",i,"&flag");

        andor_sdk_assert( AT_IsEnumIndexImplemented(deviceHndl,featureName,i,&flag),
                          logMessageStream.str());

        if ( flag ) { // if it is not implemented then it is not available too
            imIdx.push_back(i);

            formatLogMessage("AT_IsEnumIndexAvailable",i,"&flag");

            andor_sdk_assert( AT_IsEnumIndexAvailable(deviceHndl,featureName,i,&flag),
                              logMessageStream.str());

            if ( flag ) aIdx.push_back(i);
        }

    }

    getEnumIndex(); // get current index

    return vals;
}



void ANDOR_Camera::ANDOR_Feature::getFeatureInfo(bool *is_impl, bool *is_read, bool *is_readonly, bool *is_write)
{
    AT_BOOL flag;

    std::string log_msg;

    if ( is_impl == nullptr || is_read == nullptr || is_readonly == nullptr || is_write == nullptr ) return;

    formatLogMessage("AT_IsImplemented","&flag");

    andor_sdk_assert( AT_IsImplemented(deviceHndl,featureName,&flag), log_msg);

    *is_impl = flag;


    formatLogMessage("AT_IsReadable","&flag");

    andor_sdk_assert( AT_IsReadable(deviceHndl,featureName,&flag), log_msg);

    *is_read = flag;


    formatLogMessage("AT_IsReadOnly","&flag");

    andor_sdk_assert( AT_IsReadOnly(deviceHndl,featureName,&flag), log_msg);

    *is_readonly = flag;


    formatLogMessage("AT_IsWritable","&flag");

    andor_sdk_assert( AT_IsWritable(deviceHndl,featureName,&flag), log_msg);

    *is_write = flag;
}



ANDOR_Camera::ANDOR_Feature::operator ANDOR_FeatureInfo()
{
    ANDOR_FeatureInfo info;

    getFeatureInfo(&info._isImplemented, &info._isReadable, &info._isReadOnly, &info._isWritable);

    return info;
}


ANDOR_Camera::ANDOR_Feature::operator ANDOR_EnumFeatureInfo()
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    ANDOR_EnumFeatureInfo info;

    info.valueList = getEnumInfo(info._implementedIndex, info._availableIndex);
    info._name = featureNameStr;

    return info;
}


bool ANDOR_Camera::ANDOR_Feature::operator !()
{
    switch ( featureType ) {
        case ANDOR_Camera::BoolType: {
            return !at_bool;
        }
        case ANDOR_Camera::IntType: {
            return !at64_val;
        }
        case ANDOR_Camera::FloatType: {
            return !at_float;
        }
        case ANDOR_Camera::EnumType:
        case ANDOR_Camera::StringType: { // return 'true' if string is empty
            return at_string.empty() ? false : true;
        }
    }
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


void ANDOR_Camera::ANDOR_Feature::setString(const andor_string_t &val)
{
    if ( featureType != ANDOR_Camera::StringType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetString",val);

    andor_sdk_assert( AT_SetString(deviceHndl,featureName,val.c_str()), logMessageStream.str());

    at_string = val;
}

void ANDOR_Camera::ANDOR_Feature::setString(const AT_WC *val)
{
    setString(andor_string_t(val));
}



void ANDOR_Camera::ANDOR_Feature::setEnumString(const andor_string_t &val)
{
    if ( featureType != ANDOR_Camera::EnumType ) {
        throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

    formatLogMessage("AT_SetEnumString",val);

    andor_sdk_assert( AT_SetEnumString(deviceHndl,featureName,val.c_str()), logMessageStream.str());

    at_string = val;
}

void ANDOR_Camera::ANDOR_Feature::setEnumString(const AT_WC *val)
{
    setEnumString(andor_string_t(val));
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
//ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const ANDOR_StringFeature &val)
//{
//    setString(val);

//    return *this;
//}


//ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const ANDOR_EnumFeature &val)
//{
//    setEnumString(val);

//    return *this;
//}


ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const AT_WC* val)
{
    switch ( featureType ) {
        case ANDOR_Camera::StringType:
//            return operator = (ANDOR_StringFeature(val));
            setString(val);
            return *this;
        case ANDOR_Camera::EnumType:
//        return operator = (ANDOR_EnumFeature(val));
            setEnumString(val);
            return *this;
        default:
            throw AndorSDK_Exception(AT_ERR_NOTIMPLEMENTED,"Feature type missmatch!");
    }

}

ANDOR_Camera::ANDOR_Feature & ANDOR_Camera::ANDOR_Feature::operator = (const andor_string_t &val)
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

    if ( loggingFunc ) loggingFunc(ANDOR_Camera::CAMERA_INFO,logMessageStream.str(),0); // call user logging function
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

void ANDOR_Camera::ANDOR_Feature::logHelper(const andor_string_t &wstr)
{
    std::wstring_convert<std::codecvt_utf8<AT_WC>> str;
    logMessageStream << "'" << str.to_bytes(wstr) << "'";
}

void ANDOR_Camera::ANDOR_Feature::logHelper(const AT_WC *wstr)
{
    logHelper(andor_string_t(wstr));
}



                    /*  IMPLEMENTATION OF CLASS FOR BASIC FEATURE INFO  */


ANDOR_FeatureInfo::ANDOR_FeatureInfo():
    _name(),
    _isImplemented(false), _isReadable(false), _isReadOnly(false), _isWritable(false)
{
}


ANDOR_FeatureInfo::ANDOR_FeatureInfo(ANDOR_Camera::ANDOR_Feature &feature):
    ANDOR_FeatureInfo()
{
    _name = feature.featureNameStr;
    feature.getFeatureInfo(&_isImplemented, &_isReadable, &_isReadOnly, &_isWritable);
}


ANDOR_FeatureInfo::ANDOR_FeatureInfo(const ANDOR_FeatureInfo &other):
    _name(other._name),
    _isImplemented(other._isImplemented), _isReadable(other._isReadable),
    _isReadOnly(other._isReadOnly), _isWritable(other._isWritable)
{
}


ANDOR_FeatureInfo::ANDOR_FeatureInfo(ANDOR_FeatureInfo &&other):
    ANDOR_FeatureInfo()
{
    swap(*this, other);
}


ANDOR_FeatureInfo & ANDOR_FeatureInfo::operator = (ANDOR_FeatureInfo other)
{
    swap(*this, other);

    return *this;
}


andor_string_t ANDOR_FeatureInfo::name() const
{
    return _name;
}


bool ANDOR_FeatureInfo::isImplemented() const
{
    return _isImplemented;
}


bool ANDOR_FeatureInfo::isReadable() const
{
    return _isReadable;
}


bool ANDOR_FeatureInfo::isReadOnly() const
{
    return _isReadOnly;
}


bool ANDOR_FeatureInfo::isWritable() const
{
    return _isWritable;
}


void ANDOR_FeatureInfo::swap(ANDOR_FeatureInfo &v1, ANDOR_FeatureInfo &v2)
{
    std::swap(v1._name, v2._name);
    std::swap(v1._isImplemented, v2._isImplemented);
    std::swap(v1._isReadable, v2._isReadable);
    std::swap(v1._isReadOnly, v2._isReadOnly);
    std::swap(v1._isWritable, v2._isWritable);
}



                    /*  ANDOR SDK STRING AND ENUMERATED FEATURE CLASSES IMPLEMENTATION  */

NonNumericFeatureValue::NonNumericFeatureValue():
    _name(), _value()
{

}


NonNumericFeatureValue::NonNumericFeatureValue(const NonNumericFeatureValue &other):
    _name(other._name), _value(other._value)
{

}


NonNumericFeatureValue::NonNumericFeatureValue(NonNumericFeatureValue &&other):
    NonNumericFeatureValue()
{
    swap(*this,other);
}


andor_string_t NonNumericFeatureValue::name() const
{
    return _name;
}


andor_string_t NonNumericFeatureValue::value() const
{
    return _value;
}


std::string NonNumericFeatureValue::value_to_string()
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> cnv;

    return cnv.to_bytes(_value);

}

void NonNumericFeatureValue::swap(NonNumericFeatureValue &v1, NonNumericFeatureValue &v2)
{
    std::swap(v1._name, v2._name);
    std::swap(v1._value, v2._value);
}


//ANDOR_StringFeature::ANDOR_StringFeature(): andor_string_t()
ANDOR_StringFeature::ANDOR_StringFeature(): NonNumericFeatureValue()
{
}

ANDOR_StringFeature::ANDOR_StringFeature(ANDOR_Camera::ANDOR_Feature &feature):
//    andor_string_t(feature.operator andor_string_t())
    NonNumericFeatureValue()
{
    feature.getString();

    _name = feature.featureNameStr;
    _value = feature.at_string;
}


ANDOR_StringFeature::ANDOR_StringFeature(const ANDOR_StringFeature &other):
     NonNumericFeatureValue(other)
{

}


ANDOR_StringFeature::ANDOR_StringFeature(ANDOR_StringFeature &&other):
    NonNumericFeatureValue(std::move(other))
{

}


ANDOR_StringFeature& ANDOR_StringFeature::operator = (ANDOR_StringFeature other)
{
    if ( this == &other ) return *this;

//    std::swap(_name, other._name);
//    std::swap(_value, other._value);
    NonNumericFeatureValue::swap(*this,other);

    return *this;
}



ANDOR_EnumFeature::ANDOR_EnumFeature():
//    andor_string_t(), _index(-1)
    NonNumericFeatureValue(), _index(-1)
{

}


ANDOR_EnumFeature::ANDOR_EnumFeature(ANDOR_Camera::ANDOR_Feature &feature):
//    andor_string_t(feature.operator andor_string_t())
    ANDOR_EnumFeature()
{
//    _index = feature.at_index;
    feature.getEnumString();

    _name = feature.featureNameStr;
    _value = feature.at_string;
    _index = feature.at_index;
}



ANDOR_EnumFeature::ANDOR_EnumFeature(const ANDOR_EnumFeature &other):
    NonNumericFeatureValue(other)
{
    _index = other._index;
}


ANDOR_EnumFeature::ANDOR_EnumFeature(ANDOR_EnumFeature &&other):
    NonNumericFeatureValue(std::move(other))
{
    std::swap(_index, other._index);
}


ANDOR_EnumFeature & ANDOR_EnumFeature::operator = (ANDOR_EnumFeature other)
{
    if ( this == &other ) return *this;

    NonNumericFeatureValue::swap(*this,other);
    std::swap(_index, other._index);

    return *this;
}


            /*  ANDOR SDK ENUMERATED FEATURE INFO CLASS IMPLEMENTATION  */

ANDOR_EnumFeatureInfo::ANDOR_EnumFeatureInfo():
    _name(), valueList(), _availableIndex(), _implementedIndex(), currentIndex(-1)
{
}


ANDOR_EnumFeatureInfo::ANDOR_EnumFeatureInfo(const ANDOR_EnumFeatureInfo &other):
    _name(other._name),
    valueList(other.valueList), _availableIndex(other._availableIndex),
    _implementedIndex(other._implementedIndex), currentIndex(other.currentIndex)
{

}


ANDOR_EnumFeatureInfo::ANDOR_EnumFeatureInfo(ANDOR_EnumFeatureInfo &&other):
    ANDOR_EnumFeatureInfo()
{
    if ( this == &other ) return;

    swap(*this, other);
}


ANDOR_EnumFeatureInfo & ANDOR_EnumFeatureInfo::operator =(ANDOR_EnumFeatureInfo other)
{
    if ( this == &other ) return *this;

    swap(*this, other);

    return *this;
}



ANDOR_EnumFeatureInfo::ANDOR_EnumFeatureInfo(ANDOR_Camera::ANDOR_Feature &feature):
    ANDOR_EnumFeatureInfo()
{
    valueList = feature.getEnumInfo(_implementedIndex, _availableIndex);
    _name = feature.featureNameStr;
    currentIndex = feature.at_index;
}



andor_string_t ANDOR_EnumFeatureInfo::name() const
{
    return _name;
}


std::vector<andor_string_t> ANDOR_EnumFeatureInfo::values() const
{
    return valueList;
}


std::vector<andor_string_t> ANDOR_EnumFeatureInfo::availableValues()
{
    std::vector<andor_string_t> vals;

    for ( size_t i = 0; i < _availableIndex.size(); ++i ) {
        vals.push_back(valueList[i]);
    }

    return vals;
}


std::vector<andor_string_t> ANDOR_EnumFeatureInfo::implementedValues()
{
    std::vector<andor_string_t> vals;

    for ( size_t i = 0; i < _implementedIndex.size(); ++i ) {
        vals.push_back(valueList[i]);
    }

    return vals;
}


std::vector<andor_enum_index_t> ANDOR_EnumFeatureInfo::availableIndices() const
{
    return _availableIndex;
}


std::vector<andor_enum_index_t> ANDOR_EnumFeatureInfo::implementedIndices() const
{
    return _implementedIndex;
}


andor_enum_index_t ANDOR_EnumFeatureInfo::index() const
{
    return currentIndex;
}


void ANDOR_EnumFeatureInfo::swap(ANDOR_EnumFeatureInfo &v1, ANDOR_EnumFeatureInfo &v2)
{
    std::swap(v1._name, v2._name);
    std::swap(v1.valueList,v2.valueList);
    std::swap(v1._availableIndex,v2._availableIndex);
    std::swap(v1._implementedIndex,v2._implementedIndex);

    std::swap(v1.currentIndex, v2.currentIndex);
}
