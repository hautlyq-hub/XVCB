#include "mvworklistitemmodel.h"

QModelIndex WorklistItemModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent) || parent.isValid()) return QModelIndex();
	else return createIndex(row, column, itemList.at(row));
}

QVariant WorklistItemModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid() && Qt::DisplayRole == role)
	{
		WorklistItem *item = static_cast<WorklistItem*>(index.internalPointer());
		switch (index.column()) {
		case PatientId:
			return item->patientId;;
		case PatientName:
			return item->patientName;
		case PatientSex:
			return item->patientSex;
		case PatientBirth:
			return item->patientBirth;
		case PatientAge:
			return item->patientAge;
        case StudyDate:
            return item->studyDate;
        case ReqPhysician:
            return item->reqPhysician;
        case SchPhysician:
            return item->schPhysician;
		case AccNumber:
			return item->accNumber;

        case Modality:
            return item->Modality;
        case ProcID:
            return item->ProcID;
        case ProcCode:
            return item->ProcCode;
        case ProcDesc:
            return item->ProcDesc;

        case StudyID:
			return item->studyID;
        case StudyTime:
            return item->studyTime;
        case StudyDesc:
            return item->studyDesc;
        case StudyUID:
            return item->studyUID;

		default:
			return QVariant();
		}
	}
	else if (role == Qt::TextAlignmentRole)
	{
		return  int(Qt::AlignCenter | Qt::AlignVCenter);
	}
	else return QVariant();
}

QVariant WorklistItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (Qt::DisplayRole == role) {
		if (Qt::Horizontal == orientation) {
			switch (section) {
			case PatientId:
                return tr("ID");
			case PatientName:
                return tr("Name");
            case AccNumber:
                return tr("SerialNo");
            case StudyDate:
                return tr("Date");

			case PatientBirth:
                return tr("Inner\nStandard");
            case ProcID:
                return tr("Inner\nTolerance");

			case PatientAge:
                return tr("HotOuter\nStandard");
            case ProcCode:
                return tr("HotOuter\nTolerance");

            case ReqPhysician:
                return tr("Wall\nStandard");
            case ProcDesc:
                return tr("Wall\nTolerance");

            case SchPhysician:
                return tr("Eccentic\nStandard");
            case Modality:
                return tr("Eccentric\nTolerance");


            case StudyID:
                return tr("Item ID");
            case StudyTime:
                return tr("Time");
            case StudyDesc:
                return tr("Description");
            case StudyUID:
                return tr("UID");
            case PatientSex:
                return tr("Mode");

			default:
				return QVariant();

			}
		}
		else return section + 1;
	}
	else return QVariant();
}

bool WorklistItemModel::insertRows(int row, int count, const QModelIndex &parent)
{
	if (parent.isValid()) return false;
	beginInsertRows(parent, row, row + count - 1);
	for (int i = 0; i < count; ++i) {
		WorklistItem *item = new WorklistItem;
		itemList.insert(row + i, item);
	}
	endInsertRows();
	return true;
}

bool WorklistItemModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if (parent.isValid()) return false;
	beginRemoveRows(parent, row, row + count - 1);
	for (int i = 0; i < count; ++i) {
		delete itemList.takeAt(row);
	}
	endRemoveRows();
	return true;
}

void WorklistItemModel::insertItem(WorklistItem *item, int row)
{
	if (row < 0 || row > itemList.size()) row = itemList.size();
	beginInsertRows(QModelIndex(), row, row);
	itemList.insert(row, item);
	endInsertRows();
}

bool WorklistItemModel::removeItem(int row)
{
	if (row < 0 || row >= itemList.size()) return false;
	beginRemoveRows(QModelIndex(), row, row);
	delete itemList.takeAt(row);
	endRemoveRows();
	return true;
}

void WorklistItemModel::clearAllItems()
{
	if (itemList.size()) {
		beginRemoveRows(QModelIndex(), 0, itemList.size() - 1);
		qDeleteAll(itemList);
		itemList.clear();
		endRemoveRows();
	}
}


