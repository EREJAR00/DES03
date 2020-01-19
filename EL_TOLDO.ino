//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------Toldos Almeida S.L.---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------Versión 1.0.1---18/01/2020--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------Autores: Víctor Canseo Pacual, Edgardo Juan Rejas Ramos, Adolfo Trincado Rodriguez------------------------------------------------------------------------------------------------------------------------
//------Todos los derechos reservados-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------El código se divide en las siguientes partes:-------------------------------------------------------------------------------------------------------------------------------------------------------------
//------0.-Definición de las variables globales y las bibliotecas-------------------------------------------------------------------------------------------------------------------------------------------------
//------1.-Definición del Setup-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------2.-Definición de las funciones del programa en el orden: Medida, Cálculo, Control, LCD, Reglas, Motor, STOP-----------------------------------------------------------------------------------------------
//------3.-Definición del Loop------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


#include <Wire.h>                       //Libreria del protocolo I2C.
#include <LiquidCrystal_I2C.h>          //Librería de control del display.
#include <DHT.h>                        //Librería de control de sensor de Ta y Humedad.
                                        //Definimos Entradas.
const int EViento           = A0;       //Tag de Control 0.
const int ELluvia           = A1;       //Tag de Control 1.
const int EUV               = A6;       //Tag de Control 6.
const int ELuz              = A7;       //Tag de Control 7.
const int ETemperatura      = 13;       //Tag de Control 3, incluye Humedad.
const int EFinCarrera       = 9;        //Fin de Carrera.
const int EPosicion         = 8;        //Pulsador de abierto o cerrado.
const int EAutomatico       = 7;        //Pulsador selector modo automático / manual.
const int EStopEmergencia   = 2;        //Pulsador Parada de Emergencia, usamos pin especial para detección más rápida.

const int ledProteccion     = 12; 
const int ledManual         = 11;
const int ledAutomatico     = 10;

const int SMotor1           = 5;                      //Definimos Salidas, donde hay que determinar si 1 abre o si cierra.
const int SMotor2           = 4;
                                                      //Definimos las variables globales.
const int tiempoCriticas    = 750;                    //Tiempo del timer de lectura de las variables críticas.                                                      
const int tiempoNoCriticas  = 4;                      //Tiempo del timer de lectura de las variables no críticas.
const int tiempoControl     = 10;                     //Número de lecturas de control de las variables.
const int tiempoPosicion    = 1500;                   //Tiempo de espera entre pulsaciones de botón funcionales.
const int tiempoApertura    = 1500;
boolean estadoToldo[4]      = {false, false, false};  //[Posición (abierto es true) en modo manual, automático o manual (automático es true) [Se establece así ya que true equivale a un uno lógico],
                                                      //Si el toldo está abierto o cerrado (abierto es true), Posición (abierto es true) en modo automatico]

                                                      
volatile boolean funcionando = true;                  //Este es un punto crítico del programa, pues equivale al botón de seguridad. 
                                                      //Nuestro diseño no incorpora actualmente una para de emergencia analógica, pero digitalmente añadimos esta.
                                                      //Si esta variable se pone en "false", el programa para los motores y los bloquea hasta que no se reinicie.
boolean proteccion          = false;                  //Indicador de si el toldo está bloqueado debido al clima o no.
boolean bloqueoCierre       = false;                  //Encargada de bloquear las opciones de poner el toldo en modo apertura mientras se hace el proceso de cierre, por falta de segundo fin de carrera.
boolean bloqueoLCD          = false;                  //Sirve para los casos en los que se da más de una emergencia, solo presentando una de ellas. La emergencia que presenta, en caso de que se presenten varias,
                                                      //viene definida arbitrariamente por el código y decidida sin ningún criterio particular.


                                                      //Definimos vectores que han de ser inicializados pero no pueden serlo en cada bucle, o perderíamos información.
