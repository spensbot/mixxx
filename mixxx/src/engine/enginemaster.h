/***************************************************************************
                          enginemaster.h  -  description
                             -------------------
    begin                : Sun Apr 28 2002
    copyright            : (C) 2002 by
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ENGINEMASTER_H
#define ENGINEMASTER_H

#include <QMap>

#include "controlobject.h"
#include "engine/engineobject.h"
#include "engine/enginechannel.h"
#include "recording/recordingmanager.h"
#include <QtCore>

class EngineWorkerScheduler;
class EngineBuffer;
class EngineChannel;
class EngineClipping;
class EngineFlanger;
#ifdef __LADSPA__
class EngineLADSPA;
#endif
class EngineVuMeter;
class ControlPotmeter;
class ControlPushButton;
class EngineVinylSoundEmu;
class EngineSideChain;

class EngineMaster : public EngineObject {
    Q_OBJECT
public:
    EngineMaster(ConfigObject<ConfigValue>* pConfig,
                 const char* pGroup);
    virtual ~EngineMaster();

    // Get access to the sample buffers. None of these are thread safe. Only to
    // be called by SoundManager.
    const CSAMPLE* getMasterBuffer() const;
    const CSAMPLE* getHeadphoneBuffer() const;
    void pushPassthroughBuffer(int c, short *input, int len);
    int numChannels() const;
    const CSAMPLE* getChannelBuffer(unsigned int i) const;

    void process(const CSAMPLE *, const CSAMPLE *pOut, const int iBufferSize);

    // Add an EngineChannel to the mixing engine. This is not thread safe --
    // only call it before the engine has started mixing.
    void addChannel(EngineChannel* pChannel);
    static inline double gainForOrientation(EngineChannel::ChannelOrientation orientation,
                                            double leftGain,
                                            double centerGain,
                                            double rightGain) {
        switch (orientation) {
            case EngineChannel::LEFT:
                return leftGain;
            case EngineChannel::RIGHT:
                return rightGain;
            case EngineChannel::CENTER:
            default:
                return centerGain;
        }
    }


  signals:
    void bytesRecorded(int);
    void isRecording(bool);
    
  private:
    struct ChannelInfo {
        EngineChannel* m_pChannel;
        CSAMPLE* m_pBuffer;
        ControlObject* m_pVolumeControl;
    };

    class GainCalculator {
      public:
        virtual double getGain(ChannelInfo* pChannelInfo) = 0;
    };
    class ConstantGainCalculator : public GainCalculator {
      public:
        inline double getGain(ChannelInfo* pChannelInfo) {
            return m_dGain;
        }
        inline void setGain(double dGain) {
            m_dGain = dGain;
        }
      private:
        double m_dGain;
    };
    class OrientationVolumeGainCalculator : public GainCalculator {
      public:
        inline double getGain(ChannelInfo* pChannelInfo) {
            double channelVolume = pChannelInfo->m_pVolumeControl->get();
            double orientationGain = EngineMaster::gainForOrientation(
                pChannelInfo->m_pChannel->getOrientation(),
                m_dLeftGain, m_dCenterGain, m_dRightGain);
            return m_dVolume * channelVolume * orientationGain;
        }

        inline void setGains(double dVolume, double leftGain, double centerGain, double rightGain) {
            m_dVolume = dVolume;
            m_dLeftGain = leftGain;
            m_dCenterGain = centerGain;
            m_dRightGain = rightGain;
        }
      private:
        double m_dVolume, m_dLeftGain, m_dCenterGain, m_dRightGain;
    };

    void mixChannels(unsigned int channelBitvector, unsigned int maxChannels,
                     CSAMPLE* pOutput, unsigned int iBufferSize, GainCalculator* pGainCalculator);


    QList<ChannelInfo*> m_channels;

    CSAMPLE *m_pMaster, *m_pHead;
    QList<CSAMPLE*> m_channelBuffers;
    QList<CSAMPLE*> m_passthroughBuffers;
    bool m_bPassthroughWasActive[2];
    QMutex passthroughBufferMutex[2];

    EngineWorkerScheduler *m_pWorkerScheduler;

    ControlObject *m_pMasterVolume, *m_pHeadVolume;
    EngineClipping *clipping, *head_clipping;
#ifdef __LADSPA__
    EngineLADSPA *ladspa;
#endif
    EngineVuMeter *vumeter;
    EngineSideChain *sidechain;

    ControlPotmeter *crossfader, *head_mix,
        *m_pBalance, *xFaderCurve, *xFaderCalibration;
    QList<ControlPushButton*>m_passthrough;
    int m_iLastThruRead[2];
    int m_iLastThruWrote[2];
    int m_iThruBufferCount[2];
    int m_iThruFill[2];
    bool m_bFilling[2];
    QFile df;
    QTextStream writer;

    ConstantGainCalculator m_headphoneGain;
    OrientationVolumeGainCalculator m_masterGain;
};

#endif
