#include "DataBaseWorker.h"

DataBaseWorker::DataBaseWorker(){}

//Table names
static const char *dialogsTableName = "dialogs";
static const char *conversationTableName = "conversation";
static const char *onlineUsersTableName = "onlineUsers";
static const char *usersTableName = "users";
static const char *dialogMembersTableName = "dialogMembers";


//Connection to dataBase
bool DataBaseWorker::createDataBaseConnection()
{
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("postgres");
    db.setUserName("postgres");
    db.setPassword("gjcnuht");
    if (db.open()) {
        qDebug("Connected to database");
        return true;
    }
    qDebug("Error occured in connection to database");
    return false;
}



void DataBaseWorker::createTableUsers() {
    if (QSqlDatabase::database().tables().contains(usersTableName)) {
        // The table already exists; we don't need to do anything.
    }

    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id BIGSERIAL PRIMARY KEY,"
                    "login varchar(32),"
                    "passhash varchar(64)"
                    ")")) {
        qFatal("Failed to query database: %s",
               qPrintable(query.lastError().text()));
    }
    return;
}



void DataBaseWorker::createTableConversations()
{
    if (QSqlDatabase::database().tables().contains(conversationTableName)) {
        // The table already exists; we don't need to do anything.
    }
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS conversation ("
                    "id BIGSERIAL PRIMARY KEY,"
                    "dialog_id BIGSERIAL references dialogs(dialog_id) ON DELETE CASCADE,"
                    "user_id BIGSERIAL references users(id) ON DELETE CASCADE,"
                    "message varchar(2200),"
                    "date_create varchar(40)"
                    ")")) {
        qFatal("Failed to query createTableConversations: %s",
               qPrintable(query.lastError().text()));
    }
    return;
}



void DataBaseWorker::createTableDialogs()
{
    if (QSqlDatabase::database().tables().contains(dialogsTableName)) {
        // The table already exists; we don't need to do anything.
    }
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS dialogs ("
                    "dialog_id BIGSERIAL PRIMARY KEY,"
                    "user_id BIGSERIAL references users(id) ON DELETE CASCADE"//dialog creator
                    ")")) {
        qFatal("Failed to query createTableDialogs: %s",
               qPrintable(query.lastError().text()));
    }
    return;
}



void DataBaseWorker::createTableDialogMembers()
{
    if (QSqlDatabase::database().tables().contains(dialogMembersTableName)) {
        // The table already exists; we don't need to do anything.
    }
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS dialogMembers ("
                    "dialog_id BIGSERIAL references dialogs(dialog_id) ON DELETE CASCADE,"
                    "user_id BIGSERIAL references users(id) ON DELETE CASCADE"
                    ")")) {
        qFatal("Failed to query createTableDialogMembers: %s",
               qPrintable(query.lastError().text()));
    }
    return;
}



void DataBaseWorker::createTableOnlineUsers()
{
    if (QSqlDatabase::database().tables().contains(onlineUsersTableName)) {
        // The table already exists; we don't need to do anything.
    }
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS onlineUsers ("
                    "user_id BIGSERIAL references users(id) ON DELETE CASCADE"
                    ")")) {
        qFatal("Failed to query database: %s",
               qPrintable(query.lastError().text()));
    }
    return;
}



//Autentification
bool DataBaseWorker::checkUser(const QString &login,
                               const QString &password) {

    QSqlQuery query;
    query.exec("SELECT id, login, passhash FROM users WHERE login = '" +
               login + "'");
    if (query.first()) {
        QString dbPassHash = query.value(2).toString();
        QString passTextFieldHash =
                QString(QCryptographicHash::hash(password.toLocal8Bit(),
                                                 QCryptographicHash::Md5).toHex());
        if (passTextFieldHash == dbPassHash)
        {
            setCurrentUserId(query.value(0).toString());
            setCurrentUserName(query.value(1).toString());
            return true;
        }
        else
        {
            setCurrentUserId("");
            setCurrentUserName("");
            return false;
        }
    }
    return false;
}



