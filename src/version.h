/*
 * File generated automatically - do not edit.
 * Edit ..\setversion.py instead.
 */

#ifndef WEBPWICCODEC_VERSION_H
#define WEBPWICCODEC_VERSION_H

#define FILE_VERSION_MAJOR 0
#define FILE_VERSION_MINOR 10
#define FILE_VERSION_MAJOR_STR "0"
#define FILE_VERSION_MINOR_STR "10"
#define PRODUCT_VERSION_MAJOR 0
#define PRODUCT_VERSION_MINOR 10
#define PRODUCT_VERSION_MAJOR_STR "0"
#define PRODUCT_VERSION_MINOR_STR "10"

// The build id defaults to 0, but other can be specified as an MSBuild
// parameter with /p:CommandLineRCDefines='VERSION_BUILD=1;VERSION_BUILD_STR="1"'.
// Such builds won't be marked as private.
#ifndef VERSION_BUILD
#define FILE_VERSION_BUILD 0
#define FILE_VERSION_BUILD_STR "0"
#define PRODUCT_VERSION_BUILD 0
#define PRODUCT_VERSION_BUILD_STR "0"
#define VER_PRIVATE VS_FF_PRIVATEBUILD
#else
#define FILE_VERSION_BUILD VERSION_BUILD
#define FILE_VERSION_BUILD_STR VERSION_BUILD_STR
#define PRODUCT_VERSION_BUILD VERSION_BUILD
#define PRODUCT_VERSION_BUILD_STR VERSION_BUILD_STR
#define VER_PRIVATE 0
#endif

#ifdef _DEBUG
#define VER_DEBUG VS_FF_DEBUG
#else
#define VER_DEBUG 0
#endif

#endif  /* WEBPWICCODEC_VERSION_H */
