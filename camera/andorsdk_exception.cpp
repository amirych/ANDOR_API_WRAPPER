#include "andorsdk_exception.h"
//#include "atcore.h"

AndorSDK_Exception::AndorSDK_Exception(int err_code, const std::string &context):
    exception(), msg(context), errCode(err_code)
{

}


int AndorSDK_Exception::getError() const
{
    return errCode;
}

#ifdef _MSC_VER
#if (_MSC_VER > 1800)
const char* AndorSDK_Exception::what() const noexcept
#else
const char* AndorSDK_Exception::what() const
#endif
#else
const char* AndorSDK_Exception::what() const noexcept
#endif
{
    return msg.c_str();
}
