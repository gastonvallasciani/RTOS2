/*
 * pulsadores.c
 */

#include "leds.h"
#include "pulsadores.h"
#include "seniales.h"
#include "FrameworkEventos.h"
#include "sapi.h"

extern Modulo_t * ModuloDriverLeds;
static Modulo_t * mod;

tecla_antirrebote_t tecla1;
tecla_antirrebote_t tecla2;
tecla_antirrebote_t tecla3;
tecla_antirrebote_t tecla4;

void DriverPulsadoresInitMEF (tecla_antirrebote_t * tecla, gpioMap_t gpio, uint8_t teclaN ) {

	tecla->teclaN = teclaN;
	tecla->estado = BUTTON_UP;
	tecla->teclaPin = gpio;
	return;
}

void DriverPulsadores ( Evento_t *evn )
{
	switch( evn->signal )
	{
		case SIG_MODULO_INICIAR:
			mod = (Modulo_t *)evn->valor;
			mod->periodo = 20;
			TimerArmRepetitivo(mod, mod->periodo);
	      DriverPulsadoresInitMEF(&tecla1, TEC1, 1);
	      DriverPulsadoresInitMEF(&tecla2, TEC2, 2);
	      DriverPulsadoresInitMEF(&tecla3, TEC3, 3);
	      DriverPulsadoresInitMEF(&tecla4, TEC4, 4);
			break;

		case SIG_TIMEOUT:
	      AntirreboteMEF(&tecla1);
	      AntirreboteMEF(&tecla2);
	      AntirreboteMEF(&tecla3);
	      AntirreboteMEF(&tecla4);
			break;

		default:	// Ignoro todas las otras seniales
			break;
	}
}

void AntirreboteMEF (tecla_antirrebote_t * tecla) {

	bool_t tecValue = HIGH;

	switch(tecla->estado)
	{

	case BUTTON_UP:
		// evaluo si el dato para sacar corresponde a esta instancia de la MEF
		if (gpioRead(tecla->teclaPin) == LOW) {
			tecla->estado = BUTTON_FALLING;
		}
		break;

	case BUTTON_FALLING:
		/* Si el tiempo del delay expiró, paso a leer el estado del pin */
		tecValue = gpioRead( tecla->teclaPin );
		if(tecValue == HIGH) {
			tecla->estado = BUTTON_UP;
		}
		else if(tecValue == LOW) {
			tecla->estado = BUTTON_DOWN;
			EncolarEvento(ModuloDriverLeds, SIG_BOTON_PULSADO, tecla->teclaN);
		}
		break;

	case BUTTON_DOWN:
		// evaluo si el dato para sacar corresponde a esta instancia de la MEF
		if (gpioRead(tecla->teclaPin) == HIGH) {
			tecla->estado = BUTTON_RISING;
		}
		break;

	case BUTTON_RISING:
		/* Si el tiempo del delay expiró, paso a leer el estado del pin */
		tecValue = gpioRead( tecla->teclaPin );
		if(tecValue == LOW) {
			tecla->estado = BUTTON_DOWN;
		}
		else if(tecValue == HIGH) {
			tecla->estado = BUTTON_UP;
			EncolarEvento(ModuloDriverLeds, SIG_BOTON_LIBERADO, tecla->teclaN);
		}
		break;

	default:
		DriverPulsadoresInitMEF(tecla, tecla->teclaPin, tecla->teclaN);

	}
	return;
}
