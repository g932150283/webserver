#include "rock_server.h"
#include "src/log.h"
#include "src/module.h"

namespace webserver {

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

RockServer::RockServer(const std::string& type
                       ,webserver::IOManager* worker
                       ,webserver::IOManager* io_worker
                       ,webserver::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    m_type = type;
}

void RockServer::handleClient(Socket::ptr client) {
    WEBSERVER_LOG_DEBUG(g_logger) << "handleClient " << *client;
    webserver::RockSession::ptr session(new webserver::RockSession(client));
    session->setWorker(m_worker);
    ModuleMgr::GetInstance()->foreach(Module::ROCK,
            [session](Module::ptr m) {
        m->onConnect(session);
    });
    session->setDisconnectCb(
        [](AsyncSocketStream::ptr stream) {
             ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [stream](Module::ptr m) {
                m->onDisconnect(stream);
            });
        }
    );
    session->setRequestHandler(
        [](webserver::RockRequest::ptr req
           ,webserver::RockResponse::ptr rsp
           ,webserver::RockStream::ptr conn)->bool {
            //WEBSERVER_LOG_INFO(g_logger) << "handleReq " << req->toString()
            //                         << " body=" << req->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, req, rsp, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleRequest(req, rsp, conn);
            });
            return rt;
        }
    ); 
    session->setNotifyHandler(
        [](webserver::RockNotify::ptr nty
           ,webserver::RockStream::ptr conn)->bool {
            WEBSERVER_LOG_INFO(g_logger) << "handleNty " << nty->toString()
                                     << " body=" << nty->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, nty, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleNotify(nty, conn);
            });
            return rt;
        }
    );
    session->start();
}

}
