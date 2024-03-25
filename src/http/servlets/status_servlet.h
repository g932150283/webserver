#ifndef __SYLAR_HTTP_SERVLETS_STATUS_SERVLET_H__
#define __SYLAR_HTTP_SERVLETS_STATUS_SERVLET_H__

#include "src/http/servlet.h"

namespace webserver {
namespace http {

class StatusServlet : public Servlet {
public:
    StatusServlet();
    virtual int32_t handle(webserver::http::HttpRequest::ptr request
                   , webserver::http::HttpResponse::ptr response
                   , webserver::http::HttpSession::ptr session) override;
};

}
}

#endif