void DataBaseWorker::addNewDialog(const QString &authorId)
{
    qDebug()<<"DataBaseWorker::addNewDialog authorId "<<authorId;
    if (db.isOpen())
    {
        QSqlQuery query;

        query.prepare("INSERT INTO dialogs(user_id)"
                      "VALUES (?)");
        query.addBindValue(authorId);

        if(!query.exec())
            std::cerr << "addNewDialog error" << query.lastError().databaseText().toStdString();
    }
    else
    {
        std::cerr << "DataBaseWorker::addNewDialog db not open" << std::endl;
    }

}



void DataBaseWorker::addDialogMembers(const QString &authorId,
                                      const QString &recipientId,
                                      const QString &currentDialogId)
{
    std::cerr<<"DataBaseWorker::addDialogMembers authorId"<<authorId.toStdString()<<std::endl;
    std::cerr<<"DataBaseWorker::addDialogMembers recipientId"<<recipientId.toStdString()<<std::endl;
    if (db.isOpen())
    {
        QSqlQuery query;

        query.prepare("INSERT INTO dialogMembers(dialog_id, user_id) "
                      "VALUES (?,?) ");
        query.addBindValue(currentDialogId);
        query.addBindValue(authorId);
        if(!query.exec())
        {
            std::cerr << "addDialogMembers authorId error" << query.lastError().databaseText().toStdString();
        }


        query.prepare("INSERT INTO dialogMembers(dialog_id, user_id) "
                      "VALUES (?,?) ");
        query.addBindValue(currentDialogId);
        query.addBindValue(recipientId);
        if(!query.exec())
        {
            std::cerr << "addDialogMembers recipientId error" << query.lastError().databaseText().toStdString();
        }
    }
    else
    {
        std::cerr << "DataBaseWorker::addDialogMembers() db not open" << std::endl;
    }
    return;
}



void DataBaseWorker::addMessageToDataBase(const QString &dialogId,
                                          const QString &authorId,
                                          const QString &message,
                                          const QString &timestamp)
{
    if (db.isOpen())
    {
        QSqlQuery query;
        query.prepare("INSERT INTO conversation (dialog_id, user_id, message, date_create) "
                      "VALUES(?,?,?,?)");

        query.addBindValue(dialogId);
        query.addBindValue(authorId);
        query.addBindValue(message);
        query.addBindValue(timestamp);
        query.exec();

        if (query.lastError().isValid()) {
            qDebug() << "Add message error" << query.lastError();
        }
    }
}



bool DataBaseWorker::addNewUser(const QString &login,
                                const QString &password)
{
    QSqlQuery query;
    if(checkUserName(login))
    {
        QString passwordHash =
                QString(QCryptographicHash::hash(password.toLocal8Bit(),
                                                 QCryptographicHash::Md5).toHex());

        query.prepare("INSERT INTO users (login, passhash) "
                      "VALUES (?, ?)");
        query.addBindValue(login);
        query.addBindValue(passwordHash);
        query.exec();

        if (query.lastError().isValid()) {
            std::cerr << "Register error" << query.lastError().text().toStdString();
            return false;
        }
        return true;
    }
    return false;
}



bool DataBaseWorker::addOnlineUser(const QString &userId)
{
    QSqlQuery query;

    query.prepare("INSERT INTO onlineUsers (user_id) "
                  "VALUES (?)");
    query.addBindValue(userId);
    query.exec();

    if (query.lastError().isValid()) {
        qDebug() << "DataBaseWorker::addOnlineUser error" << query.lastError();
        return false;
    }
    return true;
}

void DataBaseWorker::deleteOnlineUser(const QString &userId)
{
    QSqlQuery query;

    query.exec("DELETE FROM onlineUsers WHERE user_id = "+ userId);

    if (query.lastError().isValid())
        qDebug() << "DataBaseWorker::deleteOnlineUser error" << query.lastError();
}



bool DataBaseWorker::checkUserName(const QString &login)
{
    QSqlQuery query;
    query.exec("SELECT login FROM users WHERE login= '" + login + "'");
    if (query.first())
    {
        std::cerr<<"UserName is taken"<<std::endl;
        return false;
    }
    return true;
}



