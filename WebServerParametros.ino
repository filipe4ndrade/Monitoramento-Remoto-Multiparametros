#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h> //Biblioteca para funcionamento do sensor de temperatura e umidade DHT11
#include <dht.h>
#include <OneWire.h>  
#include <DallasTemperature.h>


const char* ssid = "Sophia_up";
const char* password = "92555166";

WebServer server(80);

// DHT Sensor
//const int DHTPin = 25;
#define DHTTYPE DHT11 //Tipo do sensor DHT11
#define DHTPIN 25
DHT dht(DHTPIN, DHTTYPE); //Inicializando o objeto dht do tipo DHT passando como parâmetro o pino (DHTPIN) e o tipo do sensor (DHTTYPE)
float temperaturaExt;
float umidadeExt;

//Sensor Turbidez
// --- Protótipo das Funções ---
double calc_NTU(double volt);
// --- Variáveis Globais ---
double NTU = 0.0;
int sensorValue = analogRead(27);

//Sensor pH
#define SensorPin 33         //A saída analógica do medidor de pH está conectada com o analógico do Arduino
unsigned long int avgValue;  //Armazene o valor médio do feedback do sensor
float b;
int buf[10],temp;

//Sensor Temperatura
#define dados 26 /*o pino de dados do sensor está ligado na porta 2 do Arduino*/
 OneWire oneWire(dados);  /*Protocolo OneWire*/
/********************************************************************/
 DallasTemperature sensors(&oneWire); /*encaminha referências OneWire para o sensor*/
 
const int led = 13;

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(9600);
  delay(50);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectando a Rede: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  sensors.begin(); // biblioteca sensor de temp

  server.on("/", handle_OnConnect); //Servidor recebe uma solicitação HTTP - chama a função handle_OnConnect
  server.onNotFound(handle_NotFound); //Servidor recebe uma solicitação HTTP não especificada - chama a função handle_NotFound

  server.begin(); //Inicializa o servidor
  Serial.println("Servidor HTTP inicializado");
}

void loop() {
  server.handleClient(); //Chama o método handleClient()
  delay(2);
}

void handle_OnConnect() {
  //DHT
  temperaturaExt = dht.readTemperature();  //Realiza a leitura da temperatura
  umidadeExt = dht.readHumidity(); //Realiza a leitura da umidade
  float gambiarraT = temperaturaExt*2;
  float gambiarraU = umidadeExt-100;
  //TURBIDEZ
   
  double voltage = sensorValue * (5.0 / 1024.0);  
  NTU = calc_NTU(voltage);
  //PH
  
  for(int i=0;i<10;i++)       //Obtenha 10 valores de amostra do sensor para suavizar o valor
  { 
    buf[i]=analogRead(SensorPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //classificar o analógico de pequeno para grande
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //pegue o valor médio de 6 amostras de centro
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //converter o analógico em milivolt
  phValue=3.5*phValue/10;
  //Temperatura da água
  sensors.requestTemperatures();
float  temperaturaAgua = sensors.getTempCByIndex(0); 
  
  Serial.print("Temperatura: ");
  Serial.print(gambiarraT); //Imprime no monitor serial o valor da temperatura lida
  Serial.println(" ºC");
  Serial.print("Umidade: ");
  Serial.print(gambiarraU); //Imprime no monitor serial o valor da umidade lida
  Serial.println(" %");
  Serial.print("Turbidez");
  Serial.print(NTU);
  Serial.print("pH");
  Serial.print(phValue);
  Serial.print("Temperatura da Água");
  Serial.print(temperaturaAgua);
  
  server.send(200, "text/html", EnvioHTML(gambiarraT, gambiarraU,NTU,phValue,temperaturaAgua)); //Envia as informações usando o código 200, especifica o conteúdo como "text/html" e chama a função EnvioHTML

  
}

void handle_NotFound() { //Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado"); //Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"

}
String EnvioHTML(float Temperaturastat, float Umidadestat, float NTUstat, float phValuestat, float temperaturaAguastat) { //Exibindo a página da web em HTML
  String ptr = "<!DOCTYPE html> <html>\n"; //Indica o envio do código HTML
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n"; //Torna a página da Web responsiva em qualquer navegador Web
  ptr += "<meta http-equiv='refresh' content='2'>";//Atualizar browser a cada 2 segundos
  ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
  ptr += "<title>Monitor de Multiparâmetros</title>\n"; //Define o título da página

  //Configurações de fonte do título e do corpo do texto da página web
  ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #000080;}\n";
  ptr += "body{margin-top: 50px;}\n";
  ptr += "h1 {margin: 50px auto 30px;}\n";
  ptr += "h2 {margin: 40px auto 20px;}\n";
  ptr += "p {font-size: 24px;color: #000000;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>Monitor de Multiparâmetros</h1>\n";
  ptr += "<h2>Temperatura e Umidade Externa</h2>\n";

  //Exibe as informações de temperatura e umidade na página web
  ptr += "<p><b>Temperatura Externa: </b>";
  ptr += (float)Temperaturastat;
  ptr += " Graus Celsius</p>";
  ptr += "<p><b>Umidade Externa: </b>";
  ptr += (float)Umidadestat;
  ptr += " %</p>";
  ptr += "<p><h2>Turbidez</h2>\n";
  ptr += "<p><b>Valor de Turbidez: </b>";
  ptr += (float)NTUstat;
  ptr += "<p><h2>Temperatura da Água</h2>\n";
  ptr += "<p><b>Temperatura: </b>";
  ptr += (float)temperaturaAguastat;
  ptr += " Graus Celsius</p>";
  if (temperaturaAguastat>=20 && temperaturaAguastat<=28){
  ptr += "<p><b> A temperatura está ideal! </b>"; 
  }
  else{
  ptr += "<p><b>Atenção! A temperatura não está ideal! </b>";
  }

  ptr += "<p><h2>Medida do pH</h2>\n";
  ptr += "<p><b>Valor do pH: </b>";
  ptr += (float)phValuestat;
  if (phValuestat>=6.5 && phValuestat<=9.0){
  ptr += "<p><b>O pH está conforme! </b>"; 
  }
  else{
  ptr += "<p><b>Atenção!O ph não está conforme! </b>";
  }


  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;

}

// Equação que relaciona tensão com NTU: NTU = -1120,4*volt*volt + 5742,3*volt - 4352,9
//

double calc_NTU(double volt)
{

  double NTU_val;

  NTU_val = -(1120.4*volt*volt)+(5742.3*volt)-4352.9;

  return NTU_val;
  
}
