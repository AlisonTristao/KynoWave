#ifndef KYNOWAVE_H
#define KYNOWAVE_H

// external libraries
#include <Arduino.h>
#include <TinyShell.h>

// internal libraries
#include <Logger.h>

class KynoWave {
    public:
        KynoWave() {
            if (instante_ == nullptr)
                instante_ = this;
        };

        void begin();

        // members
        Logger logger;
        TinyShell shell;

        /*
        * @brief Default output callback, 
        * This function print print the log on serial, 
        * 
        * @param data Pointer to the log message data, which is expected to be a pointer to a message struct.
        * @param len Length of the data in bytes, which should be equal to the size
        */
        static bool sendToSerial(const uint8_t *data, size_t len);
        
        /*
        * @brief default input callback,
        * This function is a placeholder for handling input commands,
        * it can be implemented to read commands from serial or other input methods and pass them to the TinyShell for processing.
        */
        static bool readFromSerial(String& input);

    private:
        // static instance of the KynoWave class, this will be used to access the logger and shell from the tasks
        static KynoWave* instante_;

        // wrapper functions to be used in the shell
        static uint8_t wrapper_h();         // explain the available modules and commands
};  

#endif // KYNOWAVE_H