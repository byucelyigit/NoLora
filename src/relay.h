class Relay {
    public:
        enum Status {
            RELAY_ON = 1,
            RELAY_OFF = 0
        };
        Status status;        
        void (*stateChangeCallback)(int, bool, int) = nullptr;

    Relay(int _relayno, int _pinno);
    void TurnOn(int reason = 0);
    void TurnOff(int reason = 0);
    void SetStateChangeCallback(void (*callback)(int, bool, int));

    private:
        int relayNo;
        int pin;
        int oldStatus = RELAY_OFF;
    };