#ifndef DEVICE_HUB_H
#define DEVICE_HUB_H

#include <Arduino.h>

#ifndef DEVICE_HUB_MAX_OUTPUTS
#define DEVICE_HUB_MAX_OUTPUTS 4
#endif

#ifndef DEVICE_HUB_MAX_INPUTS
#define DEVICE_HUB_MAX_INPUTS 4
#endif

class IOutputSink {
public:
    virtual ~IOutputSink() = default;

    /*
    * @brief Send a raw payload to the output device.
    *
    * @param data Pointer to payload bytes
    * @param len Payload length in bytes
    * @return true if sent successfully
    */
    virtual bool send(const uint8_t* data, size_t len) = 0;
};

class IInputSource {
public:
    virtual ~IInputSource() = default;

    /*
    * @brief Read a command string from the input device.
    *
    * @param out Output string with the command
    * @return true if a command was read
    */
    virtual bool read(String& out) = 0;
};

class DeviceHub {
public:
    DeviceHub();

    /*
    * @brief Register an output sink.
    */
    bool addOutput(IOutputSink* sink, const char* name);

    /*
    * @brief Register an input source.
    */
    bool addInput(IInputSource* source, const char* name);

    /*
    * @brief Send data to all registered outputs.
    */
    bool sendToAll(const uint8_t* data, size_t len);

    /*
    * @brief Read a command from any input source.
    */
    bool readAny(String& out, uint8_t* source_index = nullptr);

    /*
    * @brief Return the name of an input by index.
    */
    const char* inputName(uint8_t index) const;

private:
    IOutputSink* outputs_[DEVICE_HUB_MAX_OUTPUTS];
    const char* output_names_[DEVICE_HUB_MAX_OUTPUTS];
    uint8_t output_count_;

    IInputSource* inputs_[DEVICE_HUB_MAX_INPUTS];
    const char* input_names_[DEVICE_HUB_MAX_INPUTS];
    uint8_t input_count_;
};

#endif // DEVICE_HUB_H