int valores[7]              = {0, 0, 0, 0, 0, 0, 0};  //Viento, Lluvia, UV, Luz, Humidex, temepratura, humedad.
int resViento[3]            = {0, 0, tiempoControl};  //Todos los vectores res tienen el esquema (Valor acumulado y posteriormente valor de la media, 1 si se ha acabado el cálculo de la media).
int resLluvia[3]            = {0, 0, tiempoControl};
int resUV[3]                = {0, 0, tiempoControl};
int resLuz[3]               = {0, 0, tiempoControl};
int resTemperatura[3]       = {0, 0, tiempoControl};
int resHumedad[2]           = {0, 0};                   //Este no tiene contador porque va ligado directamente al de temperatura.
int timerPrecipitaciones    = 0;                        //Timer para considerar cuando la lluvia es continuada.
int timerPosicion           = 0;                        //Bloqueo del cambió de posición para evitar pulsaciones fantasma.
int timerAutomatico         = 0;                        //Bloqueo del cambió de modo de funcionamiento para evitar pulsaciones fantasma.
int timerTexto              = 0;                        //Timer para que los textos cíclicos (temperatura y humedad) se presenten correctamente.
int timerApertura           = 0;                        //Timer para la apertura del toldo, debido que no disponemos de un segundo final de carrera por la forma del prototipo.
int timerNoCriticas         = 0;                        //Timer de lectura de las variables no críticas.
int timerCriticas           = 0;                        //Timer de lectura de las variables críticas.


                                                        
#define DHTPIN ETemperatura               //Definimos los datos requeridos por las librerías.
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);


void STOP(){
  funcionando = false;
}

void setup() {
  pinMode(EViento, INPUT);            //Configuramos Entradas y Salidas.
  pinMode(ELluvia, INPUT);
  pinMode(EUV, INPUT);
  pinMode(ELuz, INPUT);     
  pinMode(ETemperatura, INPUT);
  pinMode(EPosicion, INPUT);
  pinMode(EAutomatico, INPUT);
  pinMode(EStopEmergencia, INPUT);
  pinMode(SMotor1, OUTPUT);
  pinMode(SMotor2, OUTPUT);
  pinMode(ledProteccion, OUTPUT);
  pinMode(ledManual, OUTPUT);
  pinMode(ledAutomatico, OUTPUT);    
  dht.begin();                        //Iniciamos el sensor de Ta y Humedad y la LCD.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
  lcd.begin();
  lcd.backlight();
  int finCarrera=digitalRead(EFinCarrera);
  while (EFinCarrera == 1){           //Cerramos el toldo en caso de que se hubiese quedado abierto por algún problema.
    bloqueoCierre == true;
    digitalWrite(SMotor1,HIGH);
    digitalWrite(SMotor2,LOW);
    if (finCarrera == 0){
        digitalWrite(SMotor1, LOW);           //En el caso del cierre, no se establece que el toldo se encuentra cerrado hasta que se completa el mismo. 
        estadoToldo[0] = false;               //Esto se debe a que la apertura se realiza mediante tiempo de reloj y no usa un fin de carrera por falta de recursos.
        estadoToldo[3] = false;
        estadoToldo[2] = false;
        bloqueoCierre = false;
        timerApertura = tiempoApertura;      //El timer de apertura del toldo solo se reinicia en caso de 
    } 
  }
}


        
int FuncionMedida(int valVariable, int EVariable, int ControlDHT){        //En esta función hacemos el proceso de tomar una medida. 
                                                                          //Dicha medida es la media de 10 para evitar variacioes en caso de un fallo en una medida única.
  int i = 0;
  int val = 0;
  valVariable = 0;
  for(i=0; i<10; i++)  {            //Bucle donde hacemos las diez medidas, diferenciando que variable necesita que función para medir,
                                    //de la forma más generalizada permitida por los elementos de los que disponemos.         
      switch (ControlDHT) {
        case 0:
                val = analogRead(EVariable);
                break;
        case 1:
                val = dht.readTemperature();
                break;
        case 2:
                val = dht.readHumidity();
                break;
      }
      valVariable = valVariable + val;
    }
    valVariable = valVariable/10;   //Realizamos la media y devolvemos el valor al programa, para mandarlo al cálculo
    return valVariable;
    
}


