#include <DeviceHub.h>

DeviceHub::DeviceHub()
    : output_count_(0),
      input_count_(0) {
    // initialize output slots
    for (uint8_t i = 0; i < DEVICE_HUB_MAX_OUTPUTS; ++i) {
        outputs_[i] = nullptr;
        output_names_[i] = nullptr;
    }
    // initialize input slots
    for (uint8_t i = 0; i < DEVICE_HUB_MAX_INPUTS; ++i) {
        inputs_[i] = nullptr;
        input_names_[i] = nullptr;
    }
}

bool DeviceHub::addOutput(IOutputSink* sink, const char* name) {
    if (sink == nullptr || output_count_ >= DEVICE_HUB_MAX_OUTPUTS)
        return false;

    outputs_[output_count_] = sink;
    output_names_[output_count_] = name;
    ++output_count_;
    return true;
}

bool DeviceHub::addInput(IInputSource* source, const char* name) {
    if (source == nullptr || input_count_ >= DEVICE_HUB_MAX_INPUTS)
        return false;

    inputs_[input_count_] = source;
    input_names_[input_count_] = name;
    ++input_count_;
    return true;
}

bool DeviceHub::sendToAll(const uint8_t* data, size_t len) {
    bool ok = true;
    // send the payload to all registered outputs
    for (uint8_t i = 0; i < output_count_; ++i) {
        if (outputs_[i] == nullptr)
            continue;
        if (!outputs_[i]->send(data, len))
            ok = false;
    }
    return ok;
}

bool DeviceHub::readAny(String& out, uint8_t* source_index) {
    // return the first input that has data available
    for (uint8_t i = 0; i < input_count_; ++i) {
        if (inputs_[i] == nullptr)
            continue;
        if (inputs_[i]->read(out)) {
            if (source_index)
                *source_index = i;
            return true;
        }
    }
    return false;
}

const char* DeviceHub::inputName(uint8_t index) const {
    if (index >= input_count_)
        return nullptr;
    return input_names_[index];
}
