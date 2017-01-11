#include <iostream>

#include "camera/andor_camera.h"

int main()
{
    ANDOR_Camera cam;

    ANDOR_EnumFeature ef;

    ef = cam[L"AuxOutSourceTwo"];

    cam[L"AuxOutSourceTwo"] = L"sssss";
    cam[L"AuxOutSourceTwo"] = 10;

    std::wstring s;
    s = ef;

    cam[L"AuxOutSourceTwo"] = s;

    ANDOR_EnumFeature ef1 = cam[L"AuxOutSourceTwo"];

//    ANDOR_EnumFeature zz(s);
}
