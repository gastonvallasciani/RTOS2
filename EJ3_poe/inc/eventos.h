/*
 * evento.h
 */

#ifndef EVENTOS_H_
#define EVENTOS_H_

#include "FrameworkEventos.h"
#include "modulos.h"
#include "seniales.h"
#include "broadcast.h"
#include "TimerService.h"

Evento_t	EsperarEvento (xQueueHandle colaEventos);
void 		TareaDespachadoraEventos ( void * colaEventos );
void EncolarEvento (Modulo_t * receptor, Signal_t senial, int valor);


#endif /* EVENTO_H_ */
