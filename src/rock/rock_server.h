#ifndef __WEBSERVER_ROCK_SERVER_H__
#define __WEBSERVER_ROCK_SERVER_H__

#include "src/rock/rock_stream.h"
#include "src/tcp_server.h"

namespace webserver {

class RockServer : public TcpServer {
public:
    typedef std::shared_ptr<RockServer> ptr;
    RockServer(const std::string& type = "rock"
               ,webserver::IOManager* worker = webserver::IOManager::GetThis()
               ,webserver::IOManager* io_worker = webserver::IOManager::GetThis()
               ,webserver::IOManager* accept_worker = webserver::IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;
};

}

#endif
