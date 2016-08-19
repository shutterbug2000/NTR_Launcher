/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nds.h>
#include <nds/arm7/input.h>
#include <nds/system.h>

void VcountHandler() {
	inputGetAndSend();
}

void VblankHandler(void) {
}
unsigned int * ROMCTRL=(unsigned int*)0x40001A4; 
unsigned int * SCFG_EXT=(unsigned int*)0x4004008; 
unsigned int * SCFG_MC=(unsigned int*)0x4004010; 

void SwitchToNTRCARD()
{
	*SCFG_EXT&=~0x80;
}

void SwitchToTWLCARD()
{
    *SCFG_EXT|=0x80;
}

// from https://github.com/d0k3/Decrypt9WIP/blob/master/source/gamecart/protocol.c#L48-L73
/*REG_CARDCONF2 = 0x0C;
REG_CARDCONF &= ~3;

if (REG_CARDCONF2 == 0xC) {
	while (REG_CARDCONF2 != 0);
}

if (REG_CARDCONF2 != 0)
	return;

REG_CARDCONF2 = 0x4;
while(REG_CARDCONF2 != 0x4);

REG_CARDCONF2 = 0x8;
while(REG_CARDCONF2 != 0x8);*/

// from http://problemkaputt.de/gbatek.htm#dsicontrolregistersscfg	
/*4004010h - DSi9 - SCFG_MC - Memory Card Interface Status (R)
  4004010h - DSi7 - SCFG_MC - Memory Card Interface Control (R/W)
  0     1st NDS Slot Game Cartridge (0=Inserted, 1=Ejected)               (R)
  1     1st NDS Slot Unknown/Undocumented (0)
  2-3   1st NDS Slot Power State (0=Off, 1=PrepareOn, 2=On, 3=RequestOff) (R/W)
  4     2nd NDS Slot Game Cartridge (always 1=Ejected) ;\DSi              (R)
  5     2nd NDS Slot Unknown/Undocumented (0)          ; prototype
  6-7   2nd NDS Slot Power State    (always 0=Off)     ;/relict           (R/W)
  8-15  Unknown/Undocumented (0)
  16-31 ARM7: See Port 4004012h, ARM9: Unspecified (0)
NDS-Slot related. Bit3 (and maybe Bit2) are probably R/W on ARM7 side (though the register is disabled on ARM7 side in cooking coach exploit, so R/W isn't possible in practice).
Note: Additionally, the NDS slot Reset pin can be toggled (via ROMCTRL.Bit29; that bit is writeable on ARM7 side on DSi; which wasn't supported on NDS).
Power state values:
  0=Power is off
  1=Prepare Power on (shall be MANUALLY changed to state=2)
  2=Power is on
  3=Request Power off (will be AUTOMATICALLY changed to state=0)
power_on:
  wait until state<>3                   ;wait if pwr off busy?
  exit if state<>0                      ;exit if already on?
  wait 1ms, then set state=1            ;prepare pwr on?       or want RESET ?
  wait 10ms, then set state=2           ;apply pwr on?
  wait 27ms, then set ROMCTRL=20000000h ;reset cart?  or rather RELEASE reset?
  wait 120ms                            ;more insane delay?
power_off:
  wait until state<>3                   ;wait if pwr off busy?
  exit if state<>2                      ;exit if already off?
  set state=3                           ;request pwr off?
  wait until state=0                    ;wait until pwr off?
Power Off is also done automatically by hardware when ejecting the cartridge.*/	

void PowerOffSlot()
{
	while(*SCFG_MC ==  0x0C);
	if(*SCFG_MC != 0) return;
	
	*SCFG_MC = 0x0C;
	while(*SCFG_MC !=  0x0);	
}

void PowerOnSlot()
{
	while(*SCFG_MC ==  0x0C);
	if(*SCFG_MC != 2) return;
	
	swiWaitForVBlank();
	*SCFG_MC = 0x04;
	swiWaitForVBlank();
	*SCFG_MC = 0x08;
	swiWaitForVBlank();
	swiWaitForVBlank();
	*ROMCTRL = 0x20000000;
	for (int i = 0; i < 7; i++) {
		swiWaitForVBlank();
	}
}

void ResetSlot() {
	PowerOffSlot();
	PowerOnSlot();
}


//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
    SwitchToTWLCARD();
	ResetSlot();
	SwitchToNTRCARD();

	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	SetYtrigger(80);

	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT);   

	// Keep the ARM7 mostly idle
	while (1) {

		if(*((vu32*)0x027FFE24) == (u32)0x027FFE04)
		{		
			irqDisable (IRQ_ALL);
			*((vu32*)0x027FFE34) = (u32)0x06000000;

			swiSoftReset();
		}
 
		swiWaitForVBlank();
	}
	
}

