TEMPLATE	= app
CONFIG		+= qt
INCLUDEPATH	+= /usr/include/OpenEXR
LIBS		+= -ltiff -lpfs-1.2
HEADERS		+= Exposure.h
HEADERS		+= HdrMergeMainWindow.h
HEADERS		+= RenderThread.h
HEADERS		+= ImageControl.h
HEADERS		+= PreviewWidget.h
SOURCES		+= main.cpp
SOURCES		+= Exposure.cpp
SOURCES		+= HdrMergeMainWindow.cpp
SOURCES		+= RenderThread.cpp
SOURCES		+= ImageControl.cpp
SOURCES		+= PreviewWidget.cpp

TARGET = hdrmerge

