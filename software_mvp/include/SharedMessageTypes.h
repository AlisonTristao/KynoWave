#ifndef SHARED_MESSAGE_TYPES_H
#define SHARED_MESSAGE_TYPES_H

#include <stdint.h>

// sizes and limits for logger messages, ensuring they are manageable and do not exceed memory limits
// max espnow size = 250
// protocol overhead: 4 bytes for timestamp + 1 byte for type + 1 byte for packet number + 1 byte for total packets + 1 byte for checksum = 8 bytes
// therefore, max message size is 250 - 8 = 242 bytes
#define MAX_PACKET_MESSAGE_SIZE 250 // if we change the transport protocol, we can increase this value
#define PROTOCOL_OVERHEAD_SIZE  8
#define MAX_MESSAGE_SIZE        (MAX_PACKET_MESSAGE_SIZE - PROTOCOL_OVERHEAD_SIZE) 
#define MAX_MESSAGES            256 // limit for messages in memory - watch out for available ram limits
#define LOGGER_MUTEX_TIMEOUT_MS 100 // time in ms to wait for the logger to be available - used to avoid deleting messages during printing

// enum to define log types, categorizing sent messages
enum class logType : uint8_t {
    NONE = 0,       // default type, should not be used for actual log messages
    INFO,           // informational messages that indicate normal operation and important events
    CMDO,           // terminal commands received
    TELE,           // telemetry messages
    ERRO,           // error messages
    DEBG,           // debug messages
    PAKG,           // package messages
};

// struct to store logger messages, including timestamp, type, and content
// packet number and total packets are used for messages that need fragmentation due to protocol size limits
typedef struct {
    // message overhead for control and integrity verification
    uint32_t    timer;          // timestamp in ms to indicate when the message was created
    logType     type;           // message type, using the logtype enum to categorize the message
    uint8_t     packet_number;  // packet number for fragmented messages
    uint8_t     total_packets;  // total packets for fragmented messages
    uint8_t     checksum;       // simple checksum to verify message integrity

    // actual message content, limited to max_message_size to ensure the full packet stays within limits
    char        msg[MAX_MESSAGE_SIZE + 1];
} message;

inline constexpr const char* logTypeToString(logType type) {
    switch (type) {
        case logType::INFO:         return "INFO";
        case logType::CMDO:         return "CMDO";
        case logType::TELE:         return "TELE";
        case logType::ERRO:         return "ERRO";
        case logType::DEBG:         return "DEBG";
        case logType::PAKG:         return "PAKG";
        case logType::NONE:         return "NONE";
        // capotamo o corsa
        default:                    return "UNKN"; 
    }
};

#endif // SHARED_MESSAGE_TYPES_H