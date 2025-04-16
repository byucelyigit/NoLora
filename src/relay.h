class Relay {
    public:
        enum Status {
            RELAY_ON = 1,
            RELAY_OFF = 0
        };
        Status status;        
        void (*stateChangeCallback)(int, bool) = nullptr;

    Relay(int _relayno, int _pinno);
    void TurnOn();
    void TurnOff();
    void SetStateChangeCallback(void (*callback)(int, bool));

    private:
        int relayNo;
        int pin;
        int oldStatus = RELAY_OFF;
    };