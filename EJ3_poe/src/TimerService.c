/*
 * TimerService.c
 *
 *  Created on: 11/05/2012
 *      Author: alejandro
 */

#include "FrameworkEventos.h"

void 		vApplicationTickHook 		( void )
{
	int modulo;
	Modulo_t * pModulo;
	portBASE_TYPE cambiarCtx = pdFALSE;

	for (modulo = 0; modulo < ultimoModulo; ++modulo)
	{
		pModulo = &modulos[modulo];
		if(pModulo->timeoutTick != TIMER_DISABLED)
		{
			if(--pModulo->timeoutTick == 0)
			{
				cambiarCtx = EncolarEventoFromISR(pModulo, SIG_TIMEOUT, 0);
				if(pModulo->timeoutRepetitivo)
				{
					TimerArmRepetitivo(pModulo, pModulo->periodo);
				}
				else
				{
					TimerDisarm(pModulo);
				}
			}
		}
	}
	portEND_SWITCHING_ISR(cambiarCtx);
}

void 		TimerArmUnico			( Modulo_t * pModulo, unsigned int timeout ){
	pModulo->timeoutTick 		= timeout;
	pModulo->timeoutRepetitivo 	= 0;
	return;
}

void 		TimerArmRepetitivo		( Modulo_t * pModulo, unsigned int timeout ){
	pModulo->periodo 			= timeout;
	pModulo->timeoutTick 		= timeout;
	pModulo->timeoutRepetitivo 	= 1;
	return;
}


void 		TimerDisarm				( Modulo_t * pModulo ){
	pModulo->timeoutTick 		= TIMER_DISABLED;
	pModulo->timeoutRepetitivo 	= 0;
	return;
}
