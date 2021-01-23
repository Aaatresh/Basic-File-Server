#include<ESP8266WiFi.h>
#include<ESP8266WiFiMulti.h>
#include<ESP8266mDNS.h>
#include<ESP8266WebServer.h>
#include<FS.h>

#define ssid "ssid"
#define passwd "password"
#define username "username"
#define password "password"

ESP8266WiFiMulti multi;
ESP8266WebServer server(80);

bool handleFileRead(String );
String getContentType(String );
void handleFileUpload();
void list_files();
void login();
void hoome();

File file;
String filename;
String contentType;
String first_part="<h1>FILE SERVER</h1><br><h4>FILE CONTENTS</h4>";
String last_part="<br><form action=\"/format\" method=\"POST\"><input type=\"submit\" value=\"format\"></form><br><h4>View File</h4><form action=\"/getFile\" method=\"POST\">Enter the exact name of the file to be viewed:<br><input type=\"text\" name=\"filename\" placeholder=\"Enter exact filename\"><input type=\"submit\" value=\"Get File\"></form><br><br><h4>Upload to Server</h4><form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"name\"><input class=\"button\" type=\"submit\" value=\"Upload\"></form><br><br><h4>Delete a file from the server</h4><form action=\"/delete\" method=\"post\"><input type=\"text\" name=\"del\" placeholder=\"Exact filename\"><br><input type=\"submit\" value=\"delete\"></form>";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  multi.addAP(ssid,passwd);

  Serial.print("connecting");
  while(multi.run()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("IP:");
  Serial.println(WiFi.localIP());

  SPIFFS.begin();

  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
    {
      server.send(404,"text/plain","Error 404");
    }
  });

  server.on("/list",HTTP_GET,list_files);

  server.on("/login",HTTP_POST,login);

  server.on("/format",HTTP_POST,[](){
    SPIFFS.format();
    hoome();
    server.send(303);
  });

  server.on("/upload",HTTP_POST,[](){
    server.send(303);
  },handleFileUpload);

  server.on("/delete",HTTP_POST,[](){
    SPIFFS.remove(server.arg("del"));
    hoome();
    server.send(303);
  });

  server.on("/getFile",HTTP_POST,[](){

    filename=server.arg("filename");
    //filename="/data/"+filename;

    handleFileRead(filename);
    server.send(303);
  });
  
  server.begin();
  Serial.println("FTP server initiated");
  
}

void loop() {
  // put your main code here, to run repeatedly:

  server.handleClient();

}

bool handleFileRead(String path)
{
  if(path.endsWith("/"))
    path=path+"index.html";

  contentType=getContentType(path);

  if(SPIFFS.exists(path))
  {
    file=SPIFFS.open(path,"r");
    server.streamFile(file,contentType);
    file.close();

    return true;
  }

  server.send(404,"text/plain","File Not Found");
  return false;
}

String getContentType(String filename)
{
  if(filename.endsWith(".html"))
    return "text/html";
  else if(filename.endsWith(".pdf"))
    return "application/pdf";

  return "text/plain";
  
}

void handleFileUpload()
{
  HTTPUpload& upload=server.upload();

  if(upload.status==UPLOAD_FILE_START)
  {
    filename=upload.filename;

    if(!filename.startsWith("/data/"))
      filename="/data/"+filename;

    file=SPIFFS.open(filename,"w");
  }
  else if(upload.status==UPLOAD_FILE_WRITE)
  {
    if(file)
    {
      file.write(upload.buf,upload.currentSize);    
    }  
  }
  else if(upload.status==UPLOAD_FILE_END)
  {
    if(file)
    {
      file.close();
      hoome();
      server.send(303);
    }
    else
    {
      server.send(500,"text/plain","File Not Created");
    }
  }
  
}

void list_files()
{
  Dir dir=SPIFFS.openDir("/");
  String str="";

  while(dir.next())
  {
    str=str+dir.fileName()+"\t\t"+dir.fileSize()+"\r\n";
  }

  server.send(303,"text/plain",str);
}

void login()
{
  
  if(server.arg("username")==username && server.arg("password")==password)
  {
    hoome();
  }
  else 
  {
    server.sendHeader("Location","/login.html");
    server.send(303);
  }
  
}

void hoome()
{
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  
  Dir dir=SPIFFS.openDir("/data");
  String str="";
  String info="";

  info="Total space available in bytes: "+String(fs_info.totalBytes)+"  Used space in bytes: "+String(fs_info.usedBytes);
  
  while(dir.next())
  {
    str=str+dir.fileName()+"      "+dir.fileSize()+"<br>";
  }
  
  server.send(303,"text/html",info+first_part+str+"<br>--"+last_part);
}

