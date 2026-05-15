#include <Logger.h>

Logger::Logger()
    : send_callback_(defaultSendCallback){
    mutex_ = xSemaphoreCreateMutex();
}

Logger::~Logger() {
    if (mutex_ != nullptr) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
}

void Logger::begin() {
    // verifiy if the logger is already initialized to avoid inserting duplicate logs and creating multiple mutexes
    if (initialized_) return;
    initialized_ = true;

    // add 4 logs
    // the max packet size, the max message size, the protocol overhead size and the max messages
    insert_log("Logger initialized.", logType::INFO);
    insert_log("Max packet message size: " + String(MAX_PACKET_MESSAGE_SIZE) + " bytes.", logType::INFO);
    insert_log("Max message size: " + String(MAX_MESSAGE_SIZE) + " bytes.", logType::INFO);
    insert_log("Protocol overhead size: " + String(PROTOCOL_OVERHEAD_SIZE) + " bytes.", logType::INFO);
    insert_log("Max messages in buffer: " + String(MAX_MESSAGES_IN_RAM) + ".", logType::INFO);
}

bool Logger::wait_for_mutex() {
    if (mutex_ == nullptr) return false;
    return xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE;
}

bool Logger::check_mutex() {
    if (mutex_ == nullptr) return false;
    return xSemaphoreTake(mutex_, pdMS_TO_TICKS(LOGGER_MUTEX_TIMEOUT_MS));
}

void Logger::free_mutex() {
    if (mutex_ != nullptr)
        xSemaphoreGive(mutex_);
}

void Logger::set_send_callback(SendCallback callback) {
    // wait the mutex
    if (!wait_for_mutex()) return;

    // set the callback, if the callback is null, use the default callback that does nothing
    send_callback_ = (callback != nullptr) ? callback : defaultSendCallback;
    free_mutex();
}

esp_err_t Logger::defaultSendCallback(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return ESP_FAIL;
}

void Logger::insert_log(const int16_t *sound_data, size_t len, logType type) {
    // verify the mutex is created and try to take it before inserting the log,
    if (sound_data == nullptr || len == 0) return;

    // convert the sound data to send as bytes, calculating the byte length based on the number of int16_t elements and their size
    const size_t byte_len = len * sizeof(int16_t);
    const uint8_t* data = reinterpret_cast<const uint8_t*>(sound_data);
    if (!wait_for_mutex()) return;

    // insert the log message into the buffer, using the current timestamp in milliseconds for the log entry
    insert_log_impl(data, byte_len, type, millis());
    free_mutex();
}

void Logger::insert_log(const String& msg, logType type) {
    // verify the mutex is created and try to take it before inserting the log, 
    // to ensure thread safety when multiple tasks are inserting logs concurrently
    if (!wait_for_mutex()) return;

    // convert the string to send as bytes
    const uint8_t* data = reinterpret_cast<const uint8_t*>(const_cast<char*>(msg.c_str()));

    // insert the log message into the buffer, using the current timestamp in milliseconds for the log entry
    insert_log_impl(data, msg.length(), type, millis());
    free_mutex();
}

uint8_t Logger::calculate_checksum(const message& msg) {
    uint32_t sum = 0;
    const size_t len = (msg.content.byte == nullptr) ? 0 :
        ((msg.content.size > MAX_MESSAGE_SIZE) ? MAX_MESSAGE_SIZE : msg.content.size);
    for (size_t i = 0; i < len; ++i)
        sum += static_cast<uint8_t>(msg.content.byte[i]);
    sum += static_cast<uint8_t>(msg.type);
    sum += static_cast<uint8_t>(msg.packet_number);
    sum += static_cast<uint8_t>(msg.total_packets);
    return static_cast<uint8_t>(sum % 256);
}

void Logger::insert_log_impl(const uint8_t* data, size_t len, logType type, uint32_t ts) {
    if (data == nullptr || len == 0) return;

    // calculates the number of packets needed to send the message, considering the maximum message size and protocol overhead
    const size_t msgLen = len;
    size_t total_packets_to_send = (msgLen + MAX_MESSAGE_SIZE - 1) / MAX_MESSAGE_SIZE;
    if (total_packets_to_send == 0) // should not happen since we check for empty message
        total_packets_to_send = 1;

    // loop to create and store each packet in the messages array, handling fragmentation if necessary
    for (size_t i = 0; i < total_packets_to_send; ++i) {
        // variables to get the caracteres for the current packet, 
        // calculating the start index and the number of characters to copy for this packet
        const size_t start      = i * MAX_MESSAGE_SIZE;
        const size_t remaining  = (start < msgLen) ? (msgLen - start) : 0; // remaining characters to send starting from the current packet's start index
        const size_t length     = (remaining > MAX_MESSAGE_SIZE) ? MAX_MESSAGE_SIZE : remaining;    // length of the substring for the current packet

        if (message_count >= MAX_MESSAGES_IN_RAM)
            message_count = 0;

        message& m = messages[message_count++];
        m.timer = ts;
        m.type = (i == 0U) ? type : logType::PAKG; // only the first packet gets the original type, the rest are marked as PAKG (packet)
        m.packet_number = static_cast<uint8_t>(i + 1);
        m.total_packets = static_cast<uint16_t>(total_packets_to_send);
        m.content.size = length;

        // copy the payload for the current packet into the message struct
        if (m.content.byte) {
            memset(m.content.byte, 0, MAX_MESSAGE_SIZE + 1);
            if (length > 0)
                memcpy(m.content.byte, data + start, length);
            m.content.text[length] = '\0';
        }

        // calculate checksum with the sum of the bytes of the message content, type, packet number, and total packets, modulo 256. 
        // This is a simple checksum to verify the integrity of the message on the receiving end.
        m.checksum = calculate_checksum(m);
    }
}

void Logger::reset_loop_counter() {
    if (check_mutex()) {
        if (message_count == MAX_MESSAGES_IN_RAM)
            message_count = 0;
        free_mutex();
    }
}

void Logger::send_logger_live() {
    // if the mutex is not available, return immediately. 
    // This ensures that if the logger is currently being used to insert logs, 
    // we won't interfere with that process and risk losing messages or causing delays.
    if (!check_mutex()) return;

    // get the init and the end index for the messages to send, 
    // this metod send the messagens using a circular buffer.
    uint32_t start = last_index;
    uint32_t end = message_count;

    // just verify the indexes to be in the correct range
    if (start >= MAX_MESSAGES_IN_RAM)  start = 0;
    if (end > MAX_MESSAGES_IN_RAM)     end = MAX_MESSAGES_IN_RAM;
    if (start == end)           return; // nothing to send

    // dont need to wait for the mutex here, 
    // because we are not modifying the messages array or the message count
    free_mutex();

    // send the messages from the buffer using the configured send callback, 
    // starting from the last index and going up to the current message count.
    auto send_entry = [this](uint32_t idx) {
        send_callback_(reinterpret_cast<const uint8_t*>(&messages[idx]), sizeof(messages[idx]));
    };

    // logic to send the messages in the correct order, using a circular buffer
    if (start < end) {
        for (uint32_t i = start; i < end; ++i)
            send_entry(i);
    } else {
        // Buffer wrapped: send from last_index to end of array and then from 0 to message_count.
        for (uint32_t i = start; i < MAX_MESSAGES_IN_RAM; ++i)
            send_entry(i);

        for (uint32_t i = 0; i < end; ++i)
            send_entry(i);
    }
    
    // to change the last index to the current message count, we need to take the mutex
    wait_for_mutex();
    last_index = (end == MAX_MESSAGES_IN_RAM) ? 0 : end;
    free_mutex();
}