bool DataBaseWorker::checkForDialog(const QString &authorId,
                                    const QString &recipientId)
{
    qDebug()<<"DataBaseWorker::checkforDialog authorId "<<authorId;
    qDebug()<<"DataBaseWorker::checkforDialog recipientId "<<recipientId;
    bool ret = false;
    if (db.isOpen())
    {
        QSqlQuery query_dialogId;
        QSqlQuery query_recipientId;

        if(query_dialogId.exec("SELECT dialog_id FROM dialogMembers "
                               "WHERE user_id = " + authorId))
        {
            while(query_dialogId.next())
            {
                if(query_recipientId.exec("SELECT dialog_id FROM dialogMembers "
                                          "WHERE dialog_id = " + query_dialogId.value(0).toString() +
                                          " AND user_id = "+ recipientId))
                {
                    if(query_recipientId.first())
                    {
                        qDebug() << "DataBaseWorker::checkforDialog query_recipientId " << query_recipientId.value(0).toString();
                        if(query_recipientId.isSelect() && !query_recipientId.value(0).toString().isEmpty())
                        {
                            qDebug() << "DataBaseWorker::checkforDialog query_recipientId ok" << query_recipientId.value(0).toString();
                            ret = true;
                            break;
                        }
                    }
                }
                else
                {
                    qDebug() << "DataBaseWorker::checkforDialog query_recipientId error" << query.lastError();
                    ret = false;
                }
            }
        }
        else
        {
            qDebug() << "DataBaseWorker::checkforDialog query_dialogId error" << query.lastError();
            ret = false;
        }
    }
    qDebug() << "DataBaseWorker::checkforDialog result " << ret;
    return ret;
}



QList<QString> DataBaseWorker::getRecipientIdFromDialog(const QString &dialogId)
{
    QList<QString> recipientIdList;

    if (db.isOpen())
    {
        QSqlQuery query;
        if(query.exec("SELECT user_id FROM dialogMembers "
                      "WHERE dialog_id = "+ dialogId))
        {
            while(query.next())
            {
                recipientIdList.push_back(query.value(0).toString());
            }
        }
        else
        {
            qDebug() << "getRecipientIdFromDialog error" << query.lastError();
        }
    }
    else
    {
        qDebug() << "DataBaseWorker::getRecipientIdFromDialog db not open " << query.lastError();
    }

    return recipientIdList;
}

QString DataBaseWorker::getRecipientIdFromDialog_WithoutAuthorId(const QString &dialogId,
                                                                 const QString &authorId)
{
    QString recipientId;

    if (db.isOpen())
    {
        QSqlQuery query;
        if(query.exec("SELECT user_id FROM dialogMembers "
                      "WHERE dialog_id = "+ dialogId +
                      " AND user_id != "+ authorId))
        {
            while(query.next())
            {
                recipientId = query.value(0).toString();
            }
        }
        else
        {
            qDebug() << "getRecipientIdFromDialog error" << query.lastError();
        }
    }
    else
    {
        qDebug() << "DataBaseWorker::getRecipientIdFromDialog db not open " << query.lastError();
    }

    return recipientId;
}



QString DataBaseWorker::getDialogName(const QString &userId)
{
    QString dialogName;
    if (db.isOpen())
    {
        QSqlQuery query;
        if(query.exec("SELECT 'dialogs'.'dialog_name' FROM dialogs "
                      "INNER JOIN 'dialogMembers' ON 'dialogMembers'.'user_id' = "
                      + userId))
        {
            if(query.first())

                dialogName = query.value(0).toString();
        }
        else
        {
            qDebug() << "DataBaseWorker::getDialogName" << query.lastError();
        }
    }

    return dialogName;
}



