/*******************************************************************************
 Copyright(c) 2013-2016 CloudMakers, s. r. o. All rights reserved.
 Copyright(c) 2017 Marco Gulino <marco.gulino@gmai.com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 *******************************************************************************/

#pragma once

#include "baseclient.h"
#include "defaultdevice.h"
#define MAX_GROUP_COUNT 16

class Group;
class Imager : public virtual INDI::DefaultDevice, public virtual INDI::BaseClient
{
    public:
        static const std::string DEVICE_NAME;
        Imager();
        virtual ~Imager() = default;

        // DefaultDevice

        virtual bool initProperties() override;
        virtual bool updateProperties() override;
        virtual void ISGetProperties(const char *dev) override;
        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
        virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                               char *formats[], char *names[], int n) override;
        virtual bool ISSnoopDevice(XMLEle *root) override;

        // BaseClient

        virtual void newDevice(INDI::BaseDevice baseDevice) override;
        virtual void newProperty(INDI::Property property) override;
        virtual void updateProperty(INDI::Property property) override;

        virtual void serverConnected() override;
        virtual void serverDisconnected(int exit_code) override;

    protected:
        virtual const char *getDefaultName() override;
        virtual bool Connect() override;
        virtual bool Disconnect() override;

    private:
        bool isRunning();
        bool isCCDConnected();
        bool isFilterConnected();
        void defineProperties();
        void deleteProperties();
        void initiateNextFilter();
        void initiateNextCapture();
        void startBatch();
        void abortBatch();
        void batchDone();
        void initiateDownload();

        char format[16];
        int group { 0 };
        int maxGroup { 0 };
        int image { 0 };
        int maxImage { 0 };
        const char *controlledCCD { nullptr };
        const char *controlledFilterWheel { nullptr };

        INDI::PropertyText ControlledDeviceTP {2};
        enum
        {
            CCD,
            FILTER
        };

        INDI::PropertyNumber GroupCountNP {1};
        enum
        {
            GROUP_COUNT
        };

        INDI::PropertyNumber ProgressNP {3};
        enum
        {
          GROUP,
          IMAGE,
          REMAINING_TIME
        };
        INDI::PropertySwitch BatchSP {2};
        enum
        {
            START,
            ABORT
        };
        INDI::PropertyLight StatusLP {2};

        ITextVectorProperty ImageNameTP;
        IText ImageNameT[2];
        INumberVectorProperty DownloadNP;
        INumber DownloadN[2];
        IBLOBVectorProperty FitsBP;
        IBLOB FitsB[1];

        INumberVectorProperty CCDImageExposureNP;
        INumber CCDImageExposureN[1];
        INumberVectorProperty CCDImageBinNP;
        INumber CCDImageBinN[2];
        ISwitch CCDUploadS[3];
        ISwitchVectorProperty CCDUploadSP;
        IText CCDUploadSettingsT[2] {};
        ITextVectorProperty CCDUploadSettingsTP;

        INumberVectorProperty FilterSlotNP;
        INumber FilterSlotN[1];

        std::vector<std::shared_ptr<Group>> groups;
        std::shared_ptr<Group> currentGroup() const;
        std::shared_ptr<Group> nextGroup() const;
        std::shared_ptr<Group> getGroup(int index) const;

};
