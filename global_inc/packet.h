/*
 * packet.h
 *
 *      Created on: 2015年10月19日
 *      Author: Louis Ju
 *      Define Controller Message Protocol (CMP)
 */

#pragma once

#include <arpa/inet.h>
#include <stdio.h>
#include <syslog.h>
#include <ctime>
#include <iostream>
#include <map>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <string>
#include "container.h"
#include "logHandler/LogHandler.h"

using namespace std;
/*
 * CMP body data length
 */
#define MAX_DATA_LEN	2048

/*
 * this define socket packet for CMP
 */
struct CMP_HEADER
{
	int command_length;
	int command_id;
	int command_status;
	int sequence_number;
};

struct CMP_BODY
{
	char cmpdata[MAX_DATA_LEN];
};

struct CMP_BODY_UNLIMIT
{
	char *cmpdata;
};

struct CMP_PACKET
{
	CMP_HEADER cmpHeader;
	union
	{
		CMP_BODY cmpBody;
		CMP_BODY_UNLIMIT cmpBodyUnlimit;
	};
};

/*
 * 	CMP Command set
 */
#define generic_nack									0x80000000
#define bind_request									0x00000001
#define bind_response									0x80000001
#define authentication_request							0x00000002
#define authentication_response							0x80000002
#define access_log_request								0x00000003
#define access_log_response								0x80000003
#define initial_request									0x00000004
#define initial_response								0x80000004
#define sign_up_request									0x00000005
#define sign_up_response								0x80000005
#define unbind_request									0x00000006
#define unbind_response									0x80000006
#define update_request									0x00000007
#define update_response									0x80000007
#define long_data_request								0x00000008
#define long_data_response								0x80000008
#define reboot_request									0x00000010
#define reboot_response									0x80000010
#define config_request									0x00000011
#define config_response									0x80000011
#define wheelpies_request								0x00000012  // 虛擬運動教練
#define wheelpies_response								0x80000012
#define power_port_state_request						0x00000013
#define power_port_state_response						0x80000013
#define ser_api_signin_request							0x00000014
#define ser_api_signin_response							0x80000014
#define enquire_link_request							0x00000015
#define enquire_link_response							0x80000015
#define rdm_login_request								0x00000016
#define rdm_login_response								0x80000016
#define rdm_operate_request								0x00000017
#define rdm_operate_response							0x80000017
#define rdm_logout_request								0x00000018
#define rdm_logout_response								0x80000018
#define device_control_request							0x00000019
#define device_control_response							0x80000019
#define device_state_request							0x00000020
#define device_state_response							0x80000020
#define semantic_request								0x00000030
#define semantic_response								0x80000030
#define amx_control_request								0x00000040
#define amx_control_response							0x80000040
#define amx_status_request								0x00000041
#define amx_status_response								0x80000041
#define amx_broadcast_status_request					0x00000042
#define amx_broadcast_status_response					0x80000042
#define fcm_id_register_request             			0x00000044
#define fcm_id_register_response        				0x80000044
#define facebook_token_client_request      	 			0x00000045
#define facebook_token_client_response 					0x80000045
#define smart_building_qrcode_tokn_request 				0x00000050
#define smart_building_qrcode_tokn_response  			0x80000050
#define smart_building_appversion_request   			0x00000051
#define smart_building_appversion_response 				0x80000051
#define smart_building_getmeetingdata_request  			0x00000052
#define smart_building_getmeetingdata_response 			0x80000052
#define smart_building_amx_control_access_request  		0x00000053
#define smart_building_amx_control_access_response  	0x80000053
#define smart_building_wireless_power_charge_request 	0x00000055
#define smart_building_wireless_power_charge_response 	0x80000055
#define smart_building_door_control_request				0x00000056
#define smart_building_door_control_response			0x80000056
#define semantic_word_request							0x00000057
#define semantic_word_response							0x80000057
#define tts_request										0x00000058
#define tts_response									0x80000058
#define deidentify_request								0x00000059
#define deidentify_response								0x80000059
#define controller_die_request							0x000000FE
#define controller_die_response							0x800000FE

#define MAX_COMMAND										0x000000FF

/*
 * CMP status set
 */
#define STATUS_ROK								0x00000000		//No Error
#define STATUS_RINVMSGLEN						0x00000001		//Message Length is invalid
#define STATUS_RINVCMDLEN						0x00000002		//Command Length is invalid
#define STATUS_RINVCMDID						0x00000003		//Invalid Command ID
#define STATUS_RINVBNDSTS						0x00000004		//Incorrect BIND Status for given command
#define STATUS_RALYBND							0x00000005		//Already in Bound State
#define STATUS_RSYSBUSY							0x00000006      //System Busy
#define STATUS_RINVCMP							0x00000007		//Invalid CMP
#define STATUS_RSYSERR							0x00000008		//System Error
#define STATUS_RBINDFAIL						0x00000010		//Bind Failed
#define STATUS_RINVBODY							0x00000040		//Invalid Packet Body Data
#define STATUS_RINVCTRLID						0x00000041		//Invalid Controller ID
#define STATUS_RINVJSON							0x00000042		//Invalid JSON Data

/*8
 * Service Type
 */
