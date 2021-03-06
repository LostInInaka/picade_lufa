/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the KeyboardMouse demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "KeyboardMouse.h"

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** Buffer to hold the previously generated Mouse HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevMouseHIDReportBuffer[sizeof(USB_MouseReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the keyboard HID
 *  interface within the device.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = 0,
				.ReportINEndpoint             =
					{
						.Address              = KEYBOARD_IN_EPADDR,
						.Size                 = HID_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
	};

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the mouse HID
 *  interface within the device.
 */
USB_ClassInfo_HID_Device_t Mouse_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = 1,
				.ReportINEndpoint             =
					{
						.Address              = MOUSE_IN_EPADDR,
						.Size                 = HID_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevMouseHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevMouseHIDReportBuffer),
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	for (;;)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
		HID_Device_USBTask(&Mouse_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	
	Picade_Init();

	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Mouse_HID_Interface);

	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
	HID_Device_ProcessControlRequest(&Mouse_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
	HID_Device_MillisecondElapsed(&Mouse_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	/* Determine which interface must have its report generated */
	if (HIDInterfaceInfo == &Keyboard_HID_Interface)
	{
		USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;
		*ReportSize = sizeof(USB_KeyboardReport_Data_t);

		uint8_t UsedKeyCodes = 0;
		
		// #################### UTIL ####################
		uint8_t UtilStatus = Util_GetStatus();

		if (UtilStatus & UTIL_ENTER)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_ENTER;
		if (UtilStatus & UTIL_ESC)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_ESCAPE;
		
		// #################### BUTTON ####################
		uint8_t ButtonStatus = Buttons_GetStatus();
		
		if (ButtonStatus & BUTTON1)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_LEFT_CONTROL;
		if (ButtonStatus & BUTTON2)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_LEFT_ALT;
		if (ButtonStatus & BUTTON3)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_A;
		if (ButtonStatus & BUTTON4)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_Z;
		if (ButtonStatus & BUTTON5)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_S;
		if (ButtonStatus & BUTTON6)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_X;

		// #################### GPIO ####################
		uint8_t GPIODStatus = GPIOD_GetStatus();
		if (GPIODStatus & GPIO1)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_D;
		if (GPIODStatus & GPIO2)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_C;

		uint8_t GPIOBStatus = GPIOB_GetStatus();
		if (GPIOBStatus & GPIO3)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_I;
		if (GPIOBStatus & GPIO4)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_J;
		if (GPIOBStatus & GPIO5)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_K;
		
		uint8_t GPIOCStatus = GPIOC_GetStatus();
		if (GPIOCStatus & GPIO6)
			KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_L;

		//if (UsedKeyCodes)
		//	KeyboardReport->Modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
		
		return false;
	}
	else
	{
		USB_MouseReport_Data_t* MouseReport = (USB_MouseReport_Data_t*)ReportData;
		*ReportSize = sizeof(USB_MouseReport_Data_t);

		// #################### JOYSTICK ####################
		uint8_t JoyStatus = Joystick_GetStatus();
		
		if (JoyStatus & JOY_UP)
			MouseReport->Y = -1;
		else if (JoyStatus & JOY_DOWN)
			MouseReport->Y =  1;

		if (JoyStatus & JOY_LEFT)
			MouseReport->X = -1;
		else if (JoyStatus & JOY_RIGHT)
			MouseReport->X =  1;

		// #################### UTIL ####################
		uint8_t UtilStatus = Util_GetStatus();
		
		if (UtilStatus & UTIL_START)
			MouseReport->Button |= (1 << 0);
		if (UtilStatus & UTIL_SELECT)
			MouseReport->Button |= (1 << 1);

		
		return true;
	}
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	if (HIDInterfaceInfo == &Keyboard_HID_Interface)
	{
		uint8_t  LEDMask   = LEDS_NO_LEDS;
		uint8_t* LEDReport = (uint8_t*)ReportData;

		if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK)
			LEDMask |= LEDS_LED1;

		if (*LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
			LEDMask |= LEDS_LED1;

		if (*LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
			LEDMask |= LEDS_LED1;

		LEDs_SetAllLEDs(LEDMask);
	}
}