void FuncionCalculo(int valVariable, int resVariable[3], int tagVariable, int valHumedad){    //Esta función es muy similar a la de medida, pero lo que hace es guardar diez medidas y hacer la media de ellas,
                                                                                              //buscando hacer medidas separadas por intervalos de tiempo constantes.
                                                                                              //De esta manera, las variables pueden ser separadas en críticas y no críticas,
                                                                                              //y cada una tener un intervalo dentre medidas distinta. Esto está pensado para evitar picos,
    resVariable[0] = resVariable[0] + valVariable;                                            //como los que podría dar una racha de viento momentanea o una nube pasajera.                                     
    resVariable[2] = resVariable[2] - 1;            
    if (tagVariable == 3)                            //Al entregarse con la temperatura ya que tienen que estar ligadas y, por tanto, requieren la seguridad de dar el valor final al mismo tiempo,
      resHumedad[0] = resHumedad[0] + valHumedad;    //se añade un campo para que pasada la temperatura, se calcule la humedad.
    switch (resVariable[2]){
      case 0:                                      //Cuando el timer llega a cero, se hace la media de las medidas y se marca con un 1 el que se puede tomar como valor lo obtenido
        resVariable[0] = resVariable[0]/tiempoControl;      
        resVariable[1] = 1;
          if (tagVariable == 3) {
            resHumedad[0] = resHumedad[0]/tiempoControl;
            resHumedad[1] = 1;
          }
        break; 
      case -1:             //Un bucle después de que la media esté acabada, y esta habiendo sida almacenada en la FuncionControl, se reinician los valores y se reestablece el contador para que empiece de nuevo.                        
          resVariable[0] = 0; 
          resVariable[2] = tiempoControl;  
          if (tagVariable == 3) 
              resHumedad[0] = 0;
        break;
    }  
}

void FuncionControl(int resVariable[2], int tagVariable){     //Aquí operamos los valores necesarios, y los almacenamos para su uso sobre el motor. Es donde se guardan los valores de Control del motor.

  resVariable[1] = 0;
  switch (tagVariable){
    case 0: //Viento
      valores[0] = resVariable[0];
      break; 
      
    case 1: //LLuvia
      valores[1] = resVariable[0];
      break;
        
     case 6: //UV
      valores[2] = resVariable[0];
      break;
        
     case 7: //Luz
      valores[3] = resVariable[0];
      break;
        
     case 3: //Temperatura y humedad
      resHumedad[1] = 0;
      valores[5] = resVariable[0];        //Se guardan los valores de temperatura y humedad para la LCD
      valores[6] = resHumedad[0];
      float dewpoint = 273.1 +(resVariable[0] - ((100 - resHumedad[0])/5)); //"Td = 273.1 + T - ((100 - RH)/5.)" where Td is dew point temperature (in Kelvin), T is observed temperature (in degrees Celsius),
                                                                            //and RH is relative humidity (in percent). Apparently this relationship is fairly accurate for relative humidity values above 50%. 
      float e =  6.11 * exp (5417.7530 * ( (1/273.16) - (1/dewpoint) ) );   //e = vapour pressure in hPa (mbar), given by: e = 6.11 * exp [5417.7530 * ( (1/273.16) - (1/dewpoint) ) ],
                                                                            //where dewpoint is expressed in Kelvins (temperature in K = temperature in °C + 273.1) and 5417.7530 is a rounded constant based on
                                                                            //the molecular weight of water, latent heat of evaporation, and the universal gas constant. 
      float h = (0.5555)*(e - 10);                                          //h = (0.5555)*(e - 10.0)
      float humidex = ((resVariable[0]) + h);                               //humidex = (air temperature) + h
      valores[4] = round(humidex);
      break;
  }
}

