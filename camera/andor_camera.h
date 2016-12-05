#ifndef ANDOR_CAMERA_H
#define ANDOR_CAMERA_H

#include "../export_decl.h"

#include <atcore.h>

#include <string>

using namespace std;

typedef int andor_enum_index_t; // type for ANDOR SDK enumerated feature index

class ANDOR_API_WRAPPER_EXPORT ANDOR_Camera
{
public:
    explicit ANDOR_Camera();
    virtual ~ANDOR_Camera();


protected:

                /*   DECLARATION OF A PROXY CLASS TO ACCESS ANDOR SDK FEATURES  */

    class ANDOR_Feature {
    public:
        ANDOR_Feature();
        ANDOR_Feature(const AT_H device_hndl, const std::wstring &name = std::wstring());
        ANDOR_Feature(const AT_H device_hndl, const wchar_t* name = nullptr);

        ANDOR_Feature(const std::wstring &name);
        ANDOR_Feature(const wchar_t* name);

        ~ANDOR_Feature();

        void setName(const std::wstring &name);
        void setName(const wchar_t* name);

        void setDeviceHndl(const AT_H hndl);

                // get feature value operators

                // set feature value operators
        template<typename T>
        typename std::enable_if<std::is_floating_point<T>::value, T&>::type
        operator=(const T &value);

        template<typename T>
        typename std::enable_if<(std::is_integral<T>::value && !std::is_same<T,bool>::value ), T&>::type
        operator=(const T &value);

    private:
        AT_H deviceHndl;
        AT_WC* featureName;
        std::wstring featureNameStr;
        union {
            AT_64 at_val64;  // integer feature
            AT_BOOL at_bool; // boolean feature
            double at_float; // floating-point feature
            andor_enum_index_t at_index; // index of enumerated feature
        };
        std::wstring at_string; // enumerated or string feature
    };

                /*  END OF ANDOR_Feature CLASS DECLARATION  */

};

#endif // ANDOR_CAMERA_H