QJsonArray DataBaseWorker::getDialogList(const QString &userId)
{
    QJsonObject dialog;
    QJsonArray dialogList;

    if (db.isOpen())
    {
        QSqlQuery query_dialogId;
        QSqlQuery query_dialogNames;
        QSqlQuery query_lastMessage;
        QSqlQuery query_userStatus;

        if(query_dialogId.exec("SELECT DISTINCT dialogMembers.dialog_id FROM users "
                               "INNER JOIN dialogMembers ON dialogMembers.user_id = users.id "
                               "WHERE users.id = " + userId))
        {
            while(query_dialogId.next())
            {
                if(query_userStatus.exec("SELECT dialogMembers.user_id AS onlineUser_id FROM dialogMembers "
                                         "INNER JOIN onlineUsers ON dialogMembers.user_id = onlineUsers.user_id "
                                         "WHERE dialogMembers.dialog_id = "+ query_dialogId.value(0).toString()+
                                         " AND onlineUsers.user_id != " + userId))


                    qDebug()<<"query_UsersDialogs.next()"<<query_dialogId.value(0).toString();
                if(query_dialogNames.exec(
                            "SELECT DISTINCT users.login FROM dialogMembers "
                            "INNER JOIN users ON dialogMembers.user_id = users.id "
                            "WHERE dialogMembers.dialog_id = " + query_dialogId.value(0).toString() +
                            " AND users.id != " + userId))
                {
                    while(query_dialogNames.next())
                    {
                        qDebug()<<"query_dialogNames.next()"<<query_dialogNames.value(0).toString();
                        if(query_lastMessage.exec(
                                    "SELECT DISTINCT conversation.message, "
                                    "conversation.date_create FROM conversation "
                                    "WHERE conversation.dialog_id = " + query_dialogId.value(0).toString() +
                                    " ORDER BY conversation.date_create DESC"))
                        {
                            if(query_lastMessage.first())
                            {
                                if(query_userStatus.exec("SELECT dialogMembers.user_id AS onlineUser_id FROM dialogMembers "
                                                         "INNER JOIN onlineUsers ON dialogMembers.user_id = onlineUsers.user_id "
                                                         "WHERE dialogMembers.dialog_id = "+ query_dialogId.value(0).toString()+
                                                         " AND onlineUsers.user_id != " + userId))
                                {
                                    if(query_userStatus.first())
                                    {

                                        qDebug()<<"query_lastMessage.next()"<<query_lastMessage.value(0).toString();
                                        dialog.insert("dialogId", query_dialogId.value(0).toString());
                                        dialog.insert("dialogName",  query_dialogNames.value(0).toString());
                                        dialog.insert("message", query_lastMessage.value(0).toString());
                                        dialog.insert("timestamp",  query_lastMessage.value(1).toString());
                                        dialog.insert("userStatus", "1");
                                    }
                                    else
                                    {
                                        qDebug()<<"query_lastMessage.next()"<<query_lastMessage.value(0).toString();
                                        dialog.insert("dialogId", query_dialogId.value(0).toString());
                                        dialog.insert("dialogName",  query_dialogNames.value(0).toString());
                                        dialog.insert("message", query_lastMessage.value(0).toString());
                                        dialog.insert("timestamp",  query_lastMessage.value(1).toString());
                                        dialog.insert("userStatus", "0");
                                    }
                                }
                                else
                                {
                                    qDebug() << "DataBaseWorker::getDialogNamesList query_userStatus error" << query.lastError();
                                }
                            }
                        }
                        else
                        {
                            qDebug() << "DataBaseWorker::getDialogNamesList query_lastMessage error" << query.lastError();
                        }
                    }
                }
                else
                {
                    qDebug() << "DataBaseWorker::getDialogNamesList query_dialogNames error" << query.lastError();
                }
                dialogList.push_back(dialog);
            }
        }
        else
        {
            qDebug() << "DataBaseWorker::getDialogNamesList query_UsersDialogs error" << query.lastError();
        }
    }

    return dialogList;
}

QString DataBaseWorker::getLastDialogId()
{
    QString ret;
    if (db.isOpen())
    {
        QSqlQuery query;
        if(query.exec("SELECT MAX(dialog_id) FROM dialogs"))
        {
            if(query.first())
                ret = query.value(0).toString();
            else
                qDebug() << "DataBaseWorker::getLastDialogId() query error" << query.lastError();
        }
        else
        {
            qDebug() << "DataBaseWorker::getLastDialogId() db not open" << query.lastError();
        }
    }
    return ret;
}