void FuncionLCD (int textoRotativo){     //Función de impresión en la LCD

    lcd.setCursor(0,0);                  //Corresponde a la linea superior de la LCD, la cual presentará valores de Ta y Humedad mientras no precise ninguna información especial, y las accciones del toldo.
    switch (textoRotativo){
      case 100:   //Texto arranque
        lcd.print(" TOLDOS ALMEIDA ");
        break;
      case 110:
        lcd.print("Temperatura:");
        lcd.print(valores[5]);
        lcd.print(" C");
        break;  
      case 120:
        lcd.print("  Humedad:");
        lcd.print(valores[6]);
        lcd.print(" %  ");;
      case 130:
        lcd.print("Toldo Bloqueado ");
        break;
      case 180:
        lcd.print("Toldo cerrandose");
        break;
      case 190:
        lcd.print("Toldo abriendose");            
        break;
    }
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------  
    lcd.setCursor(0,1);
    if (proteccion == true){
      lcd.print("Modo Proteccion ");
      digitalWrite(ledProteccion, HIGH);
    }
    else{                 
      
      digitalWrite(ledProteccion, LOW);
      switch (estadoToldo[1]){
         case true:
          lcd.print("Modo: Automatico ");
          digitalWrite(ledManual, LOW);
          digitalWrite(ledAutomatico, HIGH);
          break;
        case false :
          lcd.print("  Modo: Manual  ");
          digitalWrite(ledManual, HIGH);
          digitalWrite(ledAutomatico, LOW);
          break;
      }
    }
   
}

void FuncionReglas (int valores[5], boolean estadoToldo[4]){ //Nuestro toldo funciona por una programación mediante reglas. Para ello se crean una serie de funciones if que funcionan como reglas de activación.
  
  int vientoAlto = 200;        //Valor del motor para más de 26Km/h, continuado
  int lluviaAlto = 350;        //Valor de tormenta > 10mm/h


  if (valores[0] >= vientoAlto){          //Estos son los valores de protección                    
     estadoToldo[0] = false;
     estadoToldo[3] = false;
     proteccion = true;
     FuncionLCD(130);
  }
  else if (valores [1] >= lluviaAlto) { 
    estadoToldo[0] = false;
    estadoToldo[3] = false;
    proteccion = true;
    FuncionLCD(130);
  }
  else 
    proteccion = false;


  if (proteccion == 0){
    
    switch (estadoToldo[1]){
      case false: 
        int posicion = digitalRead(EPosicion);
        if (posicion == 1 && estadoToldo[0] == false && timerPosicion <= 0 && bloqueoCierre == false) {   //apertura
          estadoToldo[0] = true;
          timerPosicion = tiempoPosicion;
        }
        if (posicion == 1 && estadoToldo[0] == true && timerPosicion <= 0) {    //cierre
          estadoToldo[0] = false;
          timerPosicion = tiempoPosicion;
        }
        else {
          timerPosicion = timerPosicion - 1;
          if (timerPosicion < -254)
            timerPosicion = -5;
        } 
      break;

      case true:
        int valorApertura = 5 + 1;  //Es el número de reglas creadas más 1. Está para que si todas las condiciones consideran que deberia estar cerrado
                                    //y estan en nivel de cerrar el toldo pero una lo necesita abrir, que se abra.
        int i = 0;                  //Si este valor da más de cero, entonces el toldo se abre; menos de cero lo cierra. Se usa para no cerrar el toldo mientras se estudian todas las posibilidades
        
        if (valores[2] >= 6)        //Reglas para los valores ultravioleta
          i = i + valorApertura;
        else if (valores[2] >= 3)
          i = i + 1;
        else
          i = i - 1;
          
        if (valores[4] >= 30)     //Reglas para el valor Humidex (híbrido de Ta y Humedad)
          i = i + valorApertura; 
        else if (valores[4] >= 26)
          i = i + 1;
        else
          i = i - 1;     

        if (valores[3] >= 2000)
          i = i + valorApertura;
        else if (valores[3] >= 2000)
          i = i +1;
        else
          i = i - 1;


        if (i > 0)              //Mandamos si el toldo tiene que estar abierto o cerrado en modo automático
          estadoToldo[3] = true;
        else if (i < 0)
          estadoToldo[3] = false;
          
      break;
    }
  } 
}

