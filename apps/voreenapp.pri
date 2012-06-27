####################################################
# Generic project file for all Voreen applications
####################################################

# common Qt resource files
qt : RESOURCES = "$${VRN_HOME}/resource/vrn_share/vrn_app.qrc"

qt : PRECOMPILED_HEADER = ../pch_qtapp.h

####################################################
# Platform-dependant configuration
####################################################

win32 {
  CONFIG(debug, debug|release) {
    MSC_CONFIG = Debug
  } else {
    MSC_CONFIG = Release
  }

  visual_studio {
    # Contains "Release" or "Debug" as selected in the IDE
    MSC_CONFIG = $(ConfigurationName)
  }

  QMAKE_LIBDIR += "$${VRN_HOME}/ext/glew/lib"

  win32-msvc {
    # these libs are not automatically added when building with nmake
    nmake: LIBS *= advapi32.lib shell32.lib

    qt: LIBS += "$${VRN_HOME}/$${MSC_CONFIG}/voreen_qt.lib" -lqtmain

    LIBS += "$${VRN_HOME}/$${MSC_CONFIG}/voreen_core.lib"
  }

  win32-g++ {
    qt: LIBS += "$${VRN_HOME}/$${MSC_CONFIG}/libvoreen_qt.a"
    LIBS += "$${VRN_HOME}/$${MSC_CONFIG}/libvoreen_core.a" \
  }

  LIBS += "$${VRN_HOME}/ext/glew/lib/$${MSC_CONFIG}/glew32s.lib"

  # no libc for vc++ since we build a multithreaded executable
  win32-msvc {
    LIBS += /NODEFAULTLIB:libc.lib
  }

  contains(DEFINES, VRN_WITH_DCMTK) {
    LIBS += "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/dcmimage.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/dcmimgle.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/dcmnet.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/dcmdata.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/dcmjpeg.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/ijg8.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/ijg12.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/ijg16.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/ofstd.lib" \
            "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/dcmtls.lib"

    contains(DEFINES, VRN_DCMTK_VERSION_355) {
         LIBS += "$${DCMTK_DIR}/lib/$${MSC_CONFIG}/oflog.lib" 
    }
  }

  contains(DEFINES, VRN_WITH_DEVIL) {
    LIBS += "$${DEVIL_DIR}/lib/DevIL.lib"
  }

  contains(DEFINES, VRN_WITH_MATLAB) {
    LIBS += "$${MATLAB_DIR}/extern/lib/win32/microsoft/libeng.lib" \
            "$${MATLAB_DIR}/extern/lib/win32/microsoft/libmat.lib" \
            "$${MATLAB_DIR}/extern/lib/win32/microsoft/libut.lib" \
            "$${MATLAB_DIR}/extern/lib/win32/microsoft/libmx.lib"
  }

  contains(DEFINES, VRN_WITH_TIFF) {
    LIBS += "$${TIFF_DIR}/lib/libtiff.lib"
  }

  contains(DEFINES, VRN_WITH_ZLIB) {
    LIBS += "$${ZLIB_DIR}/lib/zdll.lib"
  }	

  contains(DEFINES, VRN_WITH_PYTHON) {
    win32-msvc2005: LIBS += "$${VRN_HOME}/ext/python/lib/$${MSC_CONFIG}/VS2005/python26.lib"
    win32-msvc2008: LIBS += "$${VRN_HOME}/ext/python/lib/$${MSC_CONFIG}/python26.lib"
    win32-g++:      LIBS += "$${VRN_HOME}/ext/python/lib/$${MSC_CONFIG}/VS2005/python26.lib"
  }

  contains(DEFINES, VRN_WITH_FONTRENDERING) {
    win32-msvc: LIBS += "$${FREETYPE_DIR}/lib/freetype.lib"
    win32-g++:  LIBS += "$${FREETYPE_DIR}/lib/mingw/freetype.lib"
    win32-msvc: LIBS += "$${FTGL_DIR}/lib/ftgl.lib"
    win32-g++:  LIBS += "$${FTGL_DIR}/lib/mingw/ftgl.lib"
    INCLUDEPATH += "$${FTGL_DIR}/include"
  }

  contains(DEFINES, VRN_MODULE_HPMC) {
    LIBS += "$${HPMC_DIR}/lib/hpmc.lib"
  }

  contains(DEFINES, VRN_WITH_FFMPEG) {
    LIBS += "$${FFMPEG_DIR}/win32/avcodec.lib"
    LIBS += "$${FFMPEG_DIR}/win32/avdevice.lib"
    LIBS += "$${FFMPEG_DIR}/win32/avformat.lib"
    LIBS += "$${FFMPEG_DIR}/win32/avutil.lib"
    LIBS += "$${FFMPEG_DIR}/win32/swscale.lib"
  }
  
  contains(DEFINES, VRN_WITH_DCMTK) {
    LIBS += "$${VRN_HOME}/ext/openssl/lib/$${MSC_CONFIG}/dcmtkeay.lib"
    LIBS += "$${VRN_HOME}/ext/openssl/lib/$${MSC_CONFIG}/dcmtkssl.lib"
  }

  contains(DEFINES, VRN_WITH_BOX2D) {
    QMAKE_LIBDIR  += "$${BOX2D_DIR}/Library"
  }


  LIBS += -lnetapi32 -lopengl32 -lglu32

  # For reading file version, file date and making registry calls
  # via Windows API
  LIBS += -lVersion

  # Windows Management Instrumentation (WMI) for hardware detection
  contains(DEFINES, TGT_WITH_WMI) {
    LIBS += -lWbemUuid
  }

}

unix {
  DEFINES += LINUX

  !macx: LIBS += -lGL -lGLU
  LIBS += -lGLEW
  qt : LIBS += -lvoreen_qt
  LIBS += -lvoreen_core

  contains(DEFINES, VRN_WITH_DEVIL) {
    LIBS += -lIL
  }

  contains(DEFINES, VRN_WITH_TIFF) {
    LIBS += -ltiff
  }
  contains(DEFINES, VRN_WITH_ZLIB) {
    LIBS += -lz
  }
  contains(DEFINES, VRN_WITH_CLIBPDF) {
    LIBS += -lcpdf
  }
  contains(DEFINES, VRN_WITH_FONTRENDERING) {
    LIBS += -lfreetype -lftgl
  }
  contains(DEFINES, VRN_WITH_DCMTK) {

    LIBS += -lz -lssl
    !without_libwrap: LIBS += -lwrap

    LIBS += -ldcmimage -ldcmimgle -ldcmnet -ldcmdata \
            -ldcmjpeg -lijg8 -lijg12 -lijg16 -lofstd

    LIBS += -ldcmtls

    macx: LIBS += -lcrypto
  }
  
  contains(DEFINES, VRN_WITH_FFMPEG) {
    # It is important that this comes after linking the voreen_* libs.
    LIBS += -lbz2 -lavformat -lavcodec -lavutil -lswscale
  }

  contains(DEFINES, VRN_MODULE_HPMC) {
    LIBS += -lhpmc
  }

  contains(DEFINES, VRN_WITH_LZO) {
    LIBS += -llzo2
  }

  !isEmpty(INSTALL_PREFIX) {
    target.path = $$INSTALLPATH_BIN
    INSTALLS += target
  }
}

### Local Variables:
### mode:conf-unix
### End: