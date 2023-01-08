#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>  //version 3.0.0
#include <ArduinoJson.h>
#include <DNSServer.h>

#define RELAY 2

void initWiFiAP();
void InitWiFi();
void initServer(); //iniciamos la funcion 
void NotFound(AsyncWebServerRequest *request);
void RedesRequest(AsyncWebServerRequest *request);
void ConectarWiFi(AsyncWebServerRequest *request);
void ENCENDER(AsyncWebServerRequest *request);
void APAGAR(AsyncWebServerRequest *request);
String BuscarRedes();
String header;
String SSID="", PASS="";
const String lugar = "sala";
bool wifiIsConnected=false;
bool indicadorLed;
Preferences preferences;

StaticJsonDocument<1024> myJson;
String myJsonStr;
long tiempoDeConexion;

AsyncWebServer server(80);
DNSServer dnsServer;

class myHandler : public AsyncWebHandler { //myHandler hereda todo de AsyncWebHandler
  public:
    myHandler() {} //constructor
    virtual ~myHandler() {} //destructor

    bool canHandle(AsyncWebServerRequest *request){
      Serial.println("Entramos al canHandle");
      return true; //para cualquier tipo de peticion
    }
    
    void handleRequest(AsyncWebServerRequest *request) {
      Serial.println("Entramos al handle");
      request->send(SPIFFS, "/index.html"); //redireccionar a la pagina principal
    }
};

void setup() {

  Serial.begin(115200);
  SPIFFS.begin();
  Serial.println();
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  InitWiFi();
}

void loop(){
  /*WiFiClient client = server.available();
  if(!client){
    return;
  }  
  serial.println("hay alguien conectado");
  //aviso de alguien conectado
  while (!cliente.available())
  {
    digitalWrite(RELAY, LOW); //creo que es un aviso de espera
    Serial.println("En espera de clinte el led esta apagado?")
    delay(1);
  }
  
  String request2 = Client.readStringuntil('\r');
  Serial.println(request2);
  client.flush();*/
  //cuando este desconectado
  if(WiFi.status() != WL_CONNECTED){  //se hace para que se mantenga a la escucha de las peticiones
  //cuando esta desconectado, ya que en cuanto se pida una peticion nos enviara directamente
  //a la pagina principal
    dnsServer.processNextRequest();
  }
  
}

void NotFound(AsyncWebServerRequest *request){
  request->send(SPIFFS, "/indexNotFound.html");
  Serial.println("pagina no encontrada");
}
void ENCENDER(AsyncWebServerRequest *request){
  request->send(SPIFFS, "/RELAY=ON.html"); //a donde me tiene que llevar
  digitalWrite(RELAY, HIGH);
  indicadorLed= true; //indica al programa que esta encendido el LED
  Serial.println("se enciente el LED");
}
void APAGAR(AsyncWebServerRequest *request){
  request->send(SPIFFS, "/RELAY=OFF.html"); //a donde me tiene que llevar
  digitalWrite(RELAY, LOW);
  indicadorLed= false; //indica al programa que esta encendido el LED
  Serial.println("entramos a apagar, y se apagamos el LED");
}
void initWiFiAP(){
  WiFi.mode(WIFI_AP);
  while(!WiFi.softAP(lugar.c_str())){
    Serial.println(".");
    delay(100);  
  }
  Serial.print("Direccion IP AP: ");
  Serial.println(WiFi.softAPIP());
  initServer();
}

/*void RedesRequest(AsyncWebServerRequest *request){
 Serial.println("Peticion Redes");
 request->send(200, "text/plain", BuscarRedes().c_str());
}*/

