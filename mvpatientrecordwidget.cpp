#include "mvpatientrecordwidget.h"
#include "ui_mvpatientrecordwidget.h"

namespace mv
{
	mvPatientRecordWidget::mvPatientRecordWidget(QWidget *parent) :
		QWidget(parent),
		ui(new Ui::mvPatientRecordWidget)
	{
		ui->setupUi(this);

        initView();
        initConnect();

	}

	mvPatientRecordWidget::~mvPatientRecordWidget()
	{
        delete wlistModel;
        delete wlistProxyModel;
        delete ui;
	}

    void mvPatientRecordWidget::initView()
    {
        qRegisterMetaType<WorklistItem>("WorklistItem");

        wlistModel = new WorklistItemModel(this);
        wlistProxyModel = new QSortFilterProxyModel(this);
        wlistProxyModel->setSourceModel(wlistModel);
        ui->RecordItemTableWidget->setModel(wlistProxyModel);

        ui->RecordItemTableWidget->setMouseTracking(true);
        ui->RecordItemTableWidget->setSortingEnabled(false);
        ui->RecordItemTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->RecordItemTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        ui->RecordItemTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch/*Interactive*/);
        ui->RecordItemTableWidget->horizontalHeader()->setStretchLastSection(true);
        ui->RecordItemTableWidget->horizontalHeader()->setMinimumHeight(40);
        ui->RecordItemTableWidget->verticalHeader()->setDefaultSectionSize(40);
//        ui->RecordItemTableWidget->verticalHeader()->setFixedWidth(80);
        ui->RecordItemTableWidget->setVerticalScrollMode(QTableView::ScrollPerPixel);
        QScroller::grabGesture(ui->RecordItemTableWidget, QScroller::LeftMouseButtonGesture);
        ui->RecordItemTableWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

        ui->RecordItemTableWidget->horizontalHeader()->hideSection(2);

        ui->RecordItemTableWidget->horizontalHeader()->hideSection(12);
        ui->RecordItemTableWidget->horizontalHeader()->hideSection(13);
        ui->RecordItemTableWidget->horizontalHeader()->hideSection(14);
        ui->RecordItemTableWidget->horizontalHeader()->hideSection(15);
        ui->RecordItemTableWidget->horizontalHeader()->hideSection(16);
        ui->RecordItemTableWidget->horizontalHeader()->hideSection(17);

        ui->FromDateEdit_2->setDate(QDate::currentDate().addMonths(-3));
        ui->FromDateEdit_2->setDisplayFormat("yyyy-MM-dd");
        ui->ToDateEdit_2->setDate(QDate::currentDate());
        ui->ToDateEdit_2->setDisplayFormat("yyyy-MM-dd");

        ui->CJ_State_2->setView(new QListView());

        ui->CJ_State_2->addItem(tr("All"));

        ui->remoteApplySearch->setCheckable(true);
        ui->mRemoteOrLocal->setCheckable(true);
        ui->mRemoteOrLocal2->setCheckable(true);
        ui->mRemoteOrLocal->setAutoExclusive(true);
        ui->mRemoteOrLocal2->setAutoExclusive(true);
        ui->mRemoteOrLocal->setChecked(true);
        ui->scheduleTimecheckBox_4->setChecked(true);

        ui->PIDLineEdit_2->installEventFilter(this);
        ui->AccessionLineEdit_3->installEventFilter(this);
        ui->PersonNameLineEdit_2->installEventFilter(this);
        ui->RecordItemTableWidget->installEventFilter(this);
        ui->mStackedWidget->setCurrentIndex(0);

