/*
 * leds.c
 *
 *  Created on: 29/11/2011
 *      Author: team
 */

#include "FrameworkEventos.h"
#include "sapi.h"
#include "leds.h"

typedef enum estadosBroadcastEnum {
	sBROADCAST_IDLE		 = 0,
	sBROADCAST_NORMAL
} EstadosBroadcast_t;

static int estadoBroadcast = sBROADCAST_IDLE;

void DriverLeds (Evento_t * evn){

	switch(estadoBroadcast){

	case sBROADCAST_IDLE:

		switch(evn->signal) {

		case SIG_MODULO_INICIAR:
			estadoBroadcast = sBROADCAST_NORMAL;
			break;
		default:
			break;
		}

		break;

	case sBROADCAST_NORMAL:

		switch(evn->signal) {

		case SIG_BOTON_PULSADO:

			switch(evn->valor) {
			case 1:
				gpioWrite(LEDR, ON);
				break;
			case 2:
				gpioWrite(LED1, ON);
				break;
			case 3:
				gpioWrite(LED2, ON);
				break;
			case 4:
				gpioWrite(LED3, ON);
				break;
			default:
				break;
			}
			break;

		case SIG_BOTON_LIBERADO:
			switch(evn->valor) {
			case 1:
				gpioWrite(LEDR, OFF);
				break;
			case 2:
				gpioWrite(LED1, OFF);
				break;
			case 3:
				gpioWrite(LED2, OFF);
				break;
			case 4:
				gpioWrite(LED3, OFF);
				break;
			default:
				break;
			}
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}
