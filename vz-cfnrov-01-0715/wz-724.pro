#-------------------------------------------------
#
# Project created by QtCreator 2013-09-24T17:03:18
# wz-724 - Underwater Cleaner ROV Upper Computer
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets network svg concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = wz-724
TEMPLATE = app
CONFIG += c++11
#CONFIG += console

#-------------------------------------------------
# Platform-specific defines
#-------------------------------------------------

win32: DEFINES += WIN32 _WINDOWS _USE_MATH_DEFINES
DEFINES += _USE_MATH_DEFINES
DEFINES += MAVLINK_ALIGNED_FIELDS=0

win32:CONFIG(release, debug|release):    DEFINES += NDEBUG
else:win32:CONFIG(debug, debug|release): DEFINES += _DEBUG

#-------------------------------------------------
# Include paths
#-------------------------------------------------

INCLUDEPATH += $$PWD \
               $$PWD/UI \
               $$PWD/mavlink

#-------------------------------------------------
# SDL2 - platform conditional
#-------------------------------------------------

win32 {
    contains(QT_ARCH, x86_64) {
        SDL2_PATH = $$PWD/thirdparty/sdl2/SDL2-2.30.10/x86_64-w64-mingw32
        SDL2_DLL = $$SDL2_PATH/bin/SDL2.dll
    } else {
        SDL2_PATH = $$PWD/thirdparty/sdl2/x86
        SDL2_DLL = $$SDL2_PATH/SDL2.dll
    }
    INCLUDEPATH += $$SDL2_PATH/include
    LIBS += -L$$SDL2_PATH/lib -lSDL2main -lSDL2
}

unix {
    # Linux: use system SDL2 (pkg-config)
    CONFIG += link_pkgconfig
    PKGCONFIG += sdl2
}

#-------------------------------------------------
# FFmpeg - platform conditional
#-------------------------------------------------

win32 {
    contains(QT_ARCH, x86_64) {
        FFMPEG_PATH = $$PWD/thirdparty/ffmpeg/x64
        FFMPEG_DLLS = avcodec-62.dll avdevice-62.dll avfilter-11.dll avformat-62.dll avutil-60.dll swresample-6.dll swscale-9.dll
    } else {
        FFMPEG_PATH = $$PWD/thirdparty/ffmpeg/x86
        FFMPEG_DLLS = avcodec-60.dll avdevice-60.dll avfilter-9.dll avformat-60.dll avutil-58.dll postproc-57.dll swresample-4.dll swscale-7.dll
    }
    INCLUDEPATH += $$FFMPEG_PATH/include
    LIBS += -L$$FFMPEG_PATH/lib \
                -lavcodec \
                -lavdevice \
                -lavfilter \
                -lavformat \
                -lavutil \
                -lswresample \
                -lswscale
}

unix {
    # Linux: use system FFmpeg (pkg-config)
    CONFIG += link_pkgconfig
    PKGCONFIG += libavformat libavcodec libavdevice libavfilter libavutil libswresample libswscale
}

#-------------------------------------------------
# MAVLink
#-------------------------------------------------

INCLUDEPATH += $$PWD/thirdparty/mavlink/v2.0/ardupilotmega

#-------------------------------------------------
# Sources
#-------------------------------------------------

HEADERS += \
    UI/LayoutSquare.hpp \
    MainWindow.hpp \
    sdl2gamepad.h \
    UI/WidgetADI.hpp \
    UI/WidgetALT.hpp \
    UI/WidgetASI.hpp \
    UI/WidgetHSI.hpp \
    UI/WidgetNAV.hpp \
    UI/WidgetPFD.hpp \
    UI/WidgetTC.hpp \
    UI/WidgetVSI.hpp \
    Adi.hpp \
    Alt.hpp \
    Asi.hpp \
    Hsi.hpp \
    Nav.hpp \
    Pfd.hpp \
    Tc.hpp \
    Vsi.hpp \
    UI/WidgetSix.hpp \
    activemap.h \
    attitude3dwidget.h \
    dialogsetup.h \
    ffmpegrtspplayer.h \
    joystickthumbpad.h \
    keyedit.h \
    mavlink/mavlink_manager.h \
    udphandler.h \
    virtualjoystick.h \
    slidetounlock.h \
    rovdashboardwidgets.h \
    pwmdisplaywidget.h

