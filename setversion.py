#!/usr/bin/python
#
# Script to generate version information files for both C++ and MSI.

import os

VERSION_MAJOR=0
VERSION_MINOR=10

version_h = file('src\\version.h', 'w')
version_h.writelines([
    '/*\n',
    ' * File generated automatically - do not edit.\n',
    ' * Edit ..\setversion.py instead.\n',
    ' */\n',
    '\n',
    '#ifndef WEBPWICCODEC_VERSION_H\n',
    '#define WEBPWICCODEC_VERSION_H\n',
    '\n',
    '#define FILE_VERSION_MAJOR %d\n' % VERSION_MAJOR,
    '#define FILE_VERSION_MINOR %d\n' % VERSION_MINOR,
    '#define FILE_VERSION_MAJOR_STR "%d"\n' % VERSION_MAJOR,
    '#define FILE_VERSION_MINOR_STR "%d"\n' % VERSION_MINOR,
    '#define PRODUCT_VERSION_MAJOR %d\n' % VERSION_MAJOR,
    '#define PRODUCT_VERSION_MINOR %d\n' % VERSION_MINOR,
    '#define PRODUCT_VERSION_MAJOR_STR "%d"\n' % VERSION_MAJOR,
    '#define PRODUCT_VERSION_MINOR_STR "%d"\n' % VERSION_MINOR,
    '\n',
    '#endif  /* WEBPWICCODEC_VERSION_H */\n',
    ])
version_h.close()

version_wxs = file('setup\\version.wxs', 'w')
version_wxs.writelines([
    '<!-- File generated automatically - do not edit. -->\n',
    '<!-- Edit ..\setversion.py instead. -->\n',
    '<Include>\n',
    '  <?define version_major="%d"?>\n' % VERSION_MAJOR,
    '  <?define version_minor="%d"?>\n' % VERSION_MINOR,
    '</Include>\n',
    ])
version_wxs.close()