String BuscarRedes(){

  WiFi.scanNetworks(true, false, true, 100);

  while(WiFi.scanComplete()<0);
  Serial.println();

  int numeroDeRedes = WiFi.scanComplete();
  Serial.print(numeroDeRedes);
  Serial.println(" redes encontradas");
  
  if (numeroDeRedes > 0) {

    for (int i = 0; i < numeroDeRedes; i++) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" "); 
      Serial.print(WiFi.RSSI(i));
      Serial.print("dBm ");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?"Abierta":"Cerrada");
    }
    
    JsonArray Redes = myJson.createNestedArray("Redes");
    for (int i = 0; i < numeroDeRedes; i++) {
      Redes[i]["SSID"] = WiFi.SSID(i);
      Redes[i]["RSSI"] = WiFi.RSSI(i);
      Redes[i]["Estado"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?"Abierta":"Cerrada";
    }

    myJsonStr = "";
    serializeJson(myJson, myJsonStr); 

  } else {
    Serial.println("No se encontraron redes :(");
  }

  return myJsonStr;
}


void ConectarWiFi(AsyncWebServerRequest *request){
  Serial.print("Peticion de conectar a ");
  Serial.println(SSID);
  if(request->hasParam("SSID")){
    SSID = request->arg("SSID");
  }

  if(request->hasParam("PASS")){
    PASS = request->arg("PASS");
    Serial.print("Clave ");
    Serial.println(PASS);
  }

  SSID.trim();
  PASS.trim();
  
  Serial.print("SSID: "); 
  Serial.println(SSID);
  Serial.print("PASS: ");
  Serial.println(PASS);

  preferences.begin("Red", false);
  preferences.putString("SSID", SSID);
  preferences.putString("PASS", PASS);
  preferences.end();
  
  request->send(SPIFFS, "/index.html"); 

  ESP.restart();
}

void InitWiFi(){

  preferences.begin("Red", false);
  SSID = preferences.getString("SSID", ""); 
  PASS = preferences.getString("PASS", "");
  Serial.print("Ultima Conexión a: ");
  Serial.println(SSID);
  Serial.print("Ultima Contraseña: ");
  Serial.println(PASS);
  //preferences.clear();
  preferences.end();
  
  if(SSID.equals("") || PASS.equals("")){
    Serial.println("No se encontraron credenciales");
    initWiFiAP();
  }else{
    WiFi.mode(WIFI_STA);      
    // Inicializamos el WiFi con nuestras credenciales.
    Serial.print("Conectando a ");
    Serial.print(SSID);

    WiFi.begin(SSID.c_str(), PASS.c_str());
    
    tiempoDeConexion = millis();
    while(WiFi.status() != WL_CONNECTED){     // Se quedata en este bucle hasta que el estado del WiFi sea diferente a desconectado.
      Serial.print(".");
      delay(100);
      if(tiempoDeConexion+7000 < millis()){
        break;
      }
    }

    if(WiFi.status() == WL_CONNECTED){        // Si el estado del WiFi es conectado entra al If
      Serial.println();
      Serial.println();
      Serial.println("Conexion exitosa!!!");
      Serial.println("");
      Serial.print("Tu IP STA: ");
      Serial.println(WiFi.localIP());
    }

    if(WiFi.status() != WL_CONNECTED){
      Serial.println("");
      Serial.println("No se logro conexion");
      initWiFiAP();
    }
  }
}

void initServer(){
  Serial.println("Iniciando el servidor index");
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  //server.on("/Buscar", HTTP_GET, RedesRequest); //peticion buscar
  server.on("/RELAY=ON", HTTP_GET, ENCENDER);
  server.on("/RELAY=OFF", HTTP_GET, APAGAR);
  //server.on("/ConectarWiFi", HTTP_GET, ConectarWiFi); //peticion conectar
  server.onNotFound(NotFound); //pagina no encontrada

  dnsServer.start(53, "*", WiFi.softAPIP()); //escucha por el puerto 53 y todas las peticiones "*"
  //las redirecciona a WiFi.softAPIP() (otra ip una ip de nuestro esp32 en modo Acces Point)
  server.addHandler(new myHandler()).setFilter(ON_AP_FILTER);
  //la instruccion anterios .setFilter(ON_AP_FILTER) pregunta si la ip viene del acces point
  //myHandler() es una clase
	server.begin();
  Serial.println("Servidor iniciado");
}

