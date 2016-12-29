                        /*******************************************
                         *                                         *
                         *  IMPLEMENTATION OF ANDOR_Feature CLASS  *
                         *                                         *
                         *******************************************/


#include "andor_camera.h"

#include <typeinfo>

static std::wstring trim_str(const std::wstring &str)
{
    size_t first = str.find_first_not_of(' ');
    if ( first == std::string::npos ) return std::wstring();

    size_t last = str.find_last_not_of(' ');

    return str.substr(first, last-first+1);
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature():
    deviceHndl(AT_HANDLE_SYSTEM), featureName(nullptr), featureNameStr(std::wstring()),
    featureType(UnknownType)
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const std::wstring &name):
    ANDOR_Feature()
{
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
    if ( trim_str(name).empty() ) { // !!!!! TO DO: throw an exception!!
        return;
    }

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


//template<typename T>
//typename std::enable_if<std::is_floating_point<T>::value, T&>::type
//ANDOR_Camera::ANDOR_Feature::operator=(const T &value)
//{

//}


//template<typename T>
//typename std::enable_if<(std::is_integral<T>::value && !std::is_same<T,bool>::value ), T&>::type
//ANDOR_Camera::ANDOR_Feature::operator=(const T &value)
//{

//}


//bool & ANDOR_Camera::ANDOR_Feature::operator = (const bool & value)
//{

//}


//std::wstring & ANDOR_Camera::ANDOR_Feature::operator = (const std::wstring & value)
//{
//;
//}


//template<typename T, typename TT>
////typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
//ANDOR_Camera::ANDOR_Feature::operator T()
//{
//    switch (typeid(T)) {
//    case typeid(std::is_integral<T>::value): {
//        break;
//    }
//    case typeid(std::is_floating_point<T>::value): {
//        break;
//    }

//    }
//}




ANDOR_Camera::ANDOR_Feature::operator std::wstring()
{

}



