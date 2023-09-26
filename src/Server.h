#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QStringList>
#include <QMap>
#include <QMapIterator>


#include "DataBaseWorker.h"



class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server(QTcpServer *parent = nullptr);
    ~Server();

    bool init();
    void createUserStatus(const QString &status);
    void sendCheckUserResult(const QString &status);
    void sendAddNewUserResult(const QString &status);
    void sendUserNamesListToClient(const QString &authorId);
    void sendDialogList(const QString &userId);
    void addToSocketMap(const QString &userId, QTcpSocket *socket);
    void deleteFromSocketMap(const QString &userId);
    QTcpSocket* getSocket(const QString &userId);
    void sendUserStatus(const QString &userId,
                        const QString &dialogId,
                        const QString &status);
    void sendMessageToClient(const QString &dialogId,
                             const QString &authorId,
                             const QString &recipientId,
                             const QString &message,
                             const QString &timestamp);

    void sendMessagesHistory( const QString &dialogId);



    void slotSendUserStatus(const QJsonDocument &jsonDocument);



public slots:
    void slotConnection();
    void slotReadyRead();
    void slotSocketDeleteLater();

private:
    bool initDataBaseWorker();


    std::shared_ptr<DataBaseWorker> databaseWorker{nullptr};
    QTcpServer *server{nullptr};
    QTcpSocket *socket{nullptr};
    QVector<QTcpSocket*> socketsVector;
    QJsonDocument jsonDocument;
    QJsonParseError jsonError;
    QMap<QString, QTcpSocket*> socketMap;

    QString currentDialogId;
};

#endif // SERVER_H
