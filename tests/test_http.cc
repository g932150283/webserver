#include "src/http/http.h"
#include "src/log.h"

void test_request() {
    webserver::http::HttpRequest::ptr req(new webserver::http::HttpRequest);
    req->setHeader("host" , "www.webserver.top");
    req->setBody("hello webserver");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    webserver::http::HttpResponse::ptr rsp(new webserver::http::HttpResponse);
    rsp->setHeader("X-X", "webserver");
    rsp->setBody("hello webserver");
    rsp->setStatus((webserver::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
