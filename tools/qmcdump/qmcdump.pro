
include(../../config.pri)

QT += qml quick

QMLCBASEPATH = ../../

INCLUDEPATH += $$QMLCBASEPATH/qmcloader $$QMLCBASEPATH/include
LIBS += -L$$QMLCBASEPATH/qmcloader
LIBS += -lqmcloader

# These are just for using JIT
QT += qml-private core-private
INCLUDEPATH += $$QMLCBASEPATH/3rdparty/masm
INCLUDEPATH += $$QMLCBASEPATH/3rdparty/masm/stubs
INCLUDEPATH += $$QMLCBASEPATH/3rdparty/masm/stubs/wtf
INCLUDEPATH += $$QMLCBASEPATH/3rdparty/masm/jit
INCLUDEPATH += $$QMLCBASEPATH/3rdparty/masm/disassembler
include($$QMLCBASEPATH/3rdparty/masm/masm-defs.pri)
DEFINES += ENABLE_JIT ASSERT_DISABLED=1

SOURCES += main.cpp

HEADERS +=

target.files = qmcdump
target.path = $$BINDIR

INSTALLS += target
