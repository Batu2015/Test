; ========================================================================================================================
; * Created by: Holtek Touch MCU Workshop, Version 3.5.1.0
; TKS_GLOBE_VARIES.inc
; 12:15:00 AM Sunday, November 04, 2018
; ========================================================================================================================
#include "BS67F350.inc"
#define _BS67F350_
#define _V413_
#define SystemClock     0
#define IO              0
#define KEY             1
; Keys' attribute & settings ...
#define KEY20           IO
#define Key20Threshold  16
#define KEY19           IO
#define Key19Threshold  16
#define KEY18           IO
#define Key18Threshold  16
#define KEY17           KEY
#define Key17Threshold  16
#define KEY16           KEY
#define Key16Threshold  16
#define KEY15           KEY
#define Key15Threshold  16
#define KEY14           KEY
#define Key14Threshold  16
#define KEY13           KEY
#define Key13Threshold  16
#define KEY4            IO
#define Key4Threshold   16
#define KEY3            IO
#define Key3Threshold   16
#define KEY2            IO
#define Key2Threshold   16
#define KEY1            IO
#define Key1Threshold   16
#define KEY5            IO
#define Key5Threshold   16
#define KEY6            IO
#define Key6Threshold   16
#define KEY7            IO
#define Key7Threshold   16
#define KEY8            IO
#define Key8Threshold   16
#define KEY9            IO
#define Key9Threshold   16
#define KEY10           IO
#define Key10Threshold  16
#define KEY11           IO
#define Key11Threshold  16
#define KEY12           IO
#define Key12Threshold  16
#define IO_TOUCH_ATTR   00000000000000011111000000000000b
#define IO_TOUCH_ATTR2  0000b
; Components' settings ...
; Global options' settings ...
#define DebounceTimes   0 	; 30mS
#define AutoCalibrationPeriod 7 	; x 8 (640mS)
#define HighSensitive   0 	; uncheck
#define MaximumKeyHoldTime 0 	; Disable
#define FastResponse    0 	; uncheck
#define AutoFrequencyHopping 0 	; uncheck
#define OneKeyActive    0 	; uncheck
#define PowerSave       0 	; uncheck
#define NoiseProtect    0 	; uncheck
#define MovingCalibration 0 	; uncheck
#define GlobeOptionA    ((AutoCalibrationPeriod << 4) | DebounceTimes)
#define GlobeOptionB    ((MaximumKeyHoldTime << 4) | HighSensitive)
#define GlobeOptionC    ((NoiseProtect << 4) | (MovingCalibration << 7) | (AutoFrequencyHopping << 3) | (FastResponse << 2) | (OneKeyActive << 5) | (PowerSave << 6))
