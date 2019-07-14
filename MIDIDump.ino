/*
 * MIT License
 *
 * Copyright (c) 2019 gdsports625@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <M5Stack.h>
#include <usbh_midi.h>
#include <SPI.h>

// Turn this on slows down MIDI processing. Turn off for
// best performance.
#define DEBUG_USB 0

#define DBSerial Serial

/*! Enumeration of MIDI types */
enum MidiType
{
    InvalidType           = 0x00,    ///< For notifying errors
    NoteOff               = 0x80,    ///< Note Off
    NoteOn                = 0x90,    ///< Note On
    AfterTouchPoly        = 0xA0,    ///< Polyphonic AfterTouch
    ControlChange         = 0xB0,    ///< Control Change / Channel Mode
    ProgramChange         = 0xC0,    ///< Program Change
    AfterTouchChannel     = 0xD0,    ///< Channel (monophonic) AfterTouch
    PitchBend             = 0xE0,    ///< Pitch Bend
    SystemExclusive       = 0xF0,    ///< System Exclusive
    TimeCodeQuarterFrame  = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
    SongPosition          = 0xF2,    ///< System Common - Song Position Pointer
    SongSelect            = 0xF3,    ///< System Common - Song Select
    TuneRequest           = 0xF6,    ///< System Common - Tune Request
    Clock                 = 0xF8,    ///< System Real Time - Timing Clock
    Start                 = 0xFA,    ///< System Real Time - Start
    Continue              = 0xFB,    ///< System Real Time - Continue
    Stop                  = 0xFC,    ///< System Real Time - Stop
    ActiveSensing         = 0xFE,    ///< System Real Time - Active Sensing
    SystemReset           = 0xFF,    ///< System Real Time - System Reset
};

USB Usb;
USBH_MIDI MIDIUSBH(&Usb);
uint8_t   UsbHSysEx[1026];
uint8_t   *UsbHSysExPtr;

void setup()
{
  M5.begin();
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.println("MIDI Dump");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 31);
  M5.Lcd.println("Button A clears");
  M5.Lcd.setTextColor(TFT_YELLOW);

  DBSerial.begin(115200);
  if (Usb.Init() == -1) {
    DBSerial.println(F("USB host init failed"));
    M5.Lcd.println(F("USB host init failed"));
    while (1); //halt
  }
  UsbHSysExPtr = UsbHSysEx;
  DBSerial.println(F("USB host running"));
}

inline void dumpSongSelect(uint8_t n)
{
  M5.Lcd.print("Song Select ");
  M5.Lcd.println(n);
}

inline void dumpTimeCodeQuarterFrame(uint8_t n)
{
  M5.Lcd.print("Time Code Quarter Frame ");
  M5.Lcd.println(n);
}

