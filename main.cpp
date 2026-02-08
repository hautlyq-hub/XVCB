#include "mainwindow.h"
#include "XV.Tool/mvconfig.h"
#include "XV.Tool/mvqss.h"

#include "XV.Control/XDCustomWindow.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QTranslator>
#include <QtDebug>
#include <QtWidgets/QApplication>
// 互斥锁（多线程安全）
static QMutex logMutex;

// 自定义消息处理函数
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&logMutex);

    // 获取当前时间
    QDateTime currentTime = QDateTime::currentDateTime();

    // 构建文件夹路径: Logs/年/月/
    QString year = currentTime.toString("yyyy");
    QString month = currentTime.toString("MM");
    QString logDir = QString("Logs/%1/%2/").arg(year).arg(month);

    // 确保目录存在
    QDir dir;
    if (!dir.exists(logDir)) {
        dir.mkpath(logDir);
    }

    // 构建文件名: 日.log (例如: 28.log)
    QString fileName = currentTime.toString("dd") + ".log";
    QString filePath = logDir + fileName;

    // 打开日志文件（追加模式）
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);

        // 时间戳: 时:分:秒.毫秒
        QString timestamp = currentTime.toString("hh:mm:ss.zzz");

        // 消息类型标签
        QString typeStr;
        switch (type) {
        case QtDebugMsg:
            typeStr = "D";
            break;
        case QtInfoMsg:
            typeStr = "I";
            break;
        case QtWarningMsg:
            typeStr = "W";
            break;
        case QtCriticalMsg:
            typeStr = "C";
            break;
        case QtFatalMsg:
            typeStr = "F";
            break;
        }

        // 写入日志: [类型][时间戳] 消息
        out << "[" << typeStr << "][" << timestamp << "] " << msg << "\n";
        out.flush();
        file.close();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageHandler);

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