#define TYPE_MOBILE_SERVICE					1
#define TYPE_POWER_CHARGE_SERVICE			2
#define TYPE_SDK_SERVICE					3
#define TYPE_TRACKER_SERVICE				4
#define TYPE_TRACKER_APPLIENCE				5
#define TYPE_TRACKER_TOY					6
#define TYPE_TRACKER_IOT					7

static map<int, string> mapCommand = create_map<int, string>( generic_nack, "generic_nack")( bind_request,
		"bind_request")( bind_response, "bind_response")(
authentication_request, "authentication_request")( authentication_response, "authentication_response")(
access_log_request, "access_log_request")( access_log_response, "access_log_response")( enquire_link_request,
		"enquire_link_request")(
enquire_link_response, "enquire_link_response")( unbind_request, "unbind_request")(
unbind_response, "unbind_response")( update_request, "update_request")(
update_response, "update_response")(
reboot_request, "reboot_request")( reboot_response, "reboot_response")(long_data_request, "long_data_request")(
long_data_response, "long_data_response")(
config_request, "config_request")(
config_response, "config_response")( wheelpies_request, "wheelpies_request")( wheelpies_response, "wheelpies_response")(
power_port_state_request, "power_port_state_request")(
power_port_state_response, "power_port_state_response")(
initial_request, "initial_request")( initial_response, "initial_response")(
sign_up_request, "sign_up_request")( sign_up_response, "sign_up_response")(
rdm_login_request, "rdm_login_request")(
rdm_login_response, "rdm_login_response")(rdm_operate_request, "rdm_operate_request")(rdm_operate_response,
		"rdm_operate_response")(
rdm_logout_request, "rdm_logout_request")(rdm_logout_response, "rdm_logout_response")(
semantic_request, "semantic_request")(semantic_response, "semantic_response")(
amx_control_request, "amx_control_request")(amx_control_response, "amx_control_response")(amx_status_request,
		"amx_status_request")(amx_status_response, "amx_status_response")(amx_broadcast_status_request,
		"amx_broadcast_status_request")(amx_broadcast_status_response, "amx_broadcast_status_response")(
fcm_id_register_request, "fcm_id_register_request")(fcm_id_register_response, "fcm_id_register_response")(
facebook_token_client_request, "facebook_token_client_request")(facebook_token_client_response,
		"facebook_token_client_response")(
smart_building_qrcode_tokn_request, "smart_building_qrcode_tokn_request")(smart_building_qrcode_tokn_response,
		"smart_building_qrcode_tokn_response")(smart_building_appversion_request, "smart_building_appversion_request")(
smart_building_appversion_response, "smart_building_appversion_response")(smart_building_getmeetingdata_request,
		"smart_building_getmeetingdata_request")(smart_building_getmeetingdata_response,
		"smart_building_getmeetingdata_response")(smart_building_amx_control_access_request,
		"smart_building_amx_control_access_request")(smart_building_amx_control_access_response,
		"smart_building_amx_control_access_response")(smart_building_wireless_power_charge_request,
		"smart_building_wireless_power_charge_request")(smart_building_wireless_power_charge_response,
		"smart_building_wireless_power_charge_response")(semantic_word_request, "semantic_word_request")(
semantic_word_response, "semantic_word_response")(tts_request, "tts_request")(tts_response, "tts_response")(
		controller_die_request, "controller_die_request")(
controller_die_response, "controller_die_response")(deidentify_request,"deidentify_request")(deidentify_response,"deidentify_response");

static map<int, string> mapStatus = create_map<int, string>\
( STATUS_ROK, "No Error")( STATUS_RINVMSGLEN,
		"Message Length is invalid")(
STATUS_RINVCMDLEN, "Command Length is invalid")( STATUS_RINVCMDID, "Invalid Command ID")( STATUS_RINVBNDSTS,
		"Incorrect BIND Status for given command")( STATUS_RALYBND, "Already in Bound State")( STATUS_RSYSERR,
		"System Error")(
STATUS_RBINDFAIL, "Bind Failed")( STATUS_RINVBODY, "Invalid Packet Body Data")(
STATUS_RINVCTRLID, "Invalid Controller ID")(
STATUS_RINVJSON, "Invalid JSON Data")(STATUS_RSYSBUSY, "System Busy");

__attribute__ ((unused)) inline static void printPacket(int nCommand, int nStatus, int nSequence, int nLength,
		const char * szDesc, int nClienFD = 0)
{
	char szCmd[48];
	char szSta[32];
	memset(szCmd, 0, sizeof(szCmd));
	memset(szSta, 0, sizeof(szSta));

	strcpy(szCmd, mapCommand[nCommand].c_str());
	strcpy(szSta, mapStatus[nStatus].c_str());

	_log("%s CMP : Command=%-20s Status=%-20s Sequence=%d Length=%d [Socket FD=%d]", szDesc, szCmd, szSta, nSequence,
			nLength, nClienFD);
}

static int msnSequence = 0x00000000;
__attribute__ ((unused)) inline static int getSequence()
{
	++msnSequence;
	if(0x7FFFFFFF <= msnSequence)
		msnSequence = 0x00000001;
	return msnSequence;
}

