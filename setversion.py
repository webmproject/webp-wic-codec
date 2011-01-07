#!/usr/bin/python
#
# Script to generate version information files for both C++ and MSI.
#
# The build id and company name can be changed using command line flags, what is
# used during official builds.

import getopt
import os
import sys

VERSION_MAJOR=0
VERSION_MINOR=10
PRODUCT_NAME='WebP WIC Codec for Windows'
# These defaults can be changed using command line flags.
# Note that only builds made by Google should use Google as company name.
DEFAULT_COMPANY='Private open source build'
DEFAULT_BUILD=0

opts, args = getopt.gnu_getopt(sys.argv[1:], "", ["build=", "company="])
assert len(args) == 0
opts = dict(opts)
if opts.has_key('--build'):
    build = int(opts['--build'])
else:
    build = DEFAULT_BUILD
    
if opts.has_key('--company'):
    company = opts['--company']
else:
    company = DEFAULT_COMPANY

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
    '\n',
    '#define PRODUCT_NAME "%s"\n' % PRODUCT_NAME,
    '#define PRODUCT_COMPANY "%s"\n' % company,
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
    '#define FILE_VERSION_BUILD %d\n' % build,
    '#define FILE_VERSION_BUILD_STR "%d"\n' % build,
    '#define PRODUCT_VERSION_BUILD %d\n' % build,
    '#define PRODUCT_VERSION_BUILD_STR "%d"\n' % build,
    '\n'
    '// Builds with a set build id are considered non-private.\n'
    '#if FILE_VERSION_BUILD\n'
    '#define VER_PRIVATE 0\n'
    '#else\n'
    '#define VER_PRIVATE VS_FF_PRIVATEBUILD\n'
    '#endif\n'
    '\n'
    '#ifdef _DEBUG\n'
    '#define VER_DEBUG VS_FF_DEBUG\n'
    '#else\n'
    '#define VER_DEBUG 0\n'
    '#endif\n'
    '\n'
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
    '  <?define version_build="%d"?>\n' % build,
    '  <?define product_name="%s"?>\n' % PRODUCT_NAME,
    '  <?define company="%s"?>\n' % company,
    '</Include>\n',
    ])
version_wxs.close()