       ui->btnRecallPat->setVisible(false);
       ui->btnImportImage->setVisible(false);
       ui->mExportImage->setVisible(false);
       ui->btnArchiveImage->setVisible(false);
       ui->btnDICOMDIR->setVisible(false);

    }

    void mvPatientRecordWidget::initConnect()
    {
        connect(ui->mRemoteOrLocal, SIGNAL(clicked(bool)), this, SLOT(onRemoteOrLocalClicked(bool)));
        connect(ui->mRemoteOrLocal2, SIGNAL(clicked(bool)), this, SLOT(onRemoteOrLocal2Clicked(bool)));
        connect(ui->localApplySearch, SIGNAL(clicked()), this, SLOT(onApplySearch()));
        connect(ui->remoteApplySearch, SIGNAL(clicked(bool)), this, SLOT(onGetScuSearch(bool)));

        connect(ui->mSwitchQueryPage, SIGNAL(clicked()), this, SLOT(onSwitchQueryPage()));
        connect(ui->mBtnClose, SIGNAL(clicked()), this, SLOT(onCloseQueryPage()));
        connect(ui->PIDLineEdit_2, SIGNAL(returnPressed()), this, SLOT(OnPidClicked()));
        connect(ui->AccessionLineEdit_3, SIGNAL(returnPressed()), this, SLOT(OnAccessionClicked()));
        connect(ui->mSwitchQueryPage, SIGNAL(clicked()), this, SLOT(onSwitchQueryPage()));
        connect(ui->scheduleTimecheckBox_2, SIGNAL(stateChanged(int)), this, SLOT(enableDateWidget_Today(int)));
        connect(ui->scheduleTimecheckBox_4, SIGNAL(stateChanged(int)), this, SLOT(enableDateWidget_All(int)));
        connect(ui->deletepatientpushButton, SIGNAL(clicked()), this, SLOT(deletePatientClicked()));
        connect(ui->editpatientpushButton, SIGNAL(clicked()), this, SLOT(openUpdatePatientClicked()));

        connect(ui->pushButtonCheckPatient, SIGNAL(clicked()), this, SLOT(onWlistBeginStudy()));
        connect(ui->PreviewpatientpushButton, SIGNAL(clicked()), this, SLOT(onPreviewBeginStudy()));

        connect(ui->FromDateEdit_2, &QDateEdit::dateChanged, [this](const QDate &date) {
            if (date.isValid())
            {
                QDate dateCur = QDate::currentDate();
                if (date > dateCur)
                {
                    ui->FromDateEdit_2->setDate(dateCur);
                }
            }
        });
        connect(ui->ToDateEdit_2, &QDateEdit::dateChanged, [this](const QDate &date) {
            if (date.isValid())
            {
                QDate dateCur = QDate::currentDate();
                if (date > dateCur)
                {
                    ui->ToDateEdit_2->setDate(dateCur);
                }
            }
        });
    }

    void mvPatientRecordWidget::showEvent(QShowEvent *event)
    {

    }

    bool mvPatientRecordWidget::eventFilter(QObject* obj, QEvent* e)
    {
        bool ret = true;

        if ((obj == ui->PIDLineEdit_2) && (e->type() == QEvent::MouseButtonPress))
        {

        }
        else if (obj == ui->AccessionLineEdit_3 && (e->type() == QEvent::MouseButtonPress))
        {

        }
        else if (obj == ui->PersonNameLineEdit_2 && (e->type() == QEvent::MouseButtonPress))
        {

        }
        else if ((obj == ui->RecordItemTableWidget) && (e->type() == QEvent::FocusIn))
        {
            if (ui->mStackedWidgetControl->currentIndex() == 1)
                onResetTableWidget();
            if (!ui->RecordItemTableWidget->currentIndex().isValid() && ui->RecordItemTableWidget->model()->rowCount() > 0)
            {
                ui->RecordItemTableWidget->selectRow(0);
            }
        }

        return QWidget::eventFilter(obj, e);
    }

    void mvPatientRecordWidget::onRemoteOrLocalClicked(bool checked)
    {
        if (checked)
            ui->mStackedWidget->setCurrentIndex(0);
    }

    void mvPatientRecordWidget::onRemoteOrLocal2Clicked(bool checked)
    {
        if (checked)
            ui->mStackedWidget->setCurrentIndex(1);
    }

    void mvPatientRecordWidget::initData()
    {
        onApplySearch();
    }

    XmlOptionFile mvPatientRecordWidget::getXmlSettings()
    {
        QString databaseDirectory = DataLocations::getRootConfigPath() + "/Conf";
        QString filename = databaseDirectory + "/Procedure.xml";
        return  XmlOptionFile(filename);
    }

    XmlOptionFile mvPatientRecordWidget::getLocal1Settings()
    {
        QString databaseDirectory = DataLocations::getRootConfigPath() + "/Conf";
        QString filename = databaseDirectory + "/OtherSysSettings.xml";
        return  XmlOptionFile(filename);
    }

    void mvPatientRecordWidget::addLocalSettings()
    {
        XmlOptionFile mOptions = this->getLocal1Settings();
        bool isAutoPID = false;
        bool isAutoAccession = false;
        int PIDFrom = 0;
        int AccesionFrom = 0;

        QDomNodeList presetNodeList = mOptions.getElement().elementsByTagName("Data");
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
            else if (textname == "Accession_Number_Start_Form")
            {
                QString style = node.toElement().attribute("value");
                AccesionFrom = style.toInt();
            }
        }

        if (isAutoPID)
        {
            for (int i = 0; i < presetNodeList.count(); ++i)
            {
                QDomNode node = presetNodeList.at(i);
                QString textname = node.toElement().attribute("key");
                if (textname == "Patient_ID_Start_Form")
                {
                    node.toElement().setAttribute("value", QString::number(PIDFrom + 1));
                    break;
                }
            }
        }

        if (isAutoAccession)
        {
            for (int i = 0; i < presetNodeList.count(); ++i)
            {
                QDomNode node = presetNodeList.at(i);
                QString textname = node.toElement().attribute("key");
                if (textname == "Accession_Number_Start_Form")
                {
                    node.toElement().setAttribute("value", QString::number(AccesionFrom + 1));
                    break;
                }
            }
        }

        mOptions.save();
    }


    void mvPatientRecordWidget::createPatientStudy(const StudyRecord &study)
    {
        QString PID = study.patientId;
        QString studyID = study.studyId;
        QString accessionNumber = study.accNumber;
        QString name = study.patientName;
        QString sex = study.patientSex;
        QString birthDay = study.patientBirth;
        QString age = study.age;
        QString studyDate = study.studyDate;
        QString studyTime = study.studyTime;
        QString referringPhysician = study.reqPhysician;
        QString performingPhysician = study.perPhysician;
        QString ExposeStatus = study.ExposeStatus;

        QString modality = study.modality;
        QString procID = study.procId;
        QString protocalCode = study.protocalCode;
        QString protocalMeaning = study.protocalMeaning;

        QString studyDescription = study.studyDesc;

        int     iPatient = study.ID.toInt();

        QString currentDate = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        QString studyUID = "1.2.156.112728.101.101.024192038058038104." + currentDate;

        DicomDatabase::instance()->insertStudy(iPatient, studyUID, studyID, age, studyDate, studyTime, accessionNumber, modality,
            referringPhysician, performingPhysician, studyDescription, procID, protocalCode, protocalMeaning);

        if (iPatient != -1)
        {
            this->mInsertPatientID = QString::number(iPatient, 10);
        }

        qInfo() << "PatientID : " + PID + " StudyID : " + studyID;

        emit finishedAddNewPatient(true);



        QString currentDate1 = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        QString SeriesInstanceUID = "1.2.156.112728.101.102.024192038058038104." + currentDate1;
        QString ExposeStatus2 = "Scheduled";
        QString BodyPartExamined1 = "BREAST";
        QString seriesNumber = QString::number(1);
        DicomDatabase::instance()->insertSeries(studyUID, SeriesInstanceUID, modality, seriesNumber, BodyPartExamined1, ExposeStatus2);

        QString ViewName = "";
        QString ViewDescription = "";
        QString ViewPosition = "";
        QString PatientOrientation = "";
        QString ImageLaterality = "";
        QString ViewIconFile = DataLocations::getRootConfigPath() + "/image/Layer.png";
        QString SOPInstanceUID = "1.2.156.112728.101.103.024192038058038104." + currentDate1 + ".0";
        DicomDatabase::instance()->insertImage(SeriesInstanceUID, SOPInstanceUID, PatientOrientation, ImageLaterality, ViewIconFile, ViewName, ViewDescription, ViewPosition, 0);








        QString sdir = "";
        SessionHelper::getInstance()->selectCurrentPatient(name, PID, studyUID, birthDay, sdir);
        SessionHelper::getInstance()->selectCurrentPatientUID(this->mInsertPatientID);

        WorklistItem *item = new WorklistItem;
        item->patientId = PID;
        item->patientName = name;
        item->patientSex = sex;
        item->patientBirth = birthDay;
        item->patientAge = age;
        item->accNumber = accessionNumber;
        item->studyID = studyID;
        item->reqPhysician = referringPhysician;
        item->schPhysician = performingPhysician;
        item->studyDate = studyDate;
        item->studyTime = studyTime;
        item->Modality = modality;
        item->ProcID = procID;
        item->ProcCode = protocalCode;
        item->ProcDesc = protocalMeaning;

        item->studyDesc = studyDescription;
        item->studyUID = studyUID;
        item->ID = study.ID;

        this->wlistModel->insertItem(item, 0);
        ui->RecordItemTableWidget->selectRow(0);

        addLocalSettings();
    }

    void mvPatientRecordWidget::updatePatientStudy(const StudyRecord &study)
    {
        QString PID = study.patientId;
        QString accessionNumber = study.accNumber;
        QString name = study.patientName;
        QString sex = study.patientSex;
        QString birthDay = study.patientBirth;
        QString age = study.age;
        QString studyID = study.studyId;
        QString studyDate = study.studyDate;
        QString studyTime = study.studyTime;
        QString referringPhysician = study.reqPhysician;
        QString performingPhysician = study.perPhysician;
        QString studyDescription = study.studyDesc;
        int patientUID = study.ID.toInt();

        QString modality = study.modality;
        QString procID = study.procId;
        QString protocalCode = study.protocalCode;
        QString protocalMeaning = study.protocalMeaning;

        QString studyUID = study.studyUID;


        DicomDatabase::instance()->updateStudy(patientUID, studyID, age, studyDate, studyTime, accessionNumber, modality,
            referringPhysician, performingPhysician, studyDescription,procID, protocalCode, protocalMeaning, studyUID);

        QStringList SOPUID = DicomDatabase::instance()->getSopUIDByStudyUID(studyUID);
        for (int j = SOPUID.size() - 1; j >= 0; j--)
        {
            QString Path = DicomDatabase::instance()->getImageFileFullNameBySOPUID(SOPUID[j]);
            //updateDicomTagInfo(study, Path);
        }

        emit finishedAddNewPatient(true);

        QString sdir = "";
        SessionHelper::getInstance()->selectCurrentPatient(name, PID, studyUID, birthDay, sdir);
        QString mPatientUID = QString::number(patientUID);
        SessionHelper::getInstance()->selectCurrentPatientUID(mPatientUID);

        WorklistItem *item = new WorklistItem;
        item->patientId = PID;
        item->patientName = name;
        item->patientSex = sex;
        item->patientBirth = birthDay;
        item->patientAge = age;
        item->accNumber = accessionNumber;
        item->studyID = studyID;
        item->reqPhysician = referringPhysician;
        item->schPhysician = performingPhysician;
        item->studyDate = studyDate;
        item->studyTime = studyTime;
        item->Modality = modality;
        item->ProcID = procID;
        item->ProcCode = protocalCode;
        item->ProcDesc = protocalMeaning;

        item->studyDesc = studyDescription;
        item->studyUID = studyUID;
        item->ID = study.ID;
        item->ID = study.ID;

        int curRow = ui->RecordItemTableWidget->currentIndex().row();
        this->wlistModel->removeColumn(curRow);
        this->wlistModel->insertItem(item, curRow);
        int row = ui->RecordItemTableWidget->currentIndex().row();
        this->wlistModel->removeItem(row);
    }

    void mvPatientRecordWidget::onSwitchQueryPage()
    {
        ui->mStackedWidgetControl->setFixedWidth(380);
        ui->mStackedWidgetControl->setCurrentIndex(1);
    }

    void mvPatientRecordWidget::onCloseQueryPage()
    {
        ui->mStackedWidgetControl->setFixedWidth(130);
        ui->mStackedWidgetControl->setCurrentIndex(0);
    }

    void mvPatientRecordWidget::onResetTableWidget()
    {
        ui->mStackedWidgetControl->setFixedWidth(130);
        ui->mStackedWidgetControl->setCurrentIndex(0);
        if (ui->RecordItemTableWidget->model()->rowCount() > 0 && mCurrentSelectRow != -1)
        {
            ui->RecordItemTableWidget->selectRow(mCurrentSelectRow);
        }
        else
        {
            ui->RecordItemTableWidget->selectRow(0);
        }
    }

    void mvPatientRecordWidget::OnPidClicked()
    {
        QString pid = ui->PIDLineEdit_2->text();
        QStringList list;
        if (pid.contains(";"))
        {
            list = pid.split(";");
        }
        else
        {
            ui->PIDLineEdit_2->setText(pid);
        }

        if (list.count() > 1)
        {
            ui->PIDLineEdit_2->setText(list.at(1));
        }
        else
        {
            qDebug() << "PID : QR code parsing error";
        }
    }

    void mvPatientRecordWidget::OnAccessionClicked()
    {
        QString accession = ui->AccessionLineEdit_3->text();
        QStringList list;

        if (accession.contains(";"))
        {
            list = accession.split(";");
        }
        else
        {
            ui->PIDLineEdit_2->setText(accession);
        }

        if (list.count() > 1)
        {
            ui->PIDLineEdit_2->setText(list.at(1));
        }
        else
        {

        }
    }

    void mvPatientRecordWidget::onCurrentItemChanged(QModelIndex index)
    {
        QModelIndex idx = wlistProxyModel->mapToSource(index);
        if (idx.isValid())
        {
            WorklistItem *item = static_cast<WorklistItem*>(idx.internalPointer());
            StudyRecord study;

            study.patientName = item->patientName;
            study.patientId = item->patientId;
            study.patientSex = item->patientSex;
            study.patientBirth = item->patientBirth;
            study.age = item->patientAge;
            study.accNumber = item->accNumber;
            study.studyDate = item->studyDate;
            study.reqPhysician = item->reqPhysician;
            study.perPhysician = item->schPhysician;

            study.modality = item->Modality;
            study.procId = item->ProcID;
            study.protocalCode = item->ProcCode;
            study.protocalMeaning = item->ProcDesc;

            study.studyDesc = item->studyDesc;
            study.studyUID = item->studyUID;
            study.studyId = item->studyID;
            study.studyTime = item->studyTime;

            emitCurrentPatInfo(study);

            QString sdir = "";
            SessionHelper::getInstance()->selectCurrentPatient(item->patientName, item->patientId, item->studyUID, item->patientBirth, sdir);

            QString mPatientUID = DicomDatabase::instance()->getPatientByStudyUID(item->studyUID);
            SessionHelper::getInstance()->selectCurrentPatientUID(mPatientUID);
            SessionHelper::getInstance()->setCurrentStudy(study);
        }
        else
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("Please select a row and try again!"), QMessageBox::Ok);
            return;
        }
    }

    bool mvPatientRecordWidget::setNewPatientInfo(StudyRecord &study, bool added)
    {
        if (added)
        {
            QString PID = study.patientId;
            QString studyID = study.studyId;
            if (DicomDatabase::instance()->isExistStudyId(studyID, true))
            {
                XDMessageBox::showInformation(NULL, tr("XPhaseContrast"), tr("StudtID already exists. Failed to add it."), QMessageBox::Ok);
                return false;
            }
            QString accessionNumber = study.accNumber;
            QString name = study.patientName;
            QString sex = study.patientSex;
            QString birthDay = study.patientBirth;
            QString age = study.age;

            QString recordDate = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            int dbPatient = DicomDatabase::instance()->insertPatient(name, PID, sex, birthDay, age, recordDate);
            study.ID = QString().number(dbPatient);

            createPatientStudy(study);
        }
        else
        {
            QString PID = study.patientId;
            QString name = study.patientName;
            QString sex = study.patientSex;
            QString birthDay = study.patientBirth;
            QString age = study.age;

            QString studyUID = study.studyUID;

            QString patientUID = DicomDatabase::instance()->getPatientByStudyUID(studyUID);
            DicomDatabase::instance()->updatePatient(name, PID, sex, birthDay, age, patientUID.toInt());

            updatePatientStudy(study);
        }

        emit finishedAddNewPatient(true);
        return true;
    }

    void mvPatientRecordWidget::setNewPatientStartExam(StudyRecord &study)
    {

        QString PID = study.patientId;

        QString studyID = study.studyId;
        if (DicomDatabase::instance()->isExistStudyId(studyID, true))
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("StudtID already exists. Failed to add it"), QMessageBox::Ok);
            return;
        }

        QString accessionNumber = study.accNumber;
        QString name = study.patientName;
        QString sex = study.patientSex;
        QString birthDay = study.patientBirth;
        QString age = study.age;

        QString recordDate = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        int dbPatient = DicomDatabase::instance()->insertPatient(name, PID, sex, birthDay, age, recordDate);
        study.ID = QString().number(dbPatient);

        createPatientStudy(study);

        mIsExposeOrPreviewFlag = 0;
        onWlistBeginStudy();

    }

    void mvPatientRecordWidget::onPreviewBeginStudy()
    {
        mIsExposeOrPreviewFlag = 1;
        onWlistBeginStudy();
    }

    void mvPatientRecordWidget::onWlistBeginStudy()
    {
        QModelIndex index = ui->RecordItemTableWidget->currentIndex();
        if (index.isValid())
        {
            onWlistDoubleClicked(index);
        }
        else
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("Select an item in the table first."), QMessageBox::Ok);
            mIsExposeOrPreviewFlag = 0;
        }
    }

    void mvPatientRecordWidget::onWlistDoubleClicked(const QModelIndex &index)
    {

        QDir dir(DataLocations::getDicomPath());
        if (!dir.exists())
        {
            XDMessageBox::showError(NULL, tr(""), tr("Dicom storage path does not exist, please use System Settings ->Local database configuration->DICOM file cache address change path !"), QMessageBox::Ok);
            return;
        }

        if (index.isValid())
        {

            QModelIndex idx = wlistProxyModel->mapToSource(index);
            if (idx.isValid())
            {
                WorklistItem *item = static_cast<WorklistItem*>(idx.internalPointer());
                StudyRecord study;

                study.patientName = item->patientName;
                study.patientId = item->patientId;
                study.patientSex = item->patientSex;
                study.patientBirth = item->patientBirth;
                study.age = item->patientAge;
                study.accNumber = item->accNumber;
                study.studyDate = item->studyDate;
                study.reqPhysician = item->reqPhysician;
                study.perPhysician = item->schPhysician;

                study.modality = item->Modality;
                study.procId = item->ProcID;
                study.protocalCode = item->ProcCode;
                study.protocalMeaning = item->ProcDesc;

                study.studyDesc = item->studyDesc;
                study.studyUID = item->studyUID;
                study.studyId = item->studyID;
                study.studyTime = item->studyTime;

                study.ID = item->ID;

                emit emitCurrentPatInfo(study);

                QString sdir = "";
                SessionHelper::getInstance()->selectCurrentPatient(study.patientName, study.patientId, study.studyUID, study.patientBirth, sdir);
                QString mPatientUID = DicomDatabase::instance()->getPatientByStudyUID(study.studyUID);
                SessionHelper::getInstance()->selectCurrentPatientUID(mPatientUID);
                SessionHelper::getInstance()->setCurrentStudy(study);
                SessionHelper::getInstance()->setCurrentSeriesUID("");
            }

            emit sigPatientSwitchPage(mIsExposeOrPreviewFlag);

            mIsExposeOrPreviewFlag = 0;
            mCurrentSelectRow = ui->RecordItemTableWidget->currentIndex().row();
        }
        else
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("Select an item in the table first."), QMessageBox::Ok);
        }
    }

    void mvPatientRecordWidget::enableDateWidget_All(int state)
    {
        if (state == Qt::Checked) // "选中"
        {
            ui->FromDateEdit_2->setEnabled(true);
            ui->ToDateEdit_2->setEnabled(true);
            ui->FromDateEdit_2->setDate(QDate::currentDate().addMonths(-3));
            ui->ToDateEdit_2->setDate(QDate::currentDate());
        }
        else // 未选中 - Qt::Unchecked
        {
            ui->FromDateEdit_2->setEnabled(false);
            ui->ToDateEdit_2->setEnabled(false);
        }
    }

    void mvPatientRecordWidget::enableDateWidget_Three(int state)
    {
        if (state == Qt::Checked) // "选中"
        {
            ui->FromDateEdit_2->setDate(QDate::currentDate().addDays(-7));
            ui->ToDateEdit_2->setDate(QDate::currentDate());
        }
        else // 未选中 - Qt::Unchecked
        {
        }
    }

    void mvPatientRecordWidget::enableDateWidget_Today(int state)
    {
        if (state == Qt::Checked) // "选中"
        {
            ui->FromDateEdit_2->setDate(QDate::currentDate());
            ui->ToDateEdit_2->setDate(QDate::currentDate());
        }
        else // 未选中 - Qt::Unchecked
        {
        }
    }






    void mvPatientRecordWidget::onApplySearch()
    {
        // 1. 验证日期
        QDate dataFrom = ui->FromDateEdit_2->date();
        QDate dataTo = ui->ToDateEdit_2->date();
        if (dataFrom > dataTo)
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("The appointment start date cannot be greater than the end date."), QMessageBox::Ok);
            return;
        }

        // 2. 清理数据
        this->wlistModel->clearAllItems();

        // 3. 构建SQL查询
        QSqlQuery query(DicomDatabase::instance()->database());
        QStringList sqlParts;
        QList<QVariant> bindValues;

        // 基础查询
        sqlParts << QString(
            "SELECT Patients.PatientID, Patients.PatientName, Patients.PatientSex, "
            "Patients.PatientBirthDate, Patients.PatientAge, Studies.AccessionNumber, "
            "Studies.StudyID,Studies.Priority,Studies.PerformedProtocolCodeValue,Studies.PerformedProtocolCodeMeaning,  "
            "Studies.ReferringPhysician, Studies.PerformingPhysiciansName, "
            "Studies.ScheduledStartDate, Studies.ScheduledStartTime, "
            "Studies.ModalitiesInStudy, Studies.StudyDescription, "
            "Studies.StudyInstanceUID, "
            "Studies.PatientUID "
            "FROM Patients, Studies WHERE Patients.UID = Studies.PatientUID"
        );

        // 添加筛选条件
        if (!ui->PIDLineEdit_2->text().trimmed().isEmpty()) {
            sqlParts << "AND Patients.PatientID LIKE ?";
            bindValues << QString("%%1%").arg(ui->PIDLineEdit_2->text().trimmed());
        }

        if (!ui->AccessionLineEdit_3->text().trimmed().isEmpty()) {
            sqlParts << "AND Studies.AccessionNumber LIKE ?";
            bindValues << QString("%%1%").arg(ui->AccessionLineEdit_3->text().trimmed());
        }

        if (!ui->PersonNameLineEdit_2->text().trimmed().isEmpty()) {
            sqlParts << "AND Patients.PatientName LIKE ?";
            bindValues << QString("%%1%").arg(ui->PersonNameLineEdit_2->text().trimmed());
        }

        // 日期范围
        sqlParts << "AND Studies.ScheduledStartDate >= ? AND Studies.ScheduledStartDate <= ?";
        bindValues << dataFrom.toString("yyyy-MM-dd");
        bindValues << dataTo.toString("yyyy-MM-dd");

        // 排除已删除
        sqlParts << "AND Studies.IsDeleted = 'N'";

        // 检查状态
        int stateIndex = ui->CJ_State_2->currentIndex();
        if (stateIndex != 0) {
            QString statusValue;
            switch (stateIndex) {
                case 1: statusValue = "Completed"; break;
                case 2: statusValue = "Paused"; break;
                case 3: statusValue = "Scheduled"; break;
            }
            if (!statusValue.isEmpty()) {
                sqlParts << "AND Studies.ExposeStatus = ?";
                bindValues << statusValue;
            }
        }

        // 排序
        sqlParts << "ORDER BY Studies.ScheduledStartDate DESC, Studies.ScheduledStartTime DESC";

        // 4. 执行查询
        QString finalSql = sqlParts.join(" ");
        query.prepare(finalSql);

        for (int i = 0; i < bindValues.size(); ++i)
        {
            query.bindValue(i, bindValues[i]);
        }

        if (!query.exec())
        {
            qDebug() << "SQL :" << finalSql;
            return;
        }

        // 5. 处理结果
        while (query.next())
        {
            WorklistItem *item = new WorklistItem;

            // 使用列索引常量或枚举提高可读性
            const int col_PatientID = 0;
            const int col_PatientName = 1;
            const int col_PatientSex = 2;
            const int col_PatientBirth = 3;
            const int col_PatientAge = 4;
            const int col_AccessionNumber = 5;
            const int col_StudyID = 6;
            const int col_ProtocolID = 7;
            const int col_ProtocolCode = 8;
            const int col_ProtocolMeaning = 9;
            const int col_ReferringPhysician = 10;
            const int col_PerformingPhysician = 11;
            const int col_ScheduledDate = 12;
            const int col_ScheduledTime = 13;
            const int col_Modalities = 14;
            const int col_StudyDescription = 15;
            const int col_StudyInstanceUID = 16;
            const int col_PatientUID = 17;

            // 赋值基本字段
            item->patientId = query.value(col_PatientID).toString().trimmed();
            item->patientName = query.value(col_PatientName).toString().trimmed();
            item->patientSex = query.value(col_PatientSex).toString().trimmed();
            item->patientBirth = query.value(col_PatientBirth).toString().trimmed();
            item->patientAge = query.value(col_PatientAge).toString().trimmed();
            item->accNumber = query.value(col_AccessionNumber).toString().trimmed();
            item->studyID = query.value(col_StudyID).toString().trimmed();

            item->ProcID = query.value(col_ProtocolID).toString().trimmed();
            item->ProcCode = query.value(col_ProtocolCode).toString().trimmed();
            item->ProcDesc = query.value(col_ProtocolMeaning).toString().trimmed();
            item->Modality = query.value(col_Modalities).toString().trimmed();

            item->reqPhysician = query.value(col_ReferringPhysician).toString().trimmed();
            item->schPhysician = query.value(col_PerformingPhysician).toString().trimmed();
            item->studyDate = query.value(col_ScheduledDate).toString().trimmed();
            item->studyTime = query.value(col_ScheduledTime).toString().trimmed();
            item->studyDesc = query.value(col_StudyDescription).toString().trimmed();

            // Study UID
            if (!query.value(col_StudyInstanceUID).isNull()) {
                item->studyUID = query.value(col_StudyInstanceUID).toString().trimmed();
            }

            item->ID = query.value(col_PatientUID).toString().trimmed();

            this->wlistModel->insertItem(item);
        }

        onResetTableWidget();
    }


    void mvPatientRecordWidget::onGetScuSearch(bool checked)
    {
        if(checked)
        {

        }

        onApplySearch();

    }

    void mvPatientRecordWidget::deletePatientClicked()
    {
        QModelIndexList indexList1 = ui->RecordItemTableWidget->selectionModel()->selectedIndexes();
        if (indexList1.count() <= 0)
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("Please select a row and try again!"), QMessageBox::Ok);
            return;
        }

        int result = XDMessageBox::showQuestion(nullptr, "", tr("Delete the project informs?"), QMessageBox::Yes | QMessageBox::No);
        if (result != QMessageBox::Yes)
            return;

        if (!indexList1.isEmpty())
        {
            QModelIndexList indexList = deleteRepeatList(indexList1);
            QModelIndex index = indexList.first();

            QModelIndex idx = wlistProxyModel->mapToSource(index);
            if (idx.isValid())
            {
                WorklistItem *item = static_cast<WorklistItem*>(idx.internalPointer());
                QString studyUID = item->studyUID;
                QString str = DicomDatabase::instance()->getStudyLockedFromStudyUID(studyUID);
                if (str.isEmpty())
                {
                    this->wlistModel->removeItem(index.row());
                    return;
                }
                if (str == "N")
                {
                    bool success =  DicomDatabase::instance()->updateIsDeleteFromStudyUID(studyUID, "Y");
                    if (success)
                    {
                        this->wlistModel->removeItem(index.row());
                    }
                    else
                    {
                        XDMessageBox::showInformation(NULL, tr(""), tr("Delete check faile!"), QMessageBox::Ok);
                        return;
                    }
                }
                else
                {
                    XDMessageBox::showInformation(NULL, tr(""), tr("The patient has checked the lock. Please try again after unlocking!"), QMessageBox::Ok);
                    return;
                }
            }

            indexList1 = ui->RecordItemTableWidget->selectionModel()->selectedIndexes();
        }
        onApplySearch();

    }

    QModelIndexList mvPatientRecordWidget::deleteRepeatList(QModelIndexList indexList)
    {
        QModelIndex index, newIndex;
        QModelIndexList newIndexList;
        foreach(index, indexList)
        {
            if (newIndex.row() != index.row())
            {
                newIndex = index;
                newIndexList.append(newIndex);
            }
        }
        return newIndexList;
    }

    void mvPatientRecordWidget::openUpdatePatientClicked()
    {
        QModelIndex index = ui->RecordItemTableWidget->currentIndex();
        if (!index.isValid())
        {
            XDMessageBox::showInformation(NULL, tr(""), tr("Please select a row and try again!"), QMessageBox::Ok);
            return;
        }

        QModelIndex idx = wlistProxyModel->mapToSource(index);
        if (idx.isValid())
        {
            WorklistItem *item = static_cast<WorklistItem*>(idx.internalPointer());
            StudyRecord study;

            QString patientName = item->patientName;
            QString patientID = item->patientId;
            QString patientBirth = item->patientBirth;
            QString patientAge = item->patientAge;
            QString patientSex = item->patientSex;
            QString studyDate = item->studyDate;
            QString reqPhysician = item->reqPhysician;
            QString schPhysician = item->schPhysician;
            QString accNumber = item->accNumber;

            QString modality = item->Modality;
            QString procId = item->ProcID;
            QString protocalCode = item->ProcCode;
            QString protocalMeaning = item->ProcDesc;

            QString studyID = item->studyID;
            QString studyTime = item->studyTime;
            QString studyDesc = item->studyDesc;
            QString studyUID = item->studyUID;


            emit sigEditPatientInfo(patientID, patientName, patientSex, patientBirth, patientAge, accNumber, studyID, procId,protocalCode,protocalMeaning, reqPhysician, schPhysician, studyDate, studyTime, modality, studyDesc, studyUID);
        }
    }

}
