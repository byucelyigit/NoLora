class Relay {
    public:
        enum Status {
            RELAY_ON = 1,
            RELAY_OFF = 0
        };
        Status status;        

    Relay(int _relayno, int _pinno);
    void TurnOn();
    void TurnOff();

    private:
        int relayNo;
        int pin;
    };