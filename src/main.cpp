#include <QCoreApplication>
#include <iostream>
#include <Server.h>


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::shared_ptr<Server> server = std::make_shared<Server>();
    if (!server->init()) {
      std::cerr << "Error while initing server object" << std::endl;
      QCoreApplication::exit(-1);
    }

    return app.exec();
}
