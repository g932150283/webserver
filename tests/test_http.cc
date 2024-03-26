#include "src/http/http.h"
#include "src/log.h"

void test_request() {
    webserver::http::HttpRequest::ptr req(new webserver::http::HttpRequest);
    req->setHeader("host" , "www.sylar.top");
    req->setBody("hello sylar");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    webserver::http::HttpResponse::ptr rsp(new webserver::http::HttpResponse);
    rsp->setHeader("X-X", "sylar");
    rsp->setBody("hello sylar");
    rsp->setStatus((webserver::http::HttpStatus)404);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
