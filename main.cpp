#include "mainwindow.h"
#include "XV.Tool/mvconfig.h"
#include "XV.Tool/mvqss.h"

#include "XV.Control/XDCustomWindow.h"


#include <QtWidgets/QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "XVBVThickness_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QString titleStyle;
    QString buttonStyle;
    titleStyle = XV_MSG_TITLE_STYTLE_ONE;
    buttonStyle = XV_MSG_BOX_BUTTON_STYTLE_ONE;
    XDMessageBox::setTitleStyleSheet(titleStyle);
    XDMessageBox::setButtonStyleSheet(QDialogButtonBox::Ok, buttonStyle);

    MainWindow w;
    w.show();
    int retVal = a.exec();

    if (retVal == 666)
    {
        QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        return 0;
    }

    return retVal;
}
