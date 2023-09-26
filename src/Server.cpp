#include "Server.h"

Server::Server(QTcpServer *parent)
    :QTcpServer(parent){}



Server::~Server()
{
    delete server;
    delete socket;
    std::cerr<<"~Server()"<<std::endl;
}



bool Server::init()
{
    if(server == nullptr)
    {
        server = new QTcpServer(this);
    }

    if (server == nullptr)
        return false;

    if(!server->listen(QHostAddress::Any, 5555))
    {
        std::cerr<<"Server start error"<<std::endl;
        return false;
    }


    connect(server, SIGNAL(newConnection()), this, SLOT(slotConnection()));

    if(!initDataBaseWorker())
        return false;

    if(!databaseWorker->createDataBaseConnection())
        return false;

    databaseWorker->createTableUsers();
    databaseWorker->createTableDialogs();
    databaseWorker->createTableDialogMembers();
    databaseWorker->createTableConversations();
    databaseWorker->createTableOnlineUsers();



    return true;
}

void Server::createUserStatus(const QString &status)
{
    QJsonObject userStatus;
    userStatus.insert("result", "userStatus");
    userStatus.insert("status", status);
    jsonDocument.setObject(userStatus);


    socket->write(jsonDocument.toBinaryData());
    socket->waitForBytesWritten(500);

    qDebug()<<"sendUserNamesListToClient";
}



void Server::slotConnection()
{
    socket = server->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slotSocketDeleteLater()));

    qDebug()<<"New connection: "<<socket;
}



void Server::slotReadyRead()
{

    socket = (QTcpSocket*)sender();
    qDebug()<<"Server::slotReadyRead()(QTcpSocket*)sender()    "<<socket;
    QByteArray dataArray = socket->readAll();
    jsonDocument = QJsonDocument::fromBinaryData(dataArray);

    qDebug()<<"slotread"<<socket->socketDescriptor();

    if(jsonDocument.isObject() && !jsonDocument.isEmpty())
    {

        if(jsonDocument.object().value("query").toString() == "createUserStatus")
        {
            qDebug()<<"createUserStatus"<<jsonDocument;
            slotSendUserStatus(jsonDocument);

        }

        if(jsonDocument.object().value("query").toString() == "dialogList")
        {
            sendDialogList(jsonDocument.object().value("authorId").toString());
            qDebug()<<"sending dialogList"<<dataArray;
        }


        //sending userNamesList
        if (jsonDocument.object().value("query").toString() == "usersList")

        {
            sendUserNamesListToClient(jsonDocument.object().value("authorId").toString());
            qDebug()<<"sending userNamesList"<<dataArray;
        }



        //check users login, password
        if (jsonDocument.object().value("query").toString() == "checkUser")
        {
            if(databaseWorker->checkUser(jsonDocument.object().value("login").toString(),
                                         jsonDocument.object().value("password").toString()))
            {
                databaseWorker->addOnlineUser(databaseWorker->getCurrentUserId());

                addToSocketMap(databaseWorker->getCurrentUserId(), socket);

                sendCheckUserResult("true");
            }
            else
            {
                sendCheckUserResult("false");
            }
        }



        if(jsonDocument.object().value("query").toString() == "sendMessage")
        {
            QString timestamp = QDateTime::
                    currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz");

            if(jsonDocument.object().value("dialogId").toString().isEmpty())
            {
                if(!databaseWorker->checkForDialog(jsonDocument.object().value("authorId").toString(),
                                                   jsonDocument.object().value("recipientId").toString()))
                {
                    databaseWorker->addNewDialog(jsonDocument.object().value("authorId").toString());

                    currentDialogId = databaseWorker->getLastDialogId();

                    std::cerr<<"currentDialogId = "<<currentDialogId.toStdString()<<std::endl;

                    databaseWorker->addDialogMembers(jsonDocument.object().value("authorId").toString(),
                                                     jsonDocument.object().value("recipientId").toString(),
                                                     currentDialogId);

                    databaseWorker->addMessageToDataBase(currentDialogId,
                                                         jsonDocument.object().value("authorId").toString(),
                                                         jsonDocument.object().value("message").toString(),
                                                         timestamp);

                    sendMessageToClient(currentDialogId,
                                        jsonDocument.object().value("authorId").toString(),
                                        jsonDocument.object().value("recipientId").toString(),
                                        jsonDocument.object().value("message").toString(),
                                        timestamp);
                    currentDialogId.clear();
                }
                else
                {
                    currentDialogId = databaseWorker->getDialogIdFromDialogMembers(jsonDocument.object().value("authorId").toString(),
                                                                                   jsonDocument.object().value("recipientId").toString());
                    databaseWorker->addMessageToDataBase(currentDialogId,
                                                         jsonDocument.object().value("authorId").toString(),
                                                         jsonDocument.object().value("message").toString(),
                                                         timestamp);

                    sendMessageToClient(currentDialogId,
                                        jsonDocument.object().value("authorId").toString(),
                                        jsonDocument.object().value("recipientId").toString(),
                                        jsonDocument.object().value("message").toString(),
                                        timestamp);
                    currentDialogId.clear();
                }
            }
            else {

                currentDialogId = jsonDocument.object().value("dialogId").toString();
                std::cerr<<"currentDialogId"<<currentDialogId.toStdString()<<std::endl;

                databaseWorker->addMessageToDataBase(currentDialogId,
                                                     jsonDocument.object().value("authorId").toString(),
                                                     jsonDocument.object().value("message").toString(),
                                                     timestamp);

                sendMessageToClient(currentDialogId,
                                    jsonDocument.object().value("authorId").toString(),
                                    QString(""),
                                    jsonDocument.object().value("message").toString(),
                                    timestamp);
                currentDialogId.clear();
            }
        }



        if(jsonDocument.object().value("query").toString() == "getMessagesHistory")
        {

            if(!jsonDocument.object().value("dialogId").toString().isEmpty())
            {
                sendMessagesHistory(jsonDocument.object().value("dialogId").toString());
            }
            else
            {
                if(databaseWorker->checkForDialog(jsonDocument.object().value("authorId").toString(),
                                                  jsonDocument.object().value("recipientId").toString()))
                {


                    currentDialogId = databaseWorker->getDialogIdFromDialogMembers(
                                jsonDocument.object().value("authorId").toString(),
                                jsonDocument.object().value("recipientId").toString());

                    sendMessagesHistory(currentDialogId);
                    currentDialogId.clear();

                }
            }
        }



        if(jsonDocument.object().value("query").toString() == "addNewUser")
        {
            if(databaseWorker->addNewUser(jsonDocument.object().value("login").toString(),
                                          jsonDocument.object().value("password").toString()))
                sendAddNewUserResult("true");
            else
                sendAddNewUserResult("false");
        }

    }
    else
    {
        std::cerr<<"jsonDocument is empty or not object";
    }
}