inline void dumpProgramChange(uint8_t n, uint8_t chan)
{
  M5.Lcd.print("Program Change ");
  M5.Lcd.print(n);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

inline void dumpAfterTouch(uint8_t n, uint8_t chan)
{
  M5.Lcd.print("After Touch ");
  M5.Lcd.print(n);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

inline void dumpSongPosition(uint16_t pos)
{
  M5.Lcd.print("Song Position ");
  M5.Lcd.println(pos);
}

inline void dumpNoteOff(uint8_t note, uint8_t velocity, uint8_t chan)
{
  M5.Lcd.print("Note Off ");
  M5.Lcd.print(note);
  M5.Lcd.print(',');
  M5.Lcd.print(velocity);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

inline void dumpNoteOn(uint8_t note, uint8_t velocity, uint8_t chan)
{
  M5.Lcd.print("Note On ");
  M5.Lcd.print(note);
  M5.Lcd.print(',');
  M5.Lcd.print(velocity);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

inline void dumpPolyPressure(uint8_t note, uint8_t velocity, uint8_t chan)
{
  M5.Lcd.print("Polyphonic Key Pressure ");
  M5.Lcd.print(note);
  M5.Lcd.print(',');
  M5.Lcd.print(velocity);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

inline void dumpControlChange(uint8_t control, uint8_t change, uint8_t chan)
{
  M5.Lcd.print("Control Chng ");
  M5.Lcd.print(control);
  M5.Lcd.print(',');
  M5.Lcd.print(change);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

inline void dumpPitchBend(uint16_t bend, uint8_t chan)
{
  M5.Lcd.print("Pitch Bend ");
  M5.Lcd.print(bend);
  M5.Lcd.print(" chan ");
  M5.Lcd.println(chan);
}

void dumpSysEx(uint8_t *p, size_t len)
{
  char outbuf[8];

  M5.Lcd.print("SysEx(");
  M5.Lcd.print(len);
  M5.Lcd.print("):");
  for (size_t i = 0; i < min(16, len); i++) {
    snprintf(outbuf, sizeof(outbuf), " %02X", *p++);
    M5.Lcd.print(outbuf);
  }
  M5.Lcd.println();
}

void USBHostDump()
{
  uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
  uint8_t rcode = 0;     //return code
  uint16_t rcvd;
  uint8_t readCount = 0;

  rcode = MIDIUSBH.RecvData( &rcvd, recvBuf);

  //data check
  if (rcode != 0 || rcvd == 0) return;
  if ( recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0 ) {
    return;
  }

  uint8_t *p = recvBuf;
  while (readCount < rcvd)  {
    if (*p == 0 && *(p + 1) == 0) break; //data end
#if DEBUG_USB
    DBSerial.print(F("USB "));
    DBSerial.print(p[0], HEX);
    DBSerial.print(' ');
    DBSerial.print(p[1], HEX);
    DBSerial.print(' ');
    DBSerial.print(p[2], HEX);
    DBSerial.print(' ');
    DBSerial.println(p[3], HEX);
#endif
    uint8_t header = *p & 0x0F;
    p++;
    uint8_t chan = (*p & 0x0F) + 1;
    MidiType miditype = (MidiType)(*p & 0xF0);
#if DEBUG_USB
    DBSerial.print(F("header, chan, miditype:"));
    DBSerial.print(header, HEX);
    DBSerial.print(',');
    DBSerial.print(chan, HEX);
    DBSerial.print(',');
    DBSerial.println(miditype, HEX);
#endif
    switch (header) {
      case 0x00:  // Misc. Reserved for future extensions.
        break;
      case 0x01:  // Cable events. Reserved for future expansion.
        break;
      case 0x02:  // Two-byte System Common messages
        switch (p[0]) {
          case SongSelect:
            dumpSongSelect(p[1]);
            break;
          case TimeCodeQuarterFrame:
            dumpTimeCodeQuarterFrame(p[1]);
            break;
          default:
            break;
        }
        break;
      case 0x0C:  // Program Change
        dumpProgramChange(p[1], chan);
        break;
      case 0x0D:  // Channel Pressure
        dumpAfterTouch(p[1], chan);
        break;
      case 0x03:  // Three-byte System Common messages
        switch (p[0]) {
          case SongPosition:
            dumpSongPosition(((p[2] & 0x7F) << 7) | (p[1] & 0x7F));
            break;
          default:
            break;
        }
        break;
      case 0x08:  // Note-off
      case 0x09:  // Note-on
      case 0x0A:  // Poly-KeyPress
      case 0x0B:  // Control Change
      case 0x0E:  // PitchBend Change
        switch (miditype) {
          case MidiType::NoteOff:
            dumpNoteOff(p[1], p[2], chan);
            break;
          case MidiType::NoteOn:
            dumpNoteOn(p[1], p[2], chan);
            break;
          case MidiType::AfterTouchPoly:
            dumpPolyPressure(p[1], p[2], chan);
            break;
          case MidiType::ControlChange:
            dumpControlChange(p[1], p[2], chan);
            break;
          case MidiType::PitchBend:
            dumpPitchBend(((p[2] & 0x7F) << 7) | (p[1] & 0x7F), chan);
            break;
        }
        break;

      case 0x04:  // SysEx starts or continues
        UsbHSysExPtr = UsbHSysEx;
        memcpy(UsbHSysExPtr, p, 3);
        UsbHSysExPtr += 3;
        break;
      case 0x05:  // Single-byte System Common Message or SysEx ends with the following single byte
        memcpy(UsbHSysExPtr, p, 1);
        UsbHSysExPtr += 1;
        dumpSysEx(UsbHSysEx, UsbHSysExPtr - UsbHSysEx);
        UsbHSysExPtr = UsbHSysEx;
        break;
      case 0x06:  // SysEx ends with the following two bytes
        memcpy(UsbHSysExPtr, p, 2);
        UsbHSysExPtr += 2;
        dumpSysEx(UsbHSysEx, UsbHSysExPtr - UsbHSysEx);
        UsbHSysExPtr = UsbHSysEx;
        break;
      case 0x07:  // SysEx ends with the following three bytes
        memcpy(UsbHSysExPtr, p, 3);
        UsbHSysExPtr += 3;
        dumpSysEx(UsbHSysEx, UsbHSysExPtr - UsbHSysEx);
        UsbHSysExPtr = UsbHSysEx;
        break;
      case 0x0F:  // Single Byte, TuneRequest, Clock, Start, Continue, Stop, etc.
        switch (p[0]) {
          case MidiType::TuneRequest:
            M5.Lcd.println("TuneRequest");
            break;
          case MidiType::Clock:
            M5.Lcd.println("Clock");
            break;
          case MidiType::Start:
            M5.Lcd.println("Start");
            break;
          case MidiType::Continue:
            M5.Lcd.println("Continue");
            break;
          case MidiType::Stop:
            M5.Lcd.println("Stop");
            break;
          case MidiType::ActiveSensing:
            M5.Lcd.println("Active Sensing");
            break;
          case MidiType::SystemReset:
            M5.Lcd.println("Reset");
            break;
          default:
            break;
        }
        break;
    }
    p += 3;
    readCount += 4;
  }
}

void loop()
{
  Usb.Task();

  M5.update();
  if (M5.BtnA.wasReleased()) {
    M5.Lcd.clear(TFT_NAVY);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_YELLOW);
  }

  if (MIDIUSBH) {
    USBHostDump();
  }
}
