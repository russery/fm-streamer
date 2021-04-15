#include <ESP8266WiFi.h>


String PrepareHtmlPage_()
{
  String htmlPage;
  htmlPage.reserve(1024);               // prevent ram fragmentation
  htmlPage = F("HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n"  // the connection will be closed after completion of the response
               "Refresh: 5\r\n"         // refresh the page automatically every 5 sec
               "\r\n"
               "<!DOCTYPE HTML>"
               "<html>"
               "HELLO FUCKFACE!!");
  htmlPage += F("</html>"
                "\r\n");
  return htmlPage;
}


WebPage::WebPage(uint port){
	server_ = new WiFiServer(port);
}

WebPage::~WebPage(void){
	delete server_;
}


void WebPage::Start(void){
	server_->begin();
}

void WebPage::Loop(void){
	WiFiClient client = server_->available();
	// wait for a client (web browser) to connect
	if (client) {
	while (client.connected()) {
	  // read line by line what the client (web browser) is requesting
	  if (client.available()) {
	    String line = client.readStringUntil('\r');
	    Serial.println(line);
	    // wait for end of client's request, that is marked with an empty line
	    if (line.length() == 1 && line[0] == '\n')
	    {
	      client.println(PrepareHtmlPage_());
	      break;
	    }
	  }
	}

	while (client.available()) {
	  // but first, let client finish its request
	  // that's diplomatic compliance to protocols
	  // (and otherwise some clients may complain, like curl)
	  // (that is an example, prefer using a proper webserver library)
	  client.read();
	}

	// close the connection:
	client.stop();
	Serial.println("[Client disconnected]");
	}
}