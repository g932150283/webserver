#ifndef __WEBSERVER_HTTP_SERVLETS_CONFIG_SERVLET_H__
#define __WEBSERVER_HTTP_SERVLETS_CONFIG_SERVLET_H__

#include "src/http/servlet.h"

namespace webserver {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(webserver::http::HttpRequest::ptr request
                   , webserver::http::HttpResponse::ptr response
                   , webserver::http::HttpSession::ptr session) override;
};

}
}

#endif
