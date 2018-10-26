/*
 * modulos.h
 *
 *  Created on: 11/05/2012
 *      Author: alejandro
 */

#ifndef MODULOS_H_
#define MODULOS_H_

#include "FreeRTOS.h"
#include "queue.h"

typedef struct Modulo_t Modulo_t;
typedef struct Evento_t Evento_t;

typedef void (*fsm_ptr) (Evento_t *);

struct Modulo_t {
	fsm_ptr	manejadorEventos;
	int 	periodo;
	int		timeoutTick;
	int		timeoutRepetitivo;
	int		prioridad;
	int		medicionesTomadas;
	int		medicionesObjetivo;
	int 	acumulado;
};

struct Evento_t {
	Modulo_t *	receptor;
	int			signal;
	int			valor;
};

Modulo_t * 	RegistrarModulo ( fsm_ptr manejadorEventos, int prioridad);
void DespacharEvento	( Evento_t * evn );
void IniciarTodosLosModulos ( void );

extern xQueueHandle colaEventosPrioridadBaja;
extern xQueueHandle colaEventosPrioridadMedia;
extern xQueueHandle colaEventosPrioridadAlta;

#endif /* MODULOS_H_ */
