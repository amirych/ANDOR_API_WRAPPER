#ifndef EXPORT_DECL_H
#define EXPORT_DECL_H


#ifdef _WIN32
#  define ANDOR_API_WRAPPER_EXPORT __declspec( dllexport )
#else
#  define ANDOR_API_WRAPPER_EXPORT
#endif


#endif // EXPORT_DECL_H
