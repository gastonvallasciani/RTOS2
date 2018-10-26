/*
 * seniales.h
 */

#ifndef SENIALES_H_
#define SENIALES_H_

typedef enum {
	SIG_MODULO_INICIAR = 0		,
	SIG_TIMEOUT					,
	SIG_BOTON_PULSADO			,
	SIG_BOTON_LIBERADO			,
	SIG_ULTIMO_VALOR = SIG_BOTON_LIBERADO
} Signal_t;

extern Modulo_t * ModuloBroadcast;
extern Modulo_t * ModuloInformeEstado;

#endif /* SENIALES_H_ */
