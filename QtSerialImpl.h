#ifdef QT_PLATFORM
#include <QSerialPort>

class QtSerialImpl : public ISerialInterface {
public:
    QtSerialImpl(const QString& portName, int baudRate);
    virtual bool open() override;
    virtual void close() override;
    virtual size_t write(const uint8_t* data, size_t length) override;
    virtual size_t read(uint8_t* buffer, size_t length) override;
    virtual bool isOpen() override;
    virtual void flush() override;

private:
    QSerialPort serialPort_;
};
#endif