void Server::slotSocketDeleteLater()
{
    qDebug()<<"disconnected "<<socket;
}



bool Server::initDataBaseWorker()
{
    if (databaseWorker == nullptr) {
        databaseWorker = std::make_shared<DataBaseWorker>();
        return true;
    }
    return false;
}



void Server::sendUserNamesListToClient(const QString &authorId)
{
    QJsonObject userNamesList;
    userNamesList.insert("result", "userNamesList");
    userNamesList.insert("data", databaseWorker->getUserList(authorId));
    jsonDocument.setObject(userNamesList);

    socket->write(jsonDocument.toBinaryData());
    socket->waitForBytesWritten(500);

    qDebug()<<"sendUserNamesListToClient";
}



void Server::sendDialogList(const QString &userId)
{
    QJsonObject dialogList;
    dialogList.insert("result", "dialogList");
    dialogList.insert("dialogArray", databaseWorker->getDialogList(userId));
    jsonDocument.setObject(dialogList);

    socket->write(jsonDocument.toBinaryData());
    socket->waitForBytesWritten(500);
}



void Server::sendCheckUserResult(const QString &status)
{
    QJsonObject checkUser;
    checkUser.insert("result", "checkUser");
    checkUser.insert("status", status);
    checkUser.insert("currentUserId", databaseWorker->getCurrentUserId());
    checkUser.insert("currentUserName", databaseWorker->getCurrentUserName());
    jsonDocument.setObject(checkUser);

    socket->write(jsonDocument.toBinaryData());
    socket->waitForBytesWritten(500);
}



void Server::sendAddNewUserResult(const QString &status)
{
    QJsonObject addNewUser;
    addNewUser.insert("result", "addNewUser");
    addNewUser.insert("status", status);
    addNewUser.insert("currentUserId", databaseWorker->getCurrentUserId());
    jsonDocument.setObject(addNewUser);

    socket->write(jsonDocument.toBinaryData());
    socket->waitForBytesWritten(500);
}



