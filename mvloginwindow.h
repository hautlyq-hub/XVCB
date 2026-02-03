#ifndef CXLOGINWINDOW_H
#define CXLOGINWINDOW_H

#include "XV.Control/XDCustomWindow.h"
#include "XV.DataAccess/mvdicomdatabase.h"
#include "XV.Model/mvstudyrecord.h"
#include "XV.Model/mvworklistitemmodel.h"
#include "XV.Tool/mvSessionHelper.h"
#include "XV.Tool/mvSimpleCrypto.h"
#include "XV.Tool/mvXmlOptionItem.h"
#include "XV.Tool/mvdatalocations.h"
#include "XV.Tool/mvharddiskinformation.h"
#include "ui_mvloginwindow.h"

#include <QBitmap>
#include <QImage>
#include <QMap>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtXml/QDomDocument>

#include <QGuiApplication>
#include <QScreen>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QUuid>
#include <QtCore/QDir>
#include <QtMath>
#include <QtWidgets/QApplication>
#include <QtWidgets/QListView>

namespace mv
{

	class mvLoginWindow : public QDialog
	{
		Q_OBJECT
	public :


	public:
		mvLoginWindow(QWidget *parent = 0);
		~mvLoginWindow();
		static mvLoginWindow* getInstance(); ///< returns the only instance of this class
		static mvLoginWindow *mInstance;

        void initView();
        void initConnect();

        void readSettings();
        void writeSettings();

        int checkStateHardDisk(QString sPath);

    protected:
        bool eventFilter(QObject *target, QEvent *event) override;
        void paintEvent(QPaintEvent *event);
        void closeEvent(QCloseEvent * event);

        void mouseDoubleClickEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);

    signals:
        void emitLoginInfo(QStringList list);

   protected slots:
        void loginButton();

	private:
        Ui::mvLoginWindow ui;

        bool mIsPressed;

        QPoint mStartMovePos;

        QString m_InUser;
        QString m_InUsers;
        QStringList m_InUserList;
        QString m_DisplayName;
        QString m_InPswd ;

	};
}
#endif // CXLOGINWINDOW_H
