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


const char* AndorSDK_Exception::what() const noexcept
{
    return msg.c_str();
}
