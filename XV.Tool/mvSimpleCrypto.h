#include <QtCore/QString>
#include <QtCore/QByteArray>

class SimpleCrypto
{
public:
    // 简单 XOR 加密（固定密钥）
    static QString encrypt(const QString &str, const QString &key = "MySecretKey123")
    {
        QByteArray data = str.toUtf8();
        QByteArray keyData = key.toUtf8();
        
        for (int i = 0; i < data.size(); ++i) {
            data[i] = data[i] ^ keyData[i % keyData.size()];
        }
        
        return QString(data.toBase64());
    }
    
    // 解密（与加密相同）
    static QString decrypt(const QString &encryptedStr, const QString &key = "MySecretKey123")
    {
        QByteArray data = QByteArray::fromBase64(encryptedStr.toUtf8());
        QByteArray keyData = key.toUtf8();
        
        for (int i = 0; i < data.size(); ++i) {
            data[i] = data[i] ^ keyData[i % keyData.size()];
        }
        
        return QString::fromUtf8(data);
    }
};