QString DataBaseWorker::getDialogIdFromDialogMembers(const QString &authorId,
                                                     const QString &recipientId)
{
    qDebug()<<"DataBaseWorker::getDialogIdFromDialogMembers authorId "<<authorId;
    qDebug()<<"DataBaseWorker::getDialogIdFromDialogMembers recipientId "<<recipientId;
    QString  ret;
    if (db.isOpen())
    {
        QSqlQuery query_dialogId_by_author;
        QSqlQuery query_dialogId_by_recipient;

        if(query_dialogId_by_author.exec("SELECT dialog_id FROM dialogMembers "
                               "WHERE user_id = " + authorId))
        {
            while(query_dialogId_by_author.next())
            {
                qDebug() << "DataBaseWorker::getDialogIdFromDialogMembers query_dialogId " << query_dialogId_by_author.value(0).toString();
                if(query_dialogId_by_recipient.exec("SELECT dialog_id FROM dialogMembers "
                                          "WHERE dialog_id = " + query_dialogId_by_author.value(0).toString() +
                                          " AND user_id = "+ recipientId))
                {
                    if(query_dialogId_by_recipient.first())
                    {
                        qDebug() << "DataBaseWorker::getDialogIdFromDialogMembers query_recipientId " << query_dialogId_by_recipient.value(0).toString();
                        if(!query_dialogId_by_recipient.value(0).toString().isEmpty())
                        {
                            qDebug() << "DataBaseWorker::getDialogIdFromDialogMembers query_recipientId ok" << query_dialogId_by_recipient.value(0).toString();
                            ret = query_dialogId_by_recipient.value(0).toString();
                            break;
                        }
                    }
                }
                else
                {
                    qDebug() << "DataBaseWorker::getDialogIdFromDialogMembers query_recipientId error" << query.lastError();
                }
            }
        }
        else
        {
            qDebug() << "DataBaseWorker::getDialogIdFromDialogMembers query_dialogId error" << query.lastError();
        }
    }
    qDebug() << "DataBaseWorker::getDialogIdFromDialogMembers result " << ret;

    return ret;

}

QList<QString> DataBaseWorker::getDialogIdByUserId(const QString &authorId)
{
    QList<QString> ret;
    if (db.isOpen())
    {
        QSqlQuery query_dialogId;

        if(query_dialogId.exec("SELECT dialog_id FROM dialogMembers "
                               "WHERE user_id = " + authorId))
        {
            while(query_dialogId.next())
            {
                ret.append(query_dialogId.value(0).toString());
            }
        }
        else
        {
            qDebug() << "DataBaseWorker::getDialogIdByUserId  query_dialogId error" << query.lastError();
        }
    }
    qDebug() << "DataBaseWorker::getDialogIdByUserId result " << ret;

    return ret;
}



void DataBaseWorker::setCurrentUserId(const QString &userId)
{
    currentUserId = userId;
}



QString DataBaseWorker::getCurrentUserId()
{
    return currentUserId;
}



void DataBaseWorker::setCurrentUserName(const QString &userName)
{
    currentUserName = userName;
}



QString DataBaseWorker::getCurrentUserName()
{
    return currentUserName;
}



QJsonObject DataBaseWorker::getUserList(const QString &authorId)
{
    QJsonObject userNames;
    if (db.isOpen())
    {
        QSqlQuery query;
        if (query.exec("SELECT id, login FROM users WHERE id != " + authorId))
        {
            while (query.next())
            {
                userNames.insert(query.value(0).toString(), query.value(1).toString());
            }
        }
        else
        {
            std::cerr << "DataBaseWorker::getUserNames() exec error" << std::endl;
        }
    }
    else
    {
        std::cerr << "DataBaseWorker::getUserNames() db not open" << std::endl;
    }

    return userNames;
}



QJsonDocument DataBaseWorker::getMessagesHistory(const QString &dialogId)
{
    qDebug()<<"DataBaseWorker::getMessagesHistory dialogId "<<dialogId;
    QJsonDocument messagesHistory;
    QJsonObject mainObject;
    QJsonArray messageArray;

    mainObject.insert("result", "messagesHistory");

    QJsonObject message;

    if (db.isOpen())
    {
        if(!dialogId.isEmpty())
        {
            QSqlQuery query;
            query.exec("SELECT * FROM conversation WHERE (dialog_id = " + dialogId +
                       ") ORDER BY date_create ASC");

            while (query.next())
            {
                message.insert("dialogId", query.value(1).toString());
                message.insert("userId", query.value(2).toString());
                message.insert("message", query.value(3).toString());
                message.insert("timestamp", query.value(4).toString());
                messageArray.push_back(message);
            }

            mainObject.insert("messagesArray", messageArray);
            messagesHistory.setObject(mainObject);

            if (query.lastError().isValid()) {
                qDebug() << "getMessagesHistory error" << query.lastError();
            }
        }
    }
    return messagesHistory;
}

