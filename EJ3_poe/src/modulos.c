/*
 * modulos.c
 */

#include "FrameworkEventos.h"
#include "sapi.h"

#define 	MAX_MODULOS 20
Modulo_t 	modulos[MAX_MODULOS];
int 		ultimoModulo = 0;

xQueueHandle colaEventosPrioridadBaja;
xQueueHandle colaEventosPrioridadMedia;
xQueueHandle colaEventosPrioridadAlta;

Modulo_t * RegistrarModulo (fsm_ptr manejadorEventos, int prioridad)
{
	if(ultimoModulo >= MAX_MODULOS)
		return 0;

	Modulo_t * pModulo 			= &modulos[ultimoModulo];
	pModulo->manejadorEventos	= manejadorEventos;
	pModulo->timeoutTick		= TIMER_DISABLED;
	pModulo->prioridad 			= prioridad;

	ultimoModulo++;
	return pModulo++;
}

void IniciarTodosLosModulos( void )
{
    int modulo;
    Evento_t evn;
    for (modulo = 0; modulo < ultimoModulo; ++modulo) {
    	//modulos[modulo].estado = 0;
    	evn.signal 		= SIG_MODULO_INICIAR;
    	evn.receptor 	= &modulos[modulo];
    	evn.valor 		= (int)evn.receptor;
    	DespacharEvento(&evn);
    }
}

