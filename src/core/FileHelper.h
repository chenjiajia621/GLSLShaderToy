#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QDebug>
#include <QQmlEngine>

class FileHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit FileHelper(QObject *parent = nullptr) : QObject(parent) {}

    // 读取文件内容
    Q_INVOKABLE QString readFile(const QString &filePath) {
        // 处理 file:/// 前缀
        QString localPath = QUrl(filePath).toLocalFile();
        if (localPath.isEmpty()) localPath = filePath; // 如果不是url格式，尝试直接用

        QFile file(localPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "无法打开文件进行读取:" << localPath;
            return "";
        }
        QTextStream in(&file);
        // 设置编码，防止乱码 (通常 Shader 都是 UTF-8)
        in.setEncoding(QStringConverter::Utf8);
        return in.readAll();
    }

    // 保存文件内容
    Q_INVOKABLE bool saveFile(const QString &filePath, const QString &content) {
        QString localPath = QUrl(filePath).toLocalFile();
        if (localPath.isEmpty()) localPath = filePath;

        QFile file(localPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qDebug() << "无法打开文件进行写入:" << localPath;
            return false;
        }
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << content;
        file.close();
        return true;
    }
};

#endif // FILEHELPER_H