void Server::addToSocketMap(const QString &userId, QTcpSocket *socket)
{
    socketMap.insert(userId, socket);
    qDebug()<<"addToSocketMap"<<socketMap;
}

void Server::deleteFromSocketMap(const QString &userId)
{
    socketMap.remove(userId);
    std::cerr<<"deleteFromSocketMap "<<std::endl;
}



QTcpSocket *Server::getSocket(const QString &userId)
{

    return socketMap.value(userId);
}

void Server::slotSendUserStatus(const QJsonDocument &jsonDocument)
{
    qDebug()<<"Server::slotSendUserStatus"<<jsonDocument;
    if(jsonDocument.object().value("userStatus").toString() == "1")
    {

        std::cerr<<"userStatus:1"<<std::endl;
        QJsonDocument jsonStatus;
        QJsonObject status;

        QList<QString> dialogIdList = databaseWorker->getDialogIdByUserId(jsonDocument.object().
                                                                          value("authorId").toString());

        for(auto &i_dl:dialogIdList)
        {
            QString recipientId = databaseWorker->
                    getRecipientIdFromDialog_WithoutAuthorId(i_dl,
                                                             jsonDocument.object().value("authorId").toString());


            if(socketMap.contains(recipientId))
            {
                qDebug()<<"socketMap.contains(recipientId)"<<recipientId;
                status.insert("result", "changeUserStatus");
                status.insert("dialogId", i_dl);
                status.insert("userSatus", "1");
                jsonStatus.setObject(status);
                socket = getSocket(recipientId);

                socket->write(jsonStatus.toBinaryData());
                socket->waitForBytesWritten(500);
            }

        }

    }
    else if(jsonDocument.object().value("userStatus").toString() == "0")
    {
        std::cerr<<"userStatus:0"<<std::endl;
        deleteFromSocketMap(jsonDocument.object().value("authorId").toString());
        QJsonDocument jsonStatus;
        QJsonObject status;
        databaseWorker->deleteOnlineUser(jsonDocument.object().
                                         value("authorId").toString());

        QList<QString> dialogIdList = databaseWorker->getDialogIdByUserId(jsonDocument.object().
                                                                          value("authorId").toString());

        for(auto &i_dl:dialogIdList)
        {
            QString recipientId = databaseWorker->
                    getRecipientIdFromDialog_WithoutAuthorId(i_dl,
                                                             jsonDocument.object().value("authorId").toString());

            if(socketMap.contains(recipientId))
            {
                qDebug()<<"00000socketMap.contains(recipientId)"<<recipientId;
                status.insert("result", "changeUserStatus");
                status.insert("dialogId", i_dl);
                status.insert("userSatus", "0");
                jsonStatus.setObject(status);

                socket = getSocket(recipientId);
                socket->write(jsonStatus.toBinaryData());
                socket->waitForBytesWritten(500);
            }

        }
    }
}



void Server::sendMessageToClient(const QString &dialogId,
                                 const QString &authorId,
                                 const QString &recipientId,
                                 const QString &message,
                                 const QString &timestamp)
{
    QJsonObject jsonMessage;
    jsonMessage.insert("result", "incommingMessage");
    jsonMessage.insert("dialogId", dialogId);
    jsonMessage.insert("authorId", authorId);
    jsonMessage.insert("recipientId", recipientId);
    jsonMessage.insert("message", message);
    jsonMessage.insert("timestamp", timestamp);
    jsonDocument.setObject(jsonMessage);

    qDebug()<<"sendMessageToClient"<<jsonDocument;

    QList<QString> ret = databaseWorker->getRecipientIdFromDialog(dialogId);

    for(auto &i:ret)
    {
        if(socketMap.contains(i))
        {
            socket = getSocket(i);
            socket->write(jsonDocument.toBinaryData());
            socket->waitForBytesWritten(500);
        }
    }
}



void Server::sendMessagesHistory(const QString &dialogId)
{
    jsonDocument = databaseWorker->getMessagesHistory(dialogId);
    std::cerr<<"sendMessagesHistory"<<std::endl;
    qDebug()<<jsonDocument;
    socket->write(jsonDocument.toBinaryData());
    socket->waitForBytesWritten(500);
}



