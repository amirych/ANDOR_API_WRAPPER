#ifndef ANDORSDK_EXCEPTION_H
#define ANDORSDK_EXCEPTION_H

#include <string>
#include <exception>
#include "atcore.h"

class AndorSDK_Exception : public std::exception
{
public:
    AndorSDK_Exception(int err_code, const std::string &context = std::string());

    int getError() const;

    const char* what() const noexcept;

private:
    std::string msg;
    int errCode;
};


inline void andor_sdk_assert(int err, const std::string& context = std::string())
{
    if ( err != AT_SUCCESS ) {
        throw AndorSDK_Exception(err, context);
    }
};


#endif // ANDORSDK_EXCEPTION_H
