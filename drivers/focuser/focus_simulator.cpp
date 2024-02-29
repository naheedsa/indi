/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "focus_simulator.h"

#include <cmath>
#include <memory>
#include <cstring>
#include <unistd.h>

// We declare an auto pointer to focusSim.
static std::unique_ptr<FocusSim> focusSim(new FocusSim());

/************************************************************************************
 *
************************************************************************************/
FocusSim::FocusSim()
{
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED | FOCUSER_HAS_BACKLASH);
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::Connect()
{
    SetTimer(1000);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::Disconnect()
{
    return true;
}

/************************************************************************************
 *
************************************************************************************/
const char *FocusSim::getDefaultName()
{
    return "Focuser Simulator";
}

/************************************************************************************
 *
************************************************************************************/
void FocusSim::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    INDI::Focuser::ISGetProperties(dev);

    defineProperty(ModeSP);
    loadConfig(true, "Mode");
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::initProperties()
{
    INDI::Focuser::initProperties();

    SeeingNP[SIM_SEEING].fill("SIM_SEEING", "arcseconds", "%4.2f", 0, 60, 0, 3.5);
    SeeingNP.fill(getDeviceName(), "SEEING_SETTINGS", "Seeing", MAIN_CONTROL_TAB, IP_RW, 60,
                  IPS_IDLE);

    FWHMNP[SIM_FWHM].fill("SIM_FWHM", "arcseconds", "%4.2f", 0, 60, 0, 7.5);
    FWHMNP.fill(getDeviceName(), "FWHM", "FWHM", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    TemperatureNP[TEMPERATURE].fill("TEMPERATURE", "Celsius", "%6.2f", -50., 70., 0., 0.);
    TemperatureNP.fill(getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    DelayNP[0].fill("DELAY_VALUE", "Value (uS)", "%.f", 0, 60000, 100, 100);
    DelayNP.fill(getDeviceName(), "DELAY", "Delay", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    ModeSP[MODE_ALL].fill("All", "All", ISS_ON);
    ModeSP[MODE_ABSOLUTE].fill("Absolute", "Absolute", ISS_OFF);
    ModeSP[MODE_RELATIVE].fill("Relative", "Relative", ISS_OFF);
    ModeSP[MODE_TIMER].fill("Timer", "Timer", ISS_OFF);
    ModeSP.fill(getDeviceName(), "Mode", "Mode", MAIN_CONTROL_TAB, IP_RW,
                ISR_1OFMANY, 60, IPS_IDLE);

    initTicks = sqrt(FWHMNP[SIM_FWHM].value - SeeingNP[SIM_SEEING].value) / 0.75;

    FocusSpeedN[0].min   = 1;
    FocusSpeedN[0].max   = 5;
    FocusSpeedN[0].step  = 1;
    FocusSpeedN[0].value = 1;

    FocusAbsPosN[0].value = FocusAbsPosN[0].max / 2;

    internalTicks = FocusAbsPosN[0].value;

    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineProperty(SeeingNP);
        defineProperty(FWHMNP);
        defineProperty(TemperatureNP);
        defineProperty(DelayNP);
    }
    else
    {
        deleteProperty(SeeingNP.getName());
        deleteProperty(FWHMNP.getName());
        deleteProperty(TemperatureNP.getName());
        deleteProperty(DelayNP);
    }

    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Modes
        if (ModeSP.isNameMatch(name))
        {
            ModeSP.update(states, names, n);
            uint32_t cap = 0;
            int index    = ModeSP.findOnSwitchIndex();

            switch (index)
            {
            case MODE_ALL:
                cap = FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED;
                break;

            case MODE_ABSOLUTE:
                cap = FOCUSER_CAN_ABS_MOVE;
                break;

            case MODE_RELATIVE:
                cap = FOCUSER_CAN_REL_MOVE;
                break;

            case MODE_TIMER:
                cap = FOCUSER_HAS_VARIABLE_SPEED;
                break;

            default:
                ModeSP.setState(IPS_ALERT);
                ModeSP.apply("Unknown mode index %d", index);
                return true;
            }

            FI::SetCapability(cap);
            ModeSP.setState(IPS_OK);
            ModeSP.apply();
            return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "SEEING_SETTINGS") == 0)
        {
            SeeingNP.setState(IPS_OK);
            SeeingNP.update(values, names, n);

            SeeingNP.apply();
            return true;
        }

        if (strcmp(name, "FOCUS_TEMPERATURE") == 0)
        {
            TemperatureNP.setState(IPS_OK);
            TemperatureNP.update(values, names, n);

            TemperatureNP.apply();
            return true;
        }

        // Delay
        if (DelayNP.isNameMatch(name))
        {
            DelayNP.update(values, names, n);
            DelayNP.setState(IPS_OK);
            DelayNP.apply();
            saveConfig(true, DelayNP.getName());
            return true;
        }
    }

    // Let INDI::Focuser handle any other number properties
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

/************************************************************************************
 *
************************************************************************************/
IPState FocusSim::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    double mid         = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    int mode           = ModeSP.findOnSwitchIndex();
    double targetTicks = ((dir == FOCUS_INWARD) ? -1 : 1) * (speed * duration);

    internalTicks += targetTicks;

    if (mode == MODE_ALL)
    {
        if (internalTicks < FocusAbsPosN[0].min || internalTicks > FocusAbsPosN[0].max)
        {
            internalTicks -= targetTicks;
            LOG_ERROR("Cannot move focuser in this direction any further.");
            return IPS_ALERT;
        }
    }

    // simulate delay in motion as the focuser moves to the new position
    usleep(duration * 1000);

    double ticks = initTicks + (internalTicks - mid) / 5000.0;

    FWHMNP[SIM_FWHM].value = 0.5625 * ticks * ticks + SeeingNP[SIM_SEEING].value;

    LOGF_DEBUG("TIMER Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
               FWHMNP[SIM_FWHM].value);

    if (mode == MODE_ALL)
    {
        FocusAbsPosN[0].value = internalTicks;
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    if (FWHMNP[SIM_FWHM].value < SeeingNP[SIM_SEEING].value)
        FWHMNP[SIM_FWHM].value = SeeingNP[SIM_SEEING].value;

    FWHMNP.apply();

    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
IPState FocusSim::MoveAbsFocuser(uint32_t targetTicks)
{
    double mid = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;

    internalTicks = targetTicks;

    // Limit to +/- 10 from initTicks
    double ticks = initTicks + (targetTicks - mid) / 5000.0;

    // simulate delay in motion as the focuser moves to the new position
    usleep(std::abs((targetTicks - FocusAbsPosN[0].value) * DelayNP[0].getValue()));

    FocusAbsPosN[0].value = targetTicks;

    FWHMNP[SIM_FWHM].value = 0.5625 * ticks * ticks + SeeingNP[SIM_SEEING].value;

    LOGF_DEBUG("ABS Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
               FWHMNP[SIM_FWHM].value);

    if (FWHMNP[SIM_FWHM].value < SeeingNP[SIM_SEEING].value)
        FWHMNP[SIM_FWHM].value = SeeingNP[SIM_SEEING].value;

    FWHMNP.apply();

    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
IPState FocusSim::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    uint32_t targetTicks = FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1));
    FocusAbsPosNP.s = IPS_BUSY;
    IDSetNumber(&FocusAbsPosNP, nullptr);
    return MoveAbsFocuser(targetTicks);
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::SetFocuserSpeed(int speed)
{
    INDI_UNUSED(speed);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::SetFocuserBacklash(int32_t steps)
{
    INDI_UNUSED(steps);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::SetFocuserBacklashEnabled(bool enabled)
{
    INDI_UNUSED(enabled);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    DelayNP.save(fp);

    return true;
}
