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
 *  Main source file for the Joystick demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevJoystickHIDReportBuffer[sizeof(USB_JoystickReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Joystick_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = 0,
				.ReportINEndpoint             =
					{
						.Address              = JOYSTICK_EPADDR,
						.Size                 = JOYSTICK_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevJoystickHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevJoystickHIDReportBuffer),
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	
	SetupHardware();
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	for (;;)
	{
		HID_Device_USBTask(&Joystick_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
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

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Joystick_HID_Interface);

	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Joystick_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Joystick_HID_Interface);
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
	USB_JoystickReport_Data_t* JoystickReport = (USB_JoystickReport_Data_t*)ReportData;
	*ReportSize = sizeof(USB_JoystickReport_Data_t);

	// #################### JOYSTICK ####################
	uint8_t JoyStatus = Joystick_GetStatus();
	
	if (JoyStatus & JOY_UP)
		JoystickReport->Y = -100;
	else if (JoyStatus & JOY_DOWN)
		JoystickReport->Y =  100;

	if (JoyStatus & JOY_LEFT)
		JoystickReport->X = -100;
	else if (JoyStatus & JOY_RIGHT)
		JoystickReport->X =  100;

	//JoystickReport->Z = 0;
	
	// #################### UTIL ####################
	uint8_t UtilStatus = Util_GetStatus();
	
	if (UtilStatus & UTIL_START)
		JoystickReport->Buttons |= (1 << 0);
	if (UtilStatus & UTIL_SELECT)
		JoystickReport->Buttons |= (1 << 1);
	if (UtilStatus & UTIL_ENTER)
		JoystickReport->Buttons |= (1 << 2);
	if (UtilStatus & UTIL_ESC)
		JoystickReport->Buttons |= (1 << 3);
	
	// #################### BUTTON ####################
	uint8_t ButtonStatus = Buttons_GetStatus();
	
	if (ButtonStatus & BUTTON1)
		JoystickReport->Buttons |= (1 << 4);
	if (ButtonStatus & BUTTON2)
		JoystickReport->Buttons |= (1 << 5);
	if (ButtonStatus & BUTTON3)
		JoystickReport->Buttons |= (1 << 6);
	if (ButtonStatus & BUTTON4)
		JoystickReport->Buttons |= (1 << 7);
	if (ButtonStatus & BUTTON5)
		JoystickReport->Buttons |= (1 << 8);
	if (ButtonStatus & BUTTON6)
		JoystickReport->Buttons |= (1 << 9);

	// #################### GPIO ####################
	uint8_t GPIODStatus = GPIOD_GetStatus();
	if (GPIODStatus & GPIO1)
		JoystickReport->Buttons |= (1 << 10);
	if (GPIODStatus & GPIO2)
		JoystickReport->Buttons |= (1 << 11);

	uint8_t GPIOBStatus = GPIOB_GetStatus();
	if (GPIOBStatus & GPIO3)
		JoystickReport->Buttons |= (1 << 12);
	if (GPIOBStatus & GPIO4)
		JoystickReport->Buttons |= (1 << 13);
	if (GPIOBStatus & GPIO5)
		JoystickReport->Buttons |= (1 << 14);
	
	uint8_t GPIOCStatus = GPIOC_GetStatus();
	if (GPIOCStatus & GPIO6)
		JoystickReport->Buttons |= (1 << 15);


	return false;
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
	// Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}

