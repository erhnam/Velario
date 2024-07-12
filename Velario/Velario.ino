/*************************************************** 
Ultimo dia 05/04/2019
Para las _monedas el valor de _creditos es:
2 - 20 céntimos
3 - 50 céntimos
4 - 1 Euro
5 - 2 Euro
****************************************************/
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <TimedAction.h>

//Para testear
#define DEBUG
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

#define MAX_NUM_VELAS 72

//Estructura de una vela.
typedef struct{
	unsigned short numero;
	unsigned short pwm;
	unsigned long inicio;
	unsigned long fin;
	bool activada;
} vela_t;

unsigned int NUMVELASTOTAL = 72;

volatile bool _monedaInsertada = false;
volatile byte _creditos = 0;
volatile unsigned long _ultimoTiempocreditos = 0;

static vela_t _velas[NUMDISPOSITIVOS][NUMCANALES];
static Adafruit_PWMServoDriver _pwm[NUMDISPOSITIVOS];

static unsigned int _monedas[10];
static int _indice = -1;

void interrupcionMonedero(void)
{
	// Incrementar _creditos de manera segura
    noInterrupts();
	_creditos++;
    interrupts();

	if (_creditos <= 1){
		_monedaInsertada = false;
		_creditos = 1;
	} else{
		_monedaInsertada = true;
	}

	_ultimoTiempocreditos = millis();
}

void check(void)
{
	unsigned long ultimoTiempo = millis() - _ultimoTiempocreditos;

	if (!_monedaInsertada) {
		return;
	}

	if (ultimoTiempo <= 200) {
		return;
	}

	if (_creditos < 2) {
#ifdef DEBUG
		Serial.print("Créditos Falsos: ");
		Serial.println(_creditos);
#endif
		_creditos = 0;
		// Resetear el estado de la moneda insertada
		_monedaInsertada = false;
	}
	
	// Incrementar el índice y registrar los créditos
	_indice++;

	_monedas[_indice] = _creditos;

#ifdef DEBUG
	Serial.print("Créditos: ");
	Serial.println(_creditos);

	Serial.print("Insertados: ");
	switch(_creditos){
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
			_creditos = 0;
		break;
	}
#endif

	// Resetear créditos después de procesar
    _creditos = 0;
}

//Para pedir cada 200 ms la función check
static TimedAction _timedAction = TimedAction(200, check); 

void setup(void)
{
#ifdef DEBUG
	Serial.begin(9600);
#endif

	/* Inicializar PWM */
	for (byte i = 0 ; i < NUMDISPOSITIVOS ; i++){
		_pwm[i] = Adafruit_PWMServoDriver((0x40) + i);
		_pwm[i].begin();
		_pwm[i].setPWMFreq(100);
	}

	Wire.setClock(400000);
 
	//Inicializar matriz de _velas
	for (unsigned int i = 0 ; i < NUMDISPOSITIVOS ; i++){
		for (unsigned int j = 0 ; j < NUMCANALES ; j++){
			_velas[i][j].numero = j;
			_velas[i][j].pwm = 0;
			_velas[i][j].inicio = 0;
			_velas[i][j].fin = 0;
			_velas[i][j].activada = false;
			_pwm[i].setPWM(_velas[i][j].numero, 0, 0 );
		}
	}

	attachInterrupt(digitalPinToInterrupt(PIN_MONEDERO), interrupcionMonedero, RISING);
	_creditos = 0;

	//Dejar la primera vela encendida.
	_velas[0][0].activada = true;
	_pwm[0].setPWM(_velas[0][0].numero, 0, 4016);
	
}

