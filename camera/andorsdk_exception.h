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

#ifdef _MSC_VER
#if (_MSC_VER > 1800)
    const char* what() const noexcept;
#else
    const char* what() const; // to compile with VS2013
#endif
#else
    const char* what() const noexcept;
#endif
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
