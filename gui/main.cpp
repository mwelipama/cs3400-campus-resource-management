#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <string>

#include "MainWindow.h"

// Finds the data folder in a way that works both from the development tree and
// from the portable release folder: prefer <exe dir>/data, then <exe dir>/../data,
// then the current working directory's data/. This keeps the app free of any
// absolute path from the build machine.
static std::string resolveDataDir() {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        exeDir + "/data",
        exeDir + "/../data",
        exeDir + "/../../data",
        QDir::currentPath() + "/data"
    };
    for (const QString& path : candidates)
        if (QFileInfo::exists(path + "/resources.txt"))
            return QDir(path).absolutePath().toStdString();
    // Default to a data/ next to the executable (new, empty installs save here).
    return QDir(exeDir + "/data").absolutePath().toStdString();
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("CS3400 Campus Resource Manager");
    MainWindow window(resolveDataDir());
    window.show();
    return app.exec();
}
