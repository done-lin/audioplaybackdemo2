#include "bufferInheritedfromQIODevice.h"

const qint64 BufferDurationUs       = 10 * 1000000;  // 10second

Generator::Generator(qint64 bufferSize)
{
    m_GeneratorBuffer.resize(bufferSize);
    m_GeneratorBuffer.fill(0);
    m_pos=0;
}

Generator::~Generator()
{
}

void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    m_pos = 0;
    close();
}


qint64 Generator::readData(char *data, qint64 len)
{
    qDebug() << "len is " << len;
    qint64 total = 0;
    while (len - total > 0) {
        const qint64 chunk = qMin((m_GeneratorBuffer.size() - m_pos), len - total);
//        for(int cnt=0; cnt<chunk; cnt++){//add for test
//            m_GeneratorBuffer[(int)m_pos+cnt] = qrand()%256;//add for test
//        }//add for test
        memcpy(data + total, m_GeneratorBuffer.constData() + m_pos, chunk);
        m_pos = (m_pos + chunk) % m_GeneratorBuffer.size();
        total += chunk;
        qDebug() << "total is "<<total;
    }
    return total;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return m_GeneratorBuffer.size() + QIODevice::bytesAvailable();
}

void Generator::slot_receive_data_from_microphone(QByteArray dataArray)
{
    m_GeneratorBuffer = dataArray;
}

void bufferPlayback::startPlayback()
{
    if (m_audioOutput) {
        m_mode = QAudio::AudioOutput;
        m_dataLengthPlay = 0;
        m_generator->start();
        m_audioOutput->start(m_generator);
    }
}

bufferPlayback::bufferPlayback()
    :   m_mode(QAudio::AudioInput)
    ,   m_audioInput(0)
    ,   m_audioInputDevice(QAudioDeviceInfo::defaultInputDevice())
    ,   m_audioInputIODevice(0)
    ,   m_audioOutputDevice(QAudioDeviceInfo::defaultOutputDevice())
    ,   m_audioOutput(0)
    ,   m_bufferLength(0)
    ,   m_dataLengthPlay(0)
    ,   m_dataLengthRecord(0)
{
    selectFormat();
    initialize();
    QList<QAudioDeviceInfo> inputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    m_audioInputDevice = inputDevices.at(0);
    QList<QAudioDeviceInfo> outputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    m_audioOutputDevice = outputDevices.at(0);
    QTimer::singleShot(3000, reinterpret_cast<QObject*>(this), SLOT(startPlayback()));
    startRecording();
    qDebug() << "Device name: " << m_audioInputDevice.deviceName();
    qDebug() << "Device name: " << m_audioOutputDevice.deviceName();
    connect(this, SIGNAL(signal_finished_reading_from_microphone(QByteArray)), m_generator, SLOT(slot_receive_data_from_microphone(QByteArray)));

}

bufferPlayback::~bufferPlayback()
{
    delete m_audioInput;
    delete m_audioOutput;
    delete m_generator;
}

qint64 bufferPlayback::audioLength(const QAudioFormat &format, qint64 microSeconds)
{
    qint64 result = (format.sampleRate() * format.channelCount() * (format.sampleSize() / 8))
            * microSeconds / 1000000;
    qDebug() << "result 001: " << result;
    result -= result % (format.channelCount() * format.sampleSize());
    qDebug("sampleRate:%d, channelCount:%d, sampleSize:%d",
            format.sampleRate(), format.channelCount(), format.sampleSize());
    qDebug() << "result 002: " << result;
    return result;
}

qint64 bufferPlayback::bufferLength() const
{
    return m_bufferLength;
}

void bufferPlayback::initialize()
{
    m_bufferLength = audioLength(format, BufferDurationUs);
    m_buffer.resize(m_bufferLength);
    m_buffer.fill(0);
    m_audioInput = new QAudioInput(m_audioInputDevice, format, this);
    m_audioOutput = new QAudioOutput(m_audioOutputDevice, format, this);
    m_generator = new Generator(m_bufferLength);

}

void bufferPlayback::startRecording()
{
    if (m_audioInput) {
        m_buffer.fill(0);
        m_mode = QAudio::AudioInput;
        m_dataLengthRecord = 0;
        m_audioInputIODevice = m_audioInput->start();
        connect(m_audioInputIODevice, SIGNAL(readyRead()),
                this,SLOT(captureDataFromDevice()));
    }
}



void bufferPlayback::stopRecording()
{
    if (m_audioInput) {
        m_audioInput->stop();
        QCoreApplication::instance()->processEvents();
        m_audioInput->disconnect();
    }

    m_audioInputIODevice = 0;
}

void bufferPlayback::stopPlayback()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        QCoreApplication::instance()->processEvents();
        m_audioOutput->disconnect();
        //setPlayPosition(0);
    }
}


void bufferPlayback::selectFormat()
{
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo input_info(QAudioDeviceInfo::defaultInputDevice());
    if (!input_info.isFormatSupported(format)) {
        qWarning()<<"default format not supported try to use nearest";
        format = input_info.nearestFormat(format);
    }
    QAudioDeviceInfo output_info(QAudioDeviceInfo::defaultOutputDevice());
    if (!output_info.isFormatSupported(format)) {
        qWarning()<<"raw audio format not supported by backend, cannot play audio.";
    }
}


// push data from Mic into buffer
void bufferPlayback::captureDataFromDevice()
{
    const qint64 bytesReady = m_audioInput->bytesReady();
    m_bytesReady = bytesReady;
    //qDebug()<< "bytesReady " << bytesReady;
    const qint64 bytesSpace = m_buffer.size() - m_dataLengthRecord;  // what is m_dataLength?
    if (bytesReady > bytesSpace)
        qDebug() << "buffer is overflow";
    const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

    const qint64 bytesRead = m_audioInputIODevice->read(m_buffer.data()+m_dataLengthRecord,bytesToRead);

    qDebug() <<"bytesToRead: " << bytesToRead;

    //add some noise for testing
    for(int cnt=0; cnt<bytesToRead; cnt++){
        m_buffer[cnt+(int)m_dataLengthRecord] = qrand()%256;
    }

    if (bytesRead) {
        m_dataLengthRecord += bytesRead;
    }

    emit signal_finished_reading_from_microphone(m_buffer);

    if (m_buffer.size() == m_dataLengthRecord) {
        m_dataLengthRecord = 0;
        qDebug() << "in capture Data buffer is full";
    }
}