void FuncionMotor(){
  
  int finCarrera = digitalRead(EFinCarrera);    //Cierre
  if ((estadoToldo[0] == false && proteccion == false && estadoToldo[2] == true && estadoToldo[1] == false) || (estadoToldo[2] == true || proteccion == true) || (estadoToldo[3] == false && proteccion == false && estadoToldo[2] == true && estadoToldo[1] == true)){     
      bloqueoCierre = true;
      digitalWrite(SMotor1, HIGH);
      digitalWrite(SMotor2, LOW);
      if (proteccion == false)
        FuncionLCD(180);
      if (finCarrera == 0){
        digitalWrite(SMotor1, LOW);           //En el caso del cierre, no se establece que el toldo se encuentra cerrado hasta que se completa el mismo.
        estadoToldo[2] = false;               //Esto se debe a que la apertura se realiza mediante tiempo de reloj y no usa un fin de carrera por falta de recursos
        estadoToldo[0] = false;
        estadoToldo[3] = false;
        bloqueoCierre = false;
        timerApertura = tiempoApertura;       //El timer de apertura del toldo solo se reinicia en caso de 
      }
  }
        //Apertura
  if ((estadoToldo[0] == true && proteccion == false && bloqueoCierre == false && estadoToldo[1] == false) || (estadoToldo[3] == true && proteccion == false  && bloqueoCierre == false && estadoToldo[1] == false)){     
      timerApertura = timerApertura - 1;
      digitalWrite(SMotor1, LOW);
      digitalWrite(SMotor2, HIGH);
      estadoToldo[0] = true;
      estadoToldo[3] = true;
      estadoToldo[2] = true;
      if (proteccion == false)
        FuncionLCD(190);
      if (timerApertura <= 0)
        digitalWrite(SMotor2, LOW);
  }
}



