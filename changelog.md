# Change log

## 2023-Jan-08 (build 8)

- Fixed isssue where mode was not being saved between power cycles
## 2023-Jan-08 (build 7)

- Fixed bug where year was read from settings as a signed instead of unsigned short
- Removed github actions that are not needed

## 2023-Jan-08 (build 6)

- Save the month, year, and mode to eeprom on change
- Randomized seed on startup based on wifi noise

## 2023-Jan-08 (build 3-5)

- Added ability to update via Github

## 2023-Jan-03 (build 2)

- Added a fades pixel animation
- Moved modes to their own files
- Added reset wifi command

## 2022-Dec-29 (build 1)

- Intial version
- Added mode Calendar
  - Set current date objectives
  - Button to go to Next and Previouse month/year
  - Requires internet connection for NTP server (time)
- Captive portal on start up for entering Wifi settings
- Web server
  - Current state
  - Reset database
  - download database
- Reading/Writing to EEPROM
  - settings and database
- Added mode binary clock

## 2022-Dec-26

- Hardware recived

## 2022-Dec-11

- Hardware design done
- Submitted the PCBs to the fabrication servive (JLPCB) for manufacturing.