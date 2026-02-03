# Install script for directory: /home/jessie/Documents/libxml2

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libxml2/libxml" TYPE FILE FILES
    "/home/jessie/Documents/libxml2/include/libxml/c14n.h"
    "/home/jessie/Documents/libxml2/include/libxml/catalog.h"
    "/home/jessie/Documents/libxml2/include/libxml/chvalid.h"
    "/home/jessie/Documents/libxml2/include/libxml/debugXML.h"
    "/home/jessie/Documents/libxml2/include/libxml/dict.h"
    "/home/jessie/Documents/libxml2/include/libxml/encoding.h"
    "/home/jessie/Documents/libxml2/include/libxml/entities.h"
    "/home/jessie/Documents/libxml2/include/libxml/globals.h"
    "/home/jessie/Documents/libxml2/include/libxml/hash.h"
    "/home/jessie/Documents/libxml2/include/libxml/HTMLparser.h"
    "/home/jessie/Documents/libxml2/include/libxml/HTMLtree.h"
    "/home/jessie/Documents/libxml2/include/libxml/list.h"
    "/home/jessie/Documents/libxml2/include/libxml/nanoftp.h"
    "/home/jessie/Documents/libxml2/include/libxml/nanohttp.h"
    "/home/jessie/Documents/libxml2/include/libxml/parser.h"
    "/home/jessie/Documents/libxml2/include/libxml/parserInternals.h"
    "/home/jessie/Documents/libxml2/include/libxml/pattern.h"
    "/home/jessie/Documents/libxml2/include/libxml/relaxng.h"
    "/home/jessie/Documents/libxml2/include/libxml/SAX.h"
    "/home/jessie/Documents/libxml2/include/libxml/SAX2.h"
    "/home/jessie/Documents/libxml2/include/libxml/schemasInternals.h"
    "/home/jessie/Documents/libxml2/include/libxml/schematron.h"
    "/home/jessie/Documents/libxml2/include/libxml/threads.h"
    "/home/jessie/Documents/libxml2/include/libxml/tree.h"
    "/home/jessie/Documents/libxml2/include/libxml/uri.h"
    "/home/jessie/Documents/libxml2/include/libxml/valid.h"
    "/home/jessie/Documents/libxml2/include/libxml/xinclude.h"
    "/home/jessie/Documents/libxml2/include/libxml/xlink.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlIO.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlautomata.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlerror.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlexports.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlmemory.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlmodule.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlreader.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlregexp.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlsave.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlschemas.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlschemastypes.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlstring.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlunicode.h"
    "/home/jessie/Documents/libxml2/include/libxml/xmlwriter.h"
    "/home/jessie/Documents/libxml2/include/libxml/xpath.h"
    "/home/jessie/Documents/libxml2/include/libxml/xpathInternals.h"
    "/home/jessie/Documents/libxml2/include/libxml/xpointer.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libxml2.so.16.2.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libxml2.so.16"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/jessie/Documents/libxml2/builddir/libxml2.so.16.2.0"
    "/home/jessie/Documents/libxml2/builddir/libxml2.so.16"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libxml2.so.16.2.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libxml2.so.16"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/jessie/Documents/libxml2/builddir/libxml2.so")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "programs" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/jessie/Documents/libxml2/builddir/xmllint")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint"
         OLD_RPATH "/home/jessie/Documents/libxml2/builddir:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmllint")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "programs" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/jessie/Documents/libxml2/builddir/xmlcatalog")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog"
         OLD_RPATH "/home/jessie/Documents/libxml2/builddir:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xmlcatalog")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2" TYPE FILE FILES "/home/jessie/Documents/libxml2/builddir/libxml2-config.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2" TYPE FILE FILES "/home/jessie/Documents/libxml2/builddir/libxml2-config-version.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2/libxml2-export.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2/libxml2-export.cmake"
         "/home/jessie/Documents/libxml2/builddir/CMakeFiles/Export/e84b245f3ece1c96ac9ea1f0afd37f4b/libxml2-export.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2/libxml2-export-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2/libxml2-export.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2" TYPE FILE FILES "/home/jessie/Documents/libxml2/builddir/CMakeFiles/Export/e84b245f3ece1c96ac9ea1f0afd37f4b/libxml2-export.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libxml2" TYPE FILE FILES "/home/jessie/Documents/libxml2/builddir/CMakeFiles/Export/e84b245f3ece1c96ac9ea1f0afd37f4b/libxml2-export-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libxml2/libxml" TYPE FILE FILES "/home/jessie/Documents/libxml2/builddir/libxml/xmlversion.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/jessie/Documents/libxml2/builddir/libxml-2.0.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/home/jessie/Documents/libxml2/builddir/xml2-config")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/jessie/Documents/libxml2/builddir/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/jessie/Documents/libxml2/builddir/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