void loop() {

  attachInterrupt(digitalPinToInterrupt(EStopEmergencia),STOP,CHANGE);              //Botón de emergencia. Tras leer un cambio se mueve directamente a este punto para hacer el cambio y luego sigue su camino.
                                                                                    //Cuando está funcionando se han incluido posiciones de seguridad para evitar que se quede demasiado tiempo sin funcionar,
                                                                                    //y en cuestión de centésimas de segundo el circuito se redirige a esta posición,
  Break:                                                                            //cesando toda actividad por salirse del bucle de trabajo
  if (funcionando){
       
    int automatico = digitalRead(EAutomatico);                                      //Comprobamos la posición que se requiere para el toldo, inicializada a manual
    if (automatico == 1 && estadoToldo[1] == false && timerAutomatico <= 0) {       //Si el botón se pulsa, y se está en modo manual cambia
      estadoToldo[1] = true;
      timerAutomatico = tiempoPosicion;  //Bloqueo temporal                                         
    }
    if (automatico == 1 && estadoToldo[1] == true && timerAutomatico <= 0) {        //Si el botón se pulsa, y se está en modo automático cambia 
      estadoToldo[1] = false;
      timerAutomatico = tiempoPosicion; //Bloqueo temporal
    }
    else {
      timerAutomatico = timerAutomatico - 1;                                        //Control de que el timer no pase involuntariamente a valores positivos
      if (timerAutomatico < -254)
        timerAutomatico = -5; 
    }
    if (!funcionando)   //Parada de emergencia
      goto Break;

    if (timerCriticas > 0){                //Timer para que las medidas no críticas tengan más espaciameinto temporal
      timerCriticas = timerCriticas - 1;
      if (!funcionando)   //Parada de emergencia
        goto Break;
    }
    else if (timerNoCriticas == 0) { 
      int valViento = 0;
      int tagViento = 0;
      int valHumedad = 0;
      valViento = FuncionMedida(valViento, EViento, 0);
      FuncionCalculo(valViento, resViento, tagViento, valHumedad);
      if (resViento[1] == 1)
        FuncionControl(resViento, tagViento);
      if (!funcionando)   //Parada de emergencia
        goto Break;
      
      int valLluvia = 0;
      int tagLluvia = 1;
      valLluvia = FuncionMedida(valLluvia, ELluvia, 0);
      FuncionCalculo(valLluvia, resLluvia, tagLluvia, valHumedad);
      if (resLluvia[1] == 1)
        FuncionControl(resLluvia, tagLluvia);
      if (!funcionando)
        goto Break;
      
      
      if (timerNoCriticas > 0){                //Timer para que las medidas  críticas tengan más espaciameinto temporal
        timerNoCriticas = timerNoCriticas - 1;
        if (!funcionando)   //Parada de emergencia
          goto Break;
      }
      else if (timerNoCriticas == 0) {          //Medida variables no críticas
            
          int valUV = 0;
          int tagUV = 6;
          valUV = FuncionMedida(valUV, EUV, 0);
          FuncionCalculo(valUV, resUV, tagUV, valHumedad);
          if (resUV[1] == 1)
            FuncionControl(resUV, tagUV);
          if (!funcionando)   //Parada de emergencia
            goto Break;
        
          int valLuz = 0;
          int tagLuz = 7;
          valLuz = FuncionMedida(valLuz, ELuz, 0);
          FuncionCalculo(valLuz, resLuz, tagLuz, valHumedad);
          if (resLuz[1] == 1)
            FuncionControl(resLuz, tagLuz);
          if (!funcionando)   //Parada de emergencia
            goto Break;
    
          int valTemperatura = 0;
          valHumedad = 0;
          int tagTemperatura = 3;
          valTemperatura = FuncionMedida(valTemperatura, ETemperatura, 1);
          valHumedad = FuncionMedida(valHumedad, ETemperatura, 2);
          FuncionCalculo(valTemperatura, resTemperatura, tagTemperatura, valHumedad);
          if ( resTemperatura[1] == 1 && resHumedad[1] == 1) 
            FuncionControl(resTemperatura, tagTemperatura);
          if (!funcionando)   //Parada de emergencia
            goto Break;
                
          timerNoCriticas = tiempoNoCriticas;       //Reiniciamos el timer
          }
        timerCriticas = tiempoCriticas;
    }
    else
      lcd.println("ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_");
       
    FuncionReglas(valores, estadoToldo);
    if (!funcionando)   //Parada de emergencia
      goto Break;
    
    FuncionMotor;
    if (!funcionando)   //Parada de emergencia
      goto Break;
    
    switch (timerTexto){    //Aquí definimos el texto cíclico que reproduce nuestra LCD. Funciona de tal manera que enseña un texto inicial de presentación y luego a intervalos los valores de Ta y Humedad.
      case 0:
        FuncionLCD(100);
        break;
      case 1500:
        FuncionLCD(110);
        break;
      case 3000:
        FuncionLCD(120);
        break;
    }
    if (!funcionando)   //Parada de emergencia
      goto Break;

    timerTexto = timerTexto + 1;        //Al ponerlo a uno en el reinicio y no a cero forzamos que no pase por el caso 0 y no vuelva a mostrar el caso inicial
    if (timerTexto > 3000) 
        timerTexto = 1;
  }
  
  else{                                 //En caso de una emerjencia, el Loop solo reproducirá este trozo, donde se obliga a parar el motor, se colocan todos los leds encendidos como alarma,
    digitalWrite(SMotor1, LOW);         //y se reproduce un texto indicativo
    digitalWrite(SMotor2, LOW);
    digitalWrite(ledProteccion, HIGH);
    digitalWrite(ledManual, HIGH);
    digitalWrite(ledAutomatico, HIGH);
                                        //Por seguridad, la única manera de salir del bucle es reiniciando el microcontrolador.
    lcd.setCursor(0,0);
    lcd.print("Parada Emergecia         ");
    lcd.setCursor(0,1);
    lcd.print("   REINICIAR             ");
  }
}
