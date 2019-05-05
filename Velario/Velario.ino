/*************************************************** 
Ultimo dia 05/04/2019
Para las monedas el valor de creditos es:
2 - 20 céntimos
3 - 50 céntimos
4 - 1 Euro
5 - 2 Euro
****************************************************/
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <TimedAction.h>

//Para testear
#define DEBUG false
//Pin de interrupción
#define PIN_MONEDERO 2
//Tiempo que permanece la vela encendida
#define TIEMPOVELAENCENDIDA 1000UL*180 //3 minutos
//Cantidad de velas encendida por tipo de moneda
#define VEINTE 1
#define CINCUENTA 3
#define EURO 6
#define DOSEUROS 12
//Características del velario
#define NUMDISPOSITIVOS 5
#define NUMCANALES 16

unsigned int NUMVELASTOTAL = 72;

volatile bool monedaInsertada = false;
volatile byte creditos = 0;
volatile unsigned long ultimoTiempocreditos = 0;

//Estructura de una vela.
typedef struct{
  unsigned short numero;
  unsigned short pwm;
  unsigned long inicio;
  unsigned long fin;
  bool activada;
}vela;

vela velas[NUMDISPOSITIVOS][NUMCANALES];
Adafruit_PWMServoDriver pwm[NUMDISPOSITIVOS];

unsigned int monedas[10];
int indice = -1;

void interrupcionMonedero(){

  creditos++;
  
  if (creditos <= 1){
    monedaInsertada = false;
    creditos = 1;
  }
  else{
    monedaInsertada = true;
  }

  ultimoTiempocreditos = millis();
  
}

void check(){

  unsigned long ultimoTiempo = millis() - ultimoTiempocreditos;

  if( monedaInsertada == true and creditos < 2){
    if(DEBUG == true){
      Serial.print("Créditos Falsos: ");
      Serial.println(creditos);
    }
    creditos = 0;
    monedaInsertada = false;
  }
  
  if( monedaInsertada == true and ultimoTiempo > 200 and creditos >= 2){
    indice++;
    
    monedas[indice] = creditos;
    if(DEBUG == true){
      Serial.print("Créditos: ");
      Serial.println(creditos);
    }

    if(DEBUG == true){

      Serial.print("Insertados: ");
      switch(creditos){
        case 3:
          Serial.println("20 centimos");
          break;
        case 5:
          Serial.println("50 centimos");
          break;
        case 7:
          Serial.println("1 Euro");
          break;
        case 9:
          Serial.println("2 Euros");
          break;
        default:
          creditos = 0;
        break;
      }
    }  

    creditos=0;
  }

}

//Para pedir cada 200 ms la función check
TimedAction timedAction = TimedAction(200, check); 

void setup() {

  if(DEBUG == true)
    Serial.begin(9600);

    for (byte i = 0 ; i < NUMDISPOSITIVOS ; i++){
      pwm[i] = Adafruit_PWMServoDriver((0x40)+i);
      pwm[i].begin();
      pwm[i].setPWMFreq(100);
  }

  Wire.setClock(400000);
 
  //Inicializar matriz de velas
  for (unsigned int i = 0 ; i < NUMDISPOSITIVOS ; i++){
    for (unsigned int j = 0 ; j < NUMCANALES ; j++){
      velas[i][j].numero = j;
      velas[i][j].pwm = 0;
      velas[i][j].inicio = 0;
      velas[i][j].fin = 0;
      velas[i][j].activada = false;
      pwm[i].setPWM(velas[i][j].numero, 0, 0 );
    }
  }

  attachInterrupt(digitalPinToInterrupt(PIN_MONEDERO), interrupcionMonedero, RISING);
  creditos = 0;

  //Dejar la primera vela encendida.
  velas[0][0].activada = true;
  pwm[0].setPWM(velas[0][0].numero, 0, 4016);
  
}

void loop() {
  //Chequear cuanto tiempo llevan las velas encendidas.

  timedAction.check();

  while(indice > -1){
    if(DEBUG == true){
      Serial.println("Antes");
      for (int x = 0 ; x <= indice; x++){
          Serial.print("Monedas[");
          Serial.print(x);
          Serial.print("]= ");
          Serial.println(monedas[x]);
      }
      Serial.println("----------");
      
      Serial.print("Índice loop: ");
      Serial.println(indice);
    }
    
    activarVelas(monedas[0]);
    for (int i=0; i<= indice; i++){
        monedas[i] = monedas[i+1];
    }

    if(DEBUG == true){
      Serial.println("Después");
      for (int x = 0 ; x < indice; x++){
          Serial.print("Monedas[");
          Serial.print(x);
          Serial.print("]= ");
          Serial.println(monedas[x]);
      }
     Serial.println();
    }
   indice--;
   comprobarTiempoParaApagar( millis() ); 
  }

   comprobarTiempoParaApagar( millis() ); 

}


