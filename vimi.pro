SOURCES = \
    src/collection.cpp \
    src/config.cpp \
    src/main.cpp \
    src/videoframedumper.cpp
HEADERS = \
    src/collection.h \
    src/videoframedumper.h

QT += quick qml multimedia multimediawidgets widgets

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    qml/ControlBar.qml \
    qml/VideoElement.qml \
    qml/main.qml \
    qml/TagList.qml \
    qml/Toolbar.qml \

