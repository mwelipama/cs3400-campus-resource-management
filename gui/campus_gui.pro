# qmake project for the optional CS3400 desktop GUI.
# The GUI links the project's REAL data-structure / algorithm sources (one folder up)
# and never compiles the console entry point (main.cpp) or LibrarySystem.cpp.

QT       += widgets
CONFIG   += c++17
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = CampusResourceManager

# Headers and sources live one directory up (the graded project), unchanged.
INCLUDEPATH += $$PWD/..

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    GuiController.cpp \
    $$PWD/../Resource.cpp \
    $$PWD/../Request.cpp \
    $$PWD/../Tree.cpp \
    $$PWD/../HashTable.cpp \
    $$PWD/../Graph.cpp \
    $$PWD/../Searching.cpp \
    $$PWD/../Utils.cpp

HEADERS += \
    MainWindow.h \
    GuiController.h \
    $$PWD/../Resource.h \
    $$PWD/../Request.h \
    $$PWD/../Tree.h \
    $$PWD/../HashTable.h \
    $$PWD/../Graph.h \
    $$PWD/../Searching.h \
    $$PWD/../Sorting.h \
    $$PWD/../DynamicArray.h \
    $$PWD/../LinkedList.h \
    $$PWD/../Queue.h \
    $$PWD/../Stack.h \
    $$PWD/../PriorityQueue.h \
    $$PWD/../Utils.h