SOURCES += \
    UI/LayoutSquare.cpp \
    attitude3dwidget.cpp \
    ffmpegrtspplayer.cpp \
    joystickthumbpad.cpp \
    keyedit.cpp \
    main.cpp \
    MainWindow.cpp \
    sdl2gamepad.cpp \
    UI/WidgetADI.cpp \
    UI/WidgetALT.cpp \
    UI/WidgetASI.cpp \
    UI/WidgetHSI.cpp \
    UI/WidgetNAV.cpp \
    UI/WidgetPFD.cpp \
    UI/WidgetTC.cpp \
    UI/WidgetVSI.cpp \
    Adi.cpp \
    Alt.cpp \
    Asi.cpp \
    Hsi.cpp \
    Nav.cpp \
    Pfd.cpp \
    Tc.cpp \
    Vsi.cpp \
    UI/WidgetSix.cpp \
    dialogsetup.cpp \
    mavlink/mavlink_manager.cpp \
    udphandler.cpp \
    virtualjoystick.cpp \
    slidetounlock.cpp \
    rovdashboardwidgets.cpp \
    pwmdisplaywidget.cpp

FORMS += \
    MainWindow.ui \
    UI/WidgetADI.ui \
    UI/WidgetALT.ui \
    UI/WidgetASI.ui \
    UI/WidgetHSI.ui \
    UI/WidgetNAV.ui \
    UI/WidgetPFD.ui \
    UI/WidgetTC.ui \
    UI/WidgetVSI.ui \
    UI/WidgetSix.ui \
    dialogsetup.ui

RESOURCES += \
    models.qrc \
    pic.qrc \
    qfi.qrc \
    theme.qrc

#-------------------------------------------------
# Win32 specific: output directory and DLL deployment
#-------------------------------------------------

win32 {
        CONFIG(release, debug|release) {
            DESTDIR = $$OUT_PWD/release
        } else {
            DESTDIR = $$OUT_PWD/debug
        }

        # Copy FFmpeg DLLs to output directory
        for(dll, FFMPEG_DLLS) {
            QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$FFMPEG_PATH/bin/$$dll) $$shell_path($$DESTDIR) $$escape_expand(\n\t)
        }

        # Copy SDL2 DLL to output directory
        QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$SDL2_DLL) $$shell_path($$DESTDIR) $$escape_expand(\n\t)

        # Standalone OpenCASCADE-based STEP-to-mesh converter used by the
        # "添加模型" button. It is bundled so target PCs need no CAD/Python install.
        STEP_CONVERTER = $$PWD/tools/step_converter_dist/wz-step-converter.exe
        exists($$STEP_CONVERTER) {
            QMAKE_POST_LINK += $$QMAKE_COPY \"$$shell_path($$STEP_CONVERTER)\" \"$$shell_path($$DESTDIR)\" $$escape_expand(\n\t)
        }

        # windeployqt on this machine may select a 32-bit compiler runtime.
        # Copy the runtime shipped with the selected Qt kit explicitly.
        contains(QT_ARCH, x86_64) {
            MINGW_RUNTIME_DLLS = libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll
            for(dll, MINGW_RUNTIME_DLLS) {
                # xcopy treats the '+' characters in libstdc++-6.dll as a
                # normal filename; cmd.exe's built-in copy command does not.
                QMAKE_POST_LINK += xcopy /Y /D \"$$shell_path($$[QT_INSTALL_BINS]/$$dll)\" \"$$shell_path($$DESTDIR)\\\" $$escape_expand(\n\t)
            }
        }
}
