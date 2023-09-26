#ifndef DATABASEWORKER_H
#define DATABASEWORKER_H

#include <iostream>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVector>
#include <QVariant>
#include <QStringList>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QDateTime>


class DataBaseWorker
{
public:
    DataBaseWorker();

    bool createDataBaseConnection();

    void createTableConversations();
    void createTableDialogs();
    void createTableDialogMembers();
    void createTableOnlineUsers();
    void createTableUsers();

    bool checkUser(const QString &login, const QString &password);
    void addNewDialog(const QString &authorId);

    void addDialogMembers(const QString &authorId,
                          const QString &recipientId,
                          const QString &currentDialogId);

    void addMessageToDataBase(const QString &dialogId,
                              const QString &authorId,
                              const QString &message,
                              const QString &timestamp);

    bool addNewUser(const QString &login, const QString &password);
    bool addOnlineUser(const QString &userId);
    void deleteOnlineUser(const QString &userId);

    bool checkUserName(const QString &login);
    bool checkForDialog(const QString &authorId,
                        const QString &recipientId);

    QList<QString> getRecipientIdFromDialog(const QString &dialogId);

    QString getRecipientIdFromDialog_WithoutAuthorId(const QString &dialogId,
                                                     const QString &authorId);
    QString getDialogName(const QString &userId);
    QJsonArray getDialogList(const QString &userId);
    QString getLastDialogId();
    QString getDialogIdFromDialogMembers(const QString &authorId,
                        const QString &recipientId);
    QList<QString> getDialogIdByUserId(const QString &authorId);

    void setCurrentUserId(const QString &userId);


    QString getCurrentUserId();
    void setCurrentUserName(const QString &userName);
    QString getCurrentUserName();

    QJsonObject getUserList(const QString &authorId);




    QJsonDocument getMessagesHistory(const QString &dialogId);


private:
    QSqlDatabase db;
    QSqlQuery query;
    QString currentDialogId;
    QString currentDialogName;
    QString currentUserId;
    QString currentUserName;

};

#endif // DATABASEWORKER_H
