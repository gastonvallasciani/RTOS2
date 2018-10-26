/*
 * eventos.c
 */

#include "FrameworkEventos.h"

static xQueueHandle getColaEventos (int prioridad);
static fsm_ptr 		getManejadorEventos (Evento_t * evento);

void TareaDespachadoraEventos (void * paramColaEventos){
	/* Manejo de eventos de baja prioridad
	 * Esta siempre bloqueada salvo que tenga un evento que manejar.
	 * En cuanto hay un evento, lo despacho a la FSM correspondiente SIN BLOQUEAR */
	xQueueHandle colaEventos = (xQueueHandle) paramColaEventos;
	Evento_t evn;
	for(;;){
		evn = EsperarEvento(colaEventos);
		DespacharEvento(&evn);
	}
}

Evento_t EsperarEvento (xQueueHandle colaEventos)
{
	Evento_t evn;
	xQueueReceive(colaEventos, &evn, portMAX_DELAY);
	return evn;
}

void EncolarEvento (Modulo_t * receptor, Signal_t senial, int valor)
{
	Evento_t evn;

	xQueueHandle colaEventos = getColaEventos(receptor->prioridad);
	evn.receptor	= receptor;
	evn.signal		= senial;
	evn.valor 		= valor;

	xQueueSend(colaEventos, &evn, 0);
	return;
}

void ReenviarEvento (Modulo_t * modulo, Evento_t * evn)
{
	EncolarEvento(modulo, evn->signal, evn->valor);
}

portBASE_TYPE EncolarEventoFromISR (Modulo_t * receptor, Signal_t senial, int valor)
{
	Evento_t evn;
	portBASE_TYPE cambiarCtx = pdFALSE;

	xQueueHandle colaEventos = getColaEventos(receptor->prioridad);
	evn.receptor	= receptor;
	evn.signal 		= senial;
	evn.valor 		= valor;

	xQueueSendFromISR(colaEventos, &evn, &cambiarCtx);
	return cambiarCtx;
}

static xQueueHandle getColaEventos(int prioridad)
{
	xQueueHandle colaEventos;
	switch (prioridad)
	{
		case PRIORIDAD_BAJA:	colaEventos = colaEventosPrioridadBaja;		break;
		case PRIORIDAD_MEDIA:	colaEventos = colaEventosPrioridadMedia;	break;
		case PRIORIDAD_ALTA:	colaEventos = colaEventosPrioridadAlta;		break;
		default:
			break;
	}
	return colaEventos;
}

void DespacharEvento(Evento_t *evento)
{
	fsm_ptr manejadorEventos = getManejadorEventos(evento);
	manejadorEventos(evento); //Al receptor del evento le paso el evento
}

static fsm_ptr getManejadorEventos (Evento_t * evento)
{
	return (evento->receptor)->manejadorEventos;
}
