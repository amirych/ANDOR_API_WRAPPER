                        /*******************************************
                         *                                         *
                         *  IMPLEMENTATION OF ANDOR_Feature CLASS  *
                         *                                         *
                         *******************************************/


#include "andor_camera.h"

static std::wstring trim_str(const std::wstring &str)
{
    size_t first = str.find_first_not_of(' ');
    if ( first == std::string::npos ) return std::wstring();

    size_t last = str.find_last_not_of(' ');

    return str.substr(first, last-first+1);
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature():
    deviceHndl(AT_HANDLE_SYSTEM), featureName(nullptr), featureNameStr(std::wstring())
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const wstring &name):
    deviceHndl(device_hndl), featureName(nullptr), featureNameStr(name)
{
    setName(name);
    featureName = (AT_WC*) featureNameStr.c_str();
}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const AT_H device_hndl, const wchar_t *name):
    ANDOR_Feature(device_hndl,std::wstring(name))
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const wstring &name):
    ANDOR_Feature(AT_HANDLE_SYSTEM, name)
{

}


ANDOR_Camera::ANDOR_Feature::ANDOR_Feature(const wchar_t *name):
    ANDOR_Feature(std::wstring(name))
{

}

void ANDOR_Camera::ANDOR_Feature::setName(const wstring &name)
{
    if ( trim_str(name).empty() ) { // !!!!! TO DO: throw an exception!!
        return;
    }
}
