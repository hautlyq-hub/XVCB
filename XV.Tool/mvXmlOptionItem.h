
#ifndef MVXMLOPTIONITEM_H_
#define MVXMLOPTIONITEM_H_

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>

namespace mv
{

	class XmlOptionItem
	{
public:
	XmlOptionItem()
	{
	}
	XmlOptionItem(const QString& uid, QDomElement root);
	QString readValue(const QString& defval) const;
	void writeValue(const QString& val);

	QVariant readVariant(const QVariant& defval=QVariant()) const;
	void writeVariant(const QVariant& val);

private:
	QDomElement findElemFromUid(const QString& uid, QDomNode root) const;
	QString mUid;
	QDomElement mRoot;

	static QString SerializeDataToB64String(const QVariant& data);
	static QVariant DeserializeB64String(const QString& serializedVariant);
};


	class XmlOptionFile
	{
	public:
        static XmlOptionFile createNull();

        explicit XmlOptionFile(QString filename);
		XmlOptionFile();
		~XmlOptionFile();

		QString getFileName();

        bool isNull() const;

        XmlOptionFile root() const;
		XmlOptionFile descend(QString element) const; ///< step one level down in the xml tree
        XmlOptionFile descend(QString element, QString attributeName, QString attributeValue) const;
        XmlOptionFile ascend() const;
        XmlOptionFile tryDescend(QString element, QString attributeName, QString attributeValue) const;

        QDomDocument getDocument();
        QDomElement getElement();
        QDomElement getElement(QString level1);
        QDomElement getElement(QString level1, QString level2);

        void save();

        void removeChildren();
        void deleteNode();

		QDomElement safeGetElement(QDomElement parent, QString childName);

		//Debugging
        void printDocument();
        void printElement();
		static void printDocument(QDomDocument document);
		static void printElement(QDomElement element);

	private:
		void load();

		QString mFilename;
		QDomDocument mDocument;
        QDomElement mCurrentElement;

	};

	} 

#endif /* CXXMLOPTIONITEM_H_ */
