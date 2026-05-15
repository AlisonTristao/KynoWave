#ifndef SHARED_MESSAGE_TYPES_H
#define SHARED_MESSAGE_TYPES_H

#include <stdint.h>

// tamanhos e limites para as mensagens do logger, garantindo que as mensagens sejam gerenciáveis e não excedam os limites de memória
// tamanho maximo espnow = 250
// tamanho gasto em protocolo: 4bytes para timestamp + 1 byte para tipo + 1 byte para numero do pacote + 1 byte para total de pacotes + 1 byte para checksum = 8 bytes
// portanto, o tamanho maximo da mensagem é 250 - 8 = 242 bytes
#define MAX_PACKET_MESSAGE_SIZE 250 // se mudarmos o protocolo de envio, podemos aumentar esse valor
#define PROTOCOL_OVERHEAD_SIZE  8
#define MAX_MESSAGE_SIZE        (MAX_PACKET_MESSAGE_SIZE - PROTOCOL_OVERHEAD_SIZE) 
#define MAX_MESSAGES            256 // limite de mensagens para adicionar na memoria - cuidar com o limtie de ram disponivel
#define LOGGER_MUTEX_TIMEOUT_MS 100 // tempo em ms para esperar o logger ficar disponivel - usando para nao apagarmos as mensagens durante o print

// enum para definir os tipos de log, categorizando as mensagens enviadas
enum class logType : uint8_t {
    NONE = 0,
    INFO,
    CMD,
    TELEMETRY,
    ERROR,
    DEBUG,
    PAKG,
};

// struct para armazenar as mensagens do logger, incluindo o timestamp, tipo e conteúdo da mensagem
// packet number e total de pacotes são usados para mensagens que precisam ser fragmentadas em múltiplos pacotes devido ao limite de tamanho do protocolo
typedef struct {
    // overhead da mensagem para controle e verificação de integridade
    uint32_t    timer;          // timestamp em ms para indicar quando a mensagem foi criada
    logType     type;           // tipo da mensagem, usando o enum logType para categorizar a mensagem
    uint8_t     packet_number;  // número do pacote para mensagens fragmentadas
    uint8_t     total_packets;  // total de pacotes para mensagens fragmentadas
    uint8_t     checksum;       // checksum simples para verificar a integridade da mensagem

    // conteudo real da mensagem, limitado a MAX_MESSAGE_SIZE para garantir que o pacote completo não exceda o limite do protocolo
    char        msg[MAX_MESSAGE_SIZE + 1];
} message;

inline constexpr const char* logTypeToString(logType type) {
    switch (type) {
        case logType::INFO:         return "INFO";
        case logType::CMD:          return "CMDO";
        case logType::TELEMETRY:    return "TELE";
        case logType::ERROR:        return "ERRO";
        case logType::DEBUG:        return "DEBG";
        case logType::PAKG:         return "PAKG";
        case logType::NONE:         return "NONE";
        default:                    return "UNKN"; // tipo desconhecido - cuidado, se a mensagem ser UNK, nunca será printada
    }
};

#endif // SHARED_MESSAGE_TYPES_H