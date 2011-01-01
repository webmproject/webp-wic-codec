Windows Imaging Component wrapper around the libvpx codec for WebP support.
Currently, only decoding is supported, but that allows to e.g., see the files
in Windows PhotoViewer.

Directories:
example - an example webp file.
libvpx - binary versions of libvpx, both debug and release, x64 and x86.
setup - WiX scripts to create MSI packages and a wrapper setup.
src - source code of the WIC interfaces.
test - tests comparing the WebP codec with the builtin Jpeg codec.

Directories created after a build
bin - place where generated DLLs, MSIs etc will be put.
ipch - precompiled headers by Visual Studio.
obj - place where generated object files etc, will be put.

The solution/project were created in Visual Studio 2010, I don't know if they
work with older versions. It requires Windows SDK 7.1 - the projects its
compiler, as the compiler in Visual C++ Express doesn't support x64.

The tests loads the decoder using CoCreateInstance - i.e. they choose the DLL
that is currently registered in the system registry - they don't automatically
choose the DLL from the active configuration.

To register a DLL. Run from an elevated command prompt (elevated command
prompt = choose command prompt icon, right click and choose 'Run As
Administrator') e.g.:

regsvr32 bin\x64\Debug\WebpWICCodec.dll

Note that the registry has different branches for 32-bit and 64-bit COM object
registrations, thus, you register independently a win32 and x64 DLL. On the
other hand registering e.g. a release DLL when a debug DLL for the same
architecture was previously registered should overwrite the previous
registration.

If you want to unregister a DLL, run e.g.:

regsvr32 /u bin\x64\Debug\WebpWICCodec.dll


The tests were written on Windows 7. Some of them may fail e.g., on Vista.

The setup consists of a x86 MSI package (with the x64 DLL), x64 MSI package
(with both DLLs) and a wrapper EXE that chooses the correct one. The MSIs are
created with WiX 3.0. Typing 'msbuild' should create both packages and the
wrapper.