void loop() {
	//Chequear cuanto tiempo llevan las _velas encendidas.

	_timedAction.check();

	while (_indice > -1) {

#ifdef DEBUG
		Serial.println("Antes");
		for (int x = 0 ; x <= _indice; x++){
			Serial.print("Monedas[");
			Serial.print(x);
			Serial.print("]= ");
			Serial.println(_monedas[x]);
		}

		Serial.println("----------");
		
		Serial.print("Índice loop: ");
		Serial.println(_indice);
#endif
		
		activarVelas(_monedas[0]);

		for (int i=0; i<= _indice; i++){
				_monedas[i] = _monedas[i+1];
		}

#ifdef DEBUG
		Serial.println("Después");
		for (int x = 0 ; x < _indice; x++){
			Serial.print("Monedas[");
			Serial.print(x);
			Serial.print("]= ");
			Serial.println(_monedas[x]);
		}
		Serial.println();
#endif
		_indice--;

		comprobarTiempoParaApagar(millis()); 
	}

	comprobarTiempoParaApagar(millis());
}


void activarVelasPorTipoDeMoneda(int euro)
{
	int velasEncendidas = euro;
	
	for (unsigned int i = 0; i < NUMDISPOSITIVOS; i++) {
		for (unsigned int j = 0; j < NUMCANALES; j++) {

			_timedAction.check();

			if(_velas[i][j].activada || velasEncendidas <= 0) {
				return;
			}

			_velas[i][j].activada = true;

			if (!(i == 0 and j == 0)) {
				_velas[i][j].inicio = millis();
				_velas[i][j].fin = _velas[i][j].inicio + TIEMPOVELAENCENDIDA;
				velasEncendidas--;

				if (NUMVELASTOTAL > 0) {
					NUMVELASTOTAL--;
				}

				_pwm[i].setPWM(_velas[i][j].numero, 0, 4016);
				delay(50);
				_pwm[i].setPWM(_velas[i][j].numero, 0, 0);
				delay(50);
				_pwm[i].setPWM(_velas[i][j].numero, 0, 4016);
				delay(50);
				_pwm[i].setPWM(_velas[i][j].numero, 0, 0);
				delay(50);
				_pwm[i].setPWM(_velas[i][j].numero, 0, 4016);
				delay(50);
				_pwm[i].setPWM(_velas[i][j].numero, 0, 0);
				delay(50);
				_pwm[i].setPWM(_velas[i][j].numero, 0, 4016);

				if (velasEncendidas == false) {
					break;
				}
			}
		}
	}
}


// FUNCION PARA ACTIVAR LAS VELAS SEGÚN LA MONEDA QUE RECIBA
void activarVelas(byte moneda)
{
    if (!(NUMVELASTOTAL >= 0 && NUMVELASTOTAL <= 72)) {
        // Si el número de velas no está en el rango, salir de la función
        return;
    }

	int velasEncendidas = 0;

	//Moneda de 2 Euros
	if (moneda >= 9) {
		activarVelasPorTipoDeMoneda(DOSEUROS);
	}
	//Moneda de 1 Euro
	else if(moneda >= 7 and moneda < 9) {
		activarVelasPorTipoDeMoneda(EURO);
	}
	//Moneda de 50 centimos
	else if(moneda >= 5 and moneda < 7) {
		activarVelasPorTipoDeMoneda(CINCUENTA);
	}
	//Moneda de 20 centimos
	else if(moneda >= 3 and moneda < 5) {
		activarVelasPorTipoDeMoneda(VEINTE);
	} else {
		moneda = 0;
		_creditos = 0;  
	}
}


//Comprueba el tiempo que le quedan a las _velas para apagarse.
void comprobarTiempoParaApagar(unsigned long tiempo)
{
	for (unsigned int i = 0; i < NUMDISPOSITIVOS; i++) {
		for (unsigned int j = 0; j < NUMCANALES; j++) {
			if (_velas[i][j].activada && tiempo > _velas[i][j].fin) {
                if (i != 0 || j != 0) {
                    // Desactivar la vela
                    _velas[i][j].activada = false;
                    _velas[i][j].inicio = 0;
                    _velas[i][j].fin = 0;
                    _velas[i][j].pwm = 0;
                    _pwm[i].setPWM(_velas[i][j].numero, 0, 0);

                    // Incrementar NUMVELASTOTAL y limitar a MAX_NUM_VELAS
                    NUMVELASTOTAL++;
                    if (NUMVELASTOTAL >= MAX_NUM_VELAS) {
                        NUMVELASTOTAL = MAX_NUM_VELAS;
                    }
                }
            }
		}
	}
}
