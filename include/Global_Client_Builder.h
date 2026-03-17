/**
 * \file    Global_Client_Builder.h
 * 
 * \brief   Este archivo contiene la definición de la clase Global_Client_Builder para la construcción y configuración de clientes de comunicación global (HTTP, WebSocket, Socket.IO).
 */

#ifndef _GLOBAL_CLIENT_BUILDER
#define _GLOBAL_CLIENT_BUILDER

#include "Ethernet_HTTP.h"
#include "Ethernet_WebSocket.h"
#include "Ethernet_SocketIO.h"

/**
 * \class Global_Client_Builder
 * 
 * \brief Clase constructora para clientes de comunicación global.
 * 
 * Permite establecer los parámetros de configuración y obtener una instancia del cliente adecuado (HTTP, WebSocket, Socket.IO) según el tipo especificado.
 */
class Global_Client_Builder
{
    public:
        /**
         * \brief Destructor.
         */
        ~Global_Client_Builder() { }

        /**
         * \brief           Establece los parámetros de configuración del cliente.
         * \param p_params  Estructura con los parámetros de configuración.
         */
        void set_client_parameters(client_params p_params)
        {
            this->m_params.m_type = p_params.m_type;
            this->m_params.m_server = p_params.m_server;
            this->m_params.m_port = p_params.m_port;
        }

        /**
         * \brief   Obtiene una instancia del cliente configurado según el tipo.
         * \returns Puntero al objeto Global_Client correspondiente.
         */
        Global_Client *get_client()
        {
            this->m_clients[this->m_params.m_type]->set_client_parameters(this->m_params);
            return this->m_clients[this->m_params.m_type];
        }

    private:
        client_params m_params;                     ///< Parámetros de configuración del cliente.
        Ethernet_HTTP m_ethernet_http;              ///< Instancia del cliente HTTP.
        Ethernet_WebSocket m_ethernet_websocket;    ///< Instancia del cliente WebSocket.
        Ethernet_SocketIO m_ethernet_socketio;      ///< Instancia del cliente Socket.IO.

        /**
         * \brief Arreglo de punteros a los clientes disponibles, indexados por tipo.
         *        0: HTTP, 1: WebSocket, 2: Socket.IO
         */
        Global_Client *m_clients[3]
        {
            &m_ethernet_http, &m_ethernet_websocket, &m_ethernet_socketio
        };
};

#endif