/*
 * FrameworkEventos.h
 */

#ifndef FRAMEWORKEVENTOS_H_
#define FRAMEWORKEVENTOS_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "modulos.h"
#include "eventos.h"
#include "seniales.h"
#include "TimerService.h"
#include "leds.h"
#include "pulsadores.h"


//#include "CAPI_Definitions.h"

enum {
	PRIORIDAD_BAJA = 1,
	PRIORIDAD_MEDIA,
	PRIORIDAD_ALTA,
};

extern Modulo_t modulos[];
extern int ultimoModulo;

#endif /* FRAMEWORKEVENTOS_H_ */
