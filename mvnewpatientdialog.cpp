#include "mvnewpatientdialog.h"
#include "ui_mvnewpatientdialog.h"

#include <QKeyEvent>
#include <QTextCharFormat>
#include <QtWidgets/QCalendarWidget>
#include <QtWidgets/QMessageBox>

#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace mv
{
    mvNewPatientDialog::mvNewPatientDialog(QWidget *parent) :
        QDialog(parent),
        modifyMode(false),
        ui(new Ui::mvNewPatientDialog)
    {
        ui->setupUi(this);
        this->setWindowFlags(Qt::FramelessWindowHint);
        this->setObjectName("mNewPatientDialog");

        mPatientID = "";
        mStudyUID = "";
        mProtocal = "";

        initView();
        initConnect();

    }

    mvNewPatientDialog::~mvNewPatientDialog()
    {
        delete ui;
    }

    void mvNewPatientDialog::initView()
    {
        // ui->lineEditInnerDiameter->setValidator(new QIntValidator(0, 150, this));
        QCalendarWidget *pCalendarWidget = ui->StudyDateEdit->calendarWidget();
        QTextCharFormat f = pCalendarWidget->weekdayTextFormat(Qt::Monday);//获取周一字体格式
        f.setForeground(QBrush(QColor("#00FA9A")));
        QTextCharFormat weekdays = f;
        ui->StudyDateEdit->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, weekdays);
        ui->StudyDateEdit->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, weekdays);

        auto validator = [](QLineEdit *edit, QString patten) {
            QRegularExpression reg(patten);
            QRegularExpressionValidator *v = new QRegularExpressionValidator;
            v->setRegularExpression(reg);
            edit->setValidator(v);
        };

        ui->projectIdEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
        validator(ui->projectIdEdit, QString("^[A-Za-z\\d]{1,20}$"));

        // QRegularExpression regExp("[0-9]{1,3}");
        // ui->lineEditInnerDiameter->setValidator(new QRegularExpressionValidator(regExp, this));
        ui->StudyDateEdit->setCalendarPopup(true);  // 日历弹出

        ui->StudyDateEdit->setDate(QDate::currentDate());
        ui->StudyDateEdit->setDisplayFormat("yyyy-MM-dd");

        setTabOrder(ui->projectNameEdit, ui->projectIdEdit); // a to b
        ui->projectNameEdit->setFocus();

        ui->ExamButton->setText(tr("Start Exam"));
        ui->SaveButton->setText(tr("Save"));
        ui->CancelButton->setText(tr("Cancel"));

        ui->projectNameEdit->installEventFilter(this);
        ui->projectIdEdit->installEventFilter(this);
        ui->lineEditInnerDiameter->installEventFilter(this);
        ui->lineEditWallThickness->installEventFilter(this);
        ui->lineEditEccentric->installEventFilter(this);
        ui->lineEditInnerDiameterTolerance->installEventFilter(this);
        ui->hotOuterDiameterTolerance->installEventFilter(this);
        ui->lineEditWallThicknessTolerance->installEventFilter(this);
        ui->lineEditEccentricTolerance->installEventFilter(this);
        ui->studyDescEdit->installEventFilter(this);

        ui->projectNameEdit->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->projectIdEdit->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->lineEditInnerDiameter->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->lineEditWallThickness->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->lineEditEccentric->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->lineEditInnerDiameterTolerance->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->hotOuterDiameterTolerance->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->lineEditWallThicknessTolerance->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->lineEditEccentricTolerance->setAttribute(Qt::WA_AcceptTouchEvents);
        ui->studyDescEdit->setAttribute(Qt::WA_AcceptTouchEvents);

    }

    void mvNewPatientDialog::initConnect()
    {
        connect(ui->CancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));
        connect(ui->SaveButton, SIGNAL(clicked()), this, SLOT(confirmButtonClicked()));
        connect(ui->ExamButton, SIGNAL(clicked()), this, SLOT(startExamButtonClicked()));

    }

    bool mvNewPatientDialog::eventFilter(QObject* obj, QEvent* event)
    {
        bool ret = true;
        if (event->type() == QEvent::Show)
        {
            clearData();
        }
        else if ((obj == ui->projectNameEdit) && (event->type() == QEvent::TouchBegin))
        {

        }
        else if (obj == ui->projectIdEdit && (event->type() == QEvent::TouchBegin))
        {
        }
        else if (obj == ui->lineEditInnerDiameter && (event->type() == QEvent::TouchBegin))
        {
        }
        else if (obj == ui->lineEditWallThickness && (event->type() == QEvent::TouchBegin))
        {
        }
        else if (obj == ui->lineEditEccentric && (event->type() == QEvent::TouchBegin))
        {

        }
        else if ((obj == ui->studyDescEdit) && (event->type() == QEvent::TouchBegin))
        {

        }
        ret = QWidget::eventFilter(obj, event);     //调用父类事件过滤器

        return ret;
    }

    void mvNewPatientDialog::showEvent(QShowEvent *)
    {
        if (!modifyMode)
        {
            this->clearData();
            bool isAutoPID = false;
            int PIDFrom = 0;
            int Patient_ID_Digits = 0;
            QString Patient_ID_prefix = "";
            QString Patient_ID_Suffix = "";

            bool isAutoAccession = false;
            int AccesionFrom = 0;
            int Accession_Number_Digits = 0;
            QString Accession_Number_prefix = "";
            QString Accession_Number_Suffix = "";

            XmlOptionFile options = this->getLocalSettings();
            QDomNodeList presetNodeList = options.getElement().elementsByTagName("Data");
            for (int i = 0; i < presetNodeList.count(); ++i)
            {
                QDomNode node = presetNodeList.at(i);
                QString textname = node.toElement().attribute("key");
                if (textname == "BRIDSCFS")
                {
                    QString style = node.toElement().attribute("value");
                    if (style == "自动" || style == "Auto")
                    {
                        isAutoPID = true;
                    }
                }
                else if (textname == "JCLSHSCFS")
                {
                    QString style = node.toElement().attribute("value");
                    if (style == "自动" || style == "Auto")
                    {
                        isAutoAccession = true;
                    }
                }
                else if (textname == "Patient_ID_Start_Form")
                {
                    QString style = node.toElement().attribute("value");
                    PIDFrom = style.toInt();
                }
                else if (textname == "Patient_ID_Digits")
                {
                    QString style = node.toElement().attribute("value");
                    Patient_ID_Digits = style.toInt();
                }
                else if (textname == "Patient_ID_prefix")
                {
                    Patient_ID_prefix = node.toElement().attribute("value");
                }
                else if (textname == "Patient_ID_Suffix")
                {
                    Patient_ID_Suffix = node.toElement().attribute("value");
                }
                else if (textname == "Accession_Number_Start_Form")
                {
                    QString style = node.toElement().attribute("value");
                    AccesionFrom = style.toInt();
                }
                else if (textname == "Accession_Number_Digits")
                {
                    QString style = node.toElement().attribute("value");
                    Accession_Number_Digits = style.toInt();
                }
                else if (textname == "Accession_Number_prefix")
                {
                    Accession_Number_prefix = node.toElement().attribute("value");
                }
                else if (textname == "Accession_Number_Suffix")
                {
                    Accession_Number_Suffix = node.toElement().attribute("value");
                }
            }

            mPatientID = QString("%1%2%3").arg(Patient_ID_prefix)
                .arg(PIDFrom, Patient_ID_Digits, 10, QChar('0'))
                .arg(Patient_ID_Suffix);

            QDateTime currentTime = QDateTime::currentDateTime();
            QString timeString = currentTime.toString("yyyyMMddhhmmss");
            ui->SerialNumber->setText(timeString);

            if (isAutoPID)
            {

            }

            if (isAutoAccession)
            {
//                QString currentDate = QDate::currentDate().toString("yyyyMMdd");
//                ui->SerialNumber->setText(QString("%1%2%3%4").arg(Accession_Number_prefix)
//                    .arg(currentDate)
//                    .arg(AccesionFrom, Accession_Number_Digits, 10, QChar('0'))
//                    .arg(Accession_Number_Suffix));
            }
        }
    }

    XmlOptionFile mvNewPatientDialog::getLocalSettings()
    {
        QString databaseDirectory = DataLocations::getRootConfigPath() + "/Conf";
        QString filename = databaseDirectory + "/OtherSysSettings.xml";
        return  XmlOptionFile(filename);
    }

    void mvNewPatientDialog::acceptDialog(bool addResult)
    {
        if (addResult)
        {
            clearData();
        }
    }

    void mvNewPatientDialog::autoFillData()
    {
        QDate dateTime = ui->StudyDateEdit->date();

        if (dateTime.isNull() || !dateTime.isValid())
        {
            XDMessageBox::showError(NULL, "", tr("datetime can not be empty"), QMessageBox::Yes);
            return;
        }

        study.accNumber = ui->lineEditEccentric->text().trimmed();
        if (ui->projectIdEdit->text().isEmpty())
        {
            ui->projectIdEdit->setText(mPatientID.trimmed());
            study.patientId = mPatientID.trimmed();
        }
        else
        {
            study.patientId = ui->projectIdEdit->text();
        }
        if (ui->projectNameEdit->text().isEmpty())
        {
            ui->projectNameEdit->setText(tr("Anonymous_") + mPatientID.trimmed());
            study.patientName = tr("Anonymous_") + mPatientID.trimmed();
        }
        else
        {
            study.patientName = ui->projectNameEdit->text().trimmed();
        }

        if (study.patientName.isEmpty())
        {
            study.patientName = tr("Emergence Patient");
        }

        QString mode = "";

        study.patientSex = mode;
        study.patientBirth = ui->lineEditInnerDiameter->text();
        study.age = ui->hotOuterDiameter->text();
        study.studyDate = ui->StudyDateEdit->date().toString("yyyy-MM-dd");
        study.reqPhysician = ui->lineEditWallThickness->text();
        study.perPhysician = ui->lineEditEccentric->text();
        study.accNumber = ui->SerialNumber->text();

        study.procId = ui->lineEditInnerDiameterTolerance->text();
        study.protocalCode = ui->hotOuterDiameterTolerance->text();
        study.protocalMeaning = ui->lineEditWallThicknessTolerance->text();
        study.modality = ui->lineEditEccentricTolerance->text();

        study.studyId = ui->SerialNumber->text();
        study.studyTime = QTime::currentTime().toString("hh:mm:ss");
        study.studyDesc = ui->studyDescEdit->toPlainText();
        study.studyUID = this->mStudyUID;
    }
    void mvNewPatientDialog::confirmButtonClicked()
    {

        autoFillData();

        if (!modifyMode)
        {
            emit setNewPatient1(study, true);
        }
        else
        {
            emit setNewPatient1(study, false);
        }
    }

    void mvNewPatientDialog::startExamButtonClicked()
    {

        autoFillData();

        emit setNewPatientStartExam(study);
    }

    void mvNewPatientDialog::cancelButtonClicked()
    {
        emit sigSwitchPatMagnPage(2);
    }

    void mvNewPatientDialog::clearData()
    {
        mPatientID = "";

        ui->lineEditEccentric->clear();
        ui->projectNameEdit->clear();
        ui->projectIdEdit->clear();
        ui->lineEditWallThickness->clear();
        ui->hotOuterDiameter->clear();
        ui->lineEditInnerDiameter->clear();
        ui->StudyDateEdit->setDate(QDate::currentDate());
        ui->studyDescEdit->clear();
        ui->lineEditInnerDiameterTolerance->clear();
        ui->hotOuterDiameterTolerance->clear();
        ui->lineEditWallThicknessTolerance->clear();
        ui->lineEditEccentricTolerance->clear();

    }

    void mvNewPatientDialog::setModifyMode(bool mode)
    {
        this->modifyMode = mode;
        if (this->modifyMode)
        {
            ui->ExamButton->setVisible(false);
            ui->projectIdEdit->setEnabled(false);
            ui->SerialNumber->setEnabled(false);
        }
        else
        {
            ui->ExamButton->setVisible(true);
            ui->projectIdEdit->setEnabled(true);
            ui->SerialNumber->setEnabled(true);

        }
    }

    void mvNewPatientDialog::setProtocalState(QString protocal)
    {
        this->mProtocal = protocal;
    }

    void mvNewPatientDialog::setPatientInfo(QString patientID, QString name, QString sex, QString bitthDate, QString age, QString accNumber, QString studyID, QString procID,QString procCode,QString procMeaning,QString reqPhysician, QString perPhysician, QString studyDate, QString studyTime, QString Modality, QString studyDesc, QString studyUID)
    {

        ui->projectNameEdit->setFocus();
        ui->projectIdEdit->setText(patientID);
        mPatientID = patientID;
        ui->projectNameEdit->setText(name);

        ui->lineEditInnerDiameter->setText(bitthDate);
        ui->hotOuterDiameter->setText(age);

        ui->lineEditEccentric->setText(perPhysician);
        ui->lineEditWallThickness->setText(reqPhysician);
        ui->SerialNumber->setText(accNumber);
        ui->StudyDateEdit->setDate(QDate::fromString(studyDate, "yyyy-MM-dd"));

        ui->lineEditInnerDiameterTolerance->setText(procID);
        ui->hotOuterDiameterTolerance->setText(procCode);
        ui->lineEditWallThicknessTolerance->setText(procMeaning);
        ui->lineEditEccentricTolerance->setText(Modality);

        ui->studyDescEdit->setPlainText(studyDesc);

        this->mStudyUID = studyUID;

        setModifyMode(true);

    }



}
