all: main
main: testProxy.cpp HttpResponse.cpp HttpResponse.h HttpRequest.cpp HttpRequest.h errors.h cache.cpp cache.h ClientRequestInfo.h proxy.cpp proxy.h HttpParser.h HttpParser.cpp
		g++ -g -o main testProxy.cpp HttpResponse.cpp HttpResponse.h HttpRequest.cpp HttpRequest.h errors.h cache.cpp cache.h ClientRequestInfo.h proxy.cpp proxy.h HttpParser.h HttpParser.cpp -lpthread
.PHONY:
		cliean
clean:
		rm -rf *.o main
