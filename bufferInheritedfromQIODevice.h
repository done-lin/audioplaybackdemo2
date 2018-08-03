#ifndef BUFFERINHERITEDFROMQIODEVICE_H
#define BUFFERINHERITEDFROMQIODEVICE_H

#include <QAudioInput>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QObject>
#include <QAudioOutput>
#include <iostream>
#include <QByteArray>
#include <QBuffer>
#include <QCoreApplication>

class Generator: public QIODevice
{
    Q_OBJECT
public:
    Generator(qint64 bufferSize);
    ~Generator();
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen); // maybe this readData would affect the Mic to Buffer?
    QByteArray m_GeneratorBuffer;
    void start();
    void stop();


// by affecting the behavior of m_audioInputIODevice

    qint64 bytesAvailable() const;

private:
    // void generateData(const QAudioFormat &format, qint64 durationUs, int frequency);

private:
    qint64 m_pos;
public slots:
    void slot_receive_data_from_microphone(QByteArray dataArray);
};

class bufferPlayback: public QObject
{
    Q_OBJECT
public:
    ~bufferPlayback();
    bufferPlayback();
    qint64 bufferLength() const;

public slots:
    void startRecording();
    void captureDataFromDevice();
    void startPlayback();

signals:
    void signal_finished_reading_from_microphone(QByteArray dataArray);

private:
    Generator*          m_generator;
    QAudio::Mode        m_mode;

    void selectFormat();
    void stopPlayback();
    void initialize();
    void stopRecording();

    qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);

    qint64              m_dataLengthRecord;
    qint64              m_dataLengthPlay;
    qint64              m_bufferLength;
    qint64              m_bytesReady;
    QAudioFormat        format;

    QAudioInput*        m_audioInput;
    QAudioDeviceInfo    m_audioInputDevice;
    QIODevice*          m_temp;
    QIODevice*          m_audioInputIODevice;

    QAudioDeviceInfo    m_audioOutputDevice;
    QAudioOutput*       m_audioOutput;
    qint64              m_playPosition;
    QBuffer             m_audioOutputIODevice;

    QByteArray          m_buffer;

};


#endif // BUFFERINHERITEDFROMQIODEVICE_H
