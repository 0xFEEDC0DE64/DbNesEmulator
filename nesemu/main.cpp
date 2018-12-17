#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
#include <QImage>
#include <QFile>
#include <QTimer>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QAudioOutput>

// dbcorelib includes
#include "waverecorder.h"
#include "fifostream.h"
#include "utils/datastreamutils.h"

// dbguilib includes
#include "canvaswidget.h"

// nescorelib includes
#include "nesemulator.h"
#include "emusettings.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    NesEmulator emulator;

    const QString path = app.arguments().count() > 1 ? app.arguments().at(1) : QFileDialog::getOpenFileName(nullptr, "Select ROM file...", QString(), "ROM file (*.nes)");

    if(path.isEmpty())
        return 0;

    Rom rom;

    try {
        rom = Rom::fromFile(path);
        emulator.load(rom);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, "Error while loading rom!", QString::fromStdString(e.what()));
        return 1;
    }

    // Audio recorder
    WaveRecorder recorder(1, emulator.apu().sampleRate(), "sound.wav");
    QObject::connect(&emulator.apu(), &Apu::samplesFinished, &recorder, &WaveRecorder::addSamples);

    // Live audio playback
    FifoStream stream;
    QObject::connect(&emulator.apu(), &Apu::samplesFinished, [&stream](const QVector<qint32> &samples){
        QByteArray buf;
        buf.reserve(samples.size() * sizeof(qint32));

        QDataStream dataStream(&buf, QIODevice::WriteOnly);
        dataStream.setByteOrder(QDataStream::BigEndian);

        for(auto sample : samples)
            dataStream << sample;

        Q_ASSERT(buf.size() == samples.size() * sizeof(qint32));

        stream.write(buf);
    });

    // Display
    CanvasWidget canvas;
    canvas.setWindowTitle(QString("%0 - Mapper: %1").arg(QFileInfo(path).fileName()).arg(rom.mapperNumber));
    canvas.show();
    quint64 frameCounter {};
    QObject::connect(&emulator.ppu(), &Ppu::frameFinished, [&canvas, &frameCounter](const std::array<qint32,Ppu::SCREEN_WIDTH*Ppu::SCREEN_HEIGHT> &frame){
        canvas.setImage(QImage(reinterpret_cast<const uchar*>(&frame[0]), Ppu::SCREEN_WIDTH, Ppu::SCREEN_HEIGHT, QImage::Format_RGB32));

        QFile file(QString("frames/%0.bmp").arg(frameCounter++));
        if(!file.open(QIODevice::WriteOnly))
            return;

        constexpr quint32 bitmapSize = Ppu::SCREEN_WIDTH * Ppu::SCREEN_HEIGHT * sizeof(quint32);

        QDataStream dataStream(&file);
        dataStream.setByteOrder(QDataStream::LittleEndian);

        //BMP Header
        dataStream << quint16(0x4D42);            //BM-Header
        dataStream << quint32(54 + bitmapSize);   //File size
        dataStream << quint32(0);                 //Unused
        dataStream << quint32(54);                //Offset to bitmap data

        //DIB Header
        dataStream << quint32(40);                //DIP Header size
        dataStream << Ppu::SCREEN_WIDTH;          //width
        dataStream << Ppu::SCREEN_HEIGHT;         //height
        dataStream << quint16(1);                 //Number of color planes
        dataStream << quint16(32);                //Bits per pixel
        dataStream << quint32(0);                 //No compression;
        dataStream << bitmapSize;                 //Size of bitmap data
        dataStream << quint32(2835);              //Horizontal print resolution
        dataStream << quint32(2835);              //Horizontal print resolution
        dataStream << quint32(0);                 //Number of colors in palette
        dataStream << quint32(0);                 //Important colors

        dataStream << frame;
    });

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &emulator, &NesEmulator::emuClockFrame);
    timer.setInterval(EmuSettings::emuTimeFramePeriod);

    // HACK: overclocking a little bit when audio device reaches near end
    QObject::connect(&emulator.ppu(), &Ppu::frameFinished, [&stream, &timer](){
        int speed = EmuSettings::emuTimeFramePeriod;
        if(stream.bytesAvailable() / sizeof(qint32) < 4096)
            speed = double(speed) * 0.9;

        if(timer.interval() != speed)
            timer.setInterval(speed);
    });

    QAudioFormat format;
    format.setSampleRate(emulator.apu().sampleRate());
    format.setChannelCount(1);
    format.setSampleSize(sizeof(qint32) * 8);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    {
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(format))
        {
           qFatal("Your soundcard does not support the needed output format!");
           return -1;
        }
    }

    QAudioOutput output(format);
    output.start(&stream);
    timer.start();

    return app.exec();
}