// FUNCION PARA ACTIVAR LAS VELAS SEGÚN LA MONEDA QUE RECIBA
void activarVelas(byte moneda){
  if(NUMVELASTOTAL >= 0 and NUMVELASTOTAL <= 72){ //Comprobar que el numero de velas que quedan está en su rango.
    int velasEncendidas=0;
    
    //Moneda de 2 Euros
    if(moneda >= 9){
      velasEncendidas = DOSEUROS;
      for (unsigned int i = 0 ; i < NUMDISPOSITIVOS ; i++){
        for (unsigned int j = 0; j < NUMCANALES ; j++){
          timedAction.check();
          if(velas[i][j].activada == false and velasEncendidas > 0){
            if( (i == 0) and (j == 0) ){
              velas[i][j].activada = true;
            }
            else{
              velas[i][j].activada = true;
              velas[i][j].inicio = millis();
              velas[i][j].fin = velas[i][j].inicio + TIEMPOVELAENCENDIDA;
              velasEncendidas--;
              if (NUMVELASTOTAL == 0){
                NUMVELASTOTAL = 0;
              }else{
                NUMVELASTOTAL--;
              }
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              if(velasEncendidas == false){
                break;
              }
            }
          }
        }
      }
    }
    
    //Moneda de 1 Euro
    else if(moneda >= 7 and moneda < 9){
      velasEncendidas = EURO;
      for (unsigned int i = 0 ; i < NUMDISPOSITIVOS ; i++){
        for (unsigned int j = 0; j < NUMCANALES ; j++){
          timedAction.check();
          if(velas[i][j].activada == false and velasEncendidas > 0){
            if( (i == 0) and (j == 0) ){
              velas[i][j].activada = true;
            }
            else{
              velas[i][j].activada = true;
              velas[i][j].inicio = millis();
              velas[i][j].fin = velas[i][j].inicio + TIEMPOVELAENCENDIDA;
              velasEncendidas--;
              if (NUMVELASTOTAL == 0){
                NUMVELASTOTAL = 0;
              }else{
                NUMVELASTOTAL--;
              }
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              if(velasEncendidas == 0)
                break;         
            }
          }
        }
      }
    }
    
    //Moneda de 50 centimos
    else if(moneda >= 5 and moneda < 7){
      velasEncendidas = CINCUENTA;
      for (unsigned int i = 0 ; i < NUMDISPOSITIVOS ; i++){
        for (unsigned int j = 0; j < NUMCANALES ; j++){
          timedAction.check();            
          if(velas[i][j].activada == false and velasEncendidas > 0 ){
            if( (i == 0) and (j == 0) ){
              velas[i][j].activada = true;
            }
            else{  
              velas[i][j].activada = true;
              velas[i][j].inicio = millis();
              velas[i][j].fin = velas[i][j].inicio + TIEMPOVELAENCENDIDA;
              velasEncendidas--;
              if (NUMVELASTOTAL == 0){
                NUMVELASTOTAL = 0;
              }else{
                NUMVELASTOTAL--;
              }
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              if(velasEncendidas == 0)
                break;     
            }
          }
        }
      }
    }
    //Moneda de 20 centimos
    else if(moneda >= 3 and moneda < 5){
      velasEncendidas = VEINTE;
      for (unsigned int i = 0 ; i< NUMDISPOSITIVOS ; i++){
        for (unsigned int j = 0; j < NUMCANALES ; j++){
          timedAction.check();            
          if(velas[i][j].activada == false and velasEncendidas > 0){
            if( (i == 0) and (j == 0) ){
              velas[i][j].activada = true;
            }
            else{
              velas[i][j].activada = true;
              velas[i][j].inicio = millis();
              velas[i][j].fin = velas[i][j].inicio + TIEMPOVELAENCENDIDA;
              velasEncendidas--;
              if (NUMVELASTOTAL == 0){
                NUMVELASTOTAL = 0;
              }else{
                NUMVELASTOTAL--;
              }
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 0);
              delay(50);
              pwm[i].setPWM(velas[i][j].numero, 0, 4016);
              if(velasEncendidas == 0)
                break; 
            }
          }
        }
      }
    }
    else{
      moneda = 0;
      creditos = 0;  
    }
  }//Fin de comprobar velas totales      
}//Fin activar moneda


//Comprueba el tiempo que le quedan a las velas para apagarse.
void comprobarTiempoParaApagar(unsigned long tiempo){
  for (unsigned int i = 0; i < NUMDISPOSITIVOS; i++){
    for (unsigned int j = 0; j < NUMCANALES; j++){
      if (velas[i][j].activada == true and (tiempo > velas[i][j].fin)){
        if( (i == 0) and (j == 0) ){//Dejar esta vela fija siempre.
          velas[i][j].activada = true;
        }
        else{
          velas[i][j].activada = false;
          velas[i][j].inicio = 0;
          velas[i][j].fin = 0;
          velas[i][j].pwm = 0;
          pwm[i].setPWM(velas[i][j].numero, 0, 0);
          NUMVELASTOTAL++;
          if(NUMVELASTOTAL >= 72){
            NUMVELASTOTAL = 72;
          }
        }
      }
    }
  }
}


