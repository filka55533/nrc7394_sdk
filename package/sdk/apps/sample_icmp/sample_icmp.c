#include "sample_icmp.h"
#include "nrc_sdk.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

static BOARD_STATE bd_state = DISCONECTED;

char* ip_address = NULL;
void free_resources()
{
	if (ip_address){
		nrc_mem_free(ip_address);
		ip_address = NULL;
	}
}

nrc_err_t connect_wifi(WIFI_CONFIG* param)
{
  int is_ssid_found = false;
  SCAN_RESULTS results;
	int count_tries;

  nrc_usr_print("[%s]: Start connect to the network\r\n", __func__);
  nrc_usr_print("\t\t\tssid: %s\r\n", param->ssid);
  nrc_usr_print("\t\t\tsecurity: ");
  if (param->security_mode == WIFI_SEC_OPEN) {
		nrc_usr_print("open\r\n");
	} else if (param->security_mode == WIFI_SEC_WPA2) {
		nrc_usr_print("wpa2\r\n");
	} else if (param->security_mode == WIFI_SEC_WPA3_OWE) {
		nrc_usr_print("wpa3-owe\r\n");
	} else if (param->security_mode == WIFI_SEC_WPA3_SAE) {
		nrc_usr_print("wpa3-sae\r\n");
	} else {
		nrc_usr_print("unknown\r\n");
	}
  
  nrc_usr_print("\t\t\tpassword : %s\r\n",
				  (param->security_mode == WIFI_SEC_WPA2) || (param->security_mode == WIFI_SEC_WPA3_SAE) ? (char *)param->password : "");

	// set initial wifi configuration
	count_tries = 0;
	while (1) {
		tWIFI_STATUS code = wifi_init(param);
		if (code == WIFI_SUCCESS){
			nrc_usr_print("[%s]: wifi init Success !! \r\n", __func__);
			break;
		} else {
			nrc_usr_print("[%s]: wifi init Fail with code %d !! \r\n", __func__, code);
			if (++count_tries > MAX_COUNT_TRIES){
				nrc_usr_print("[%s]: wifi init max try count%d !! \r\n", __func__);
				return NRC_FAIL;
			}
			_delay_ms(1000);
		}
	}

	// find AP
	count_tries = 0;
	while(1){
		if (nrc_wifi_scan(0) == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(0, &results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(int i=0; i<results.n_result ; i++){
					if((strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0)
					   && (results.result[i].security == param->security_mode)){
						is_ssid_found = true;
						break;
					}
				}

				if(is_ssid_found){
					nrc_usr_print ("[%s] %s is found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			nrc_usr_print ("[%s] Scan fail !! \r\n", __func__);
			if (++count_tries > MAX_COUNT_TRIES){
				nrc_usr_print("[%s]: scan ap max try count%d !! \r\n", __func__);
				return NRC_FAIL;
			}
			_delay_ms(1000);
		}
	}

	//Connect AP 
	count_tries = 0;
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \r\n", __func__, param->ssid);
			uint8_t	txpower = 0;
			nrc_wifi_get_tx_power(0, &txpower);
			nrc_usr_print("[%s] TX Power (%d dBm)\r\n", __func__, txpower);
			break;
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\r\n", __func__, param->ssid);
			if (++count_tries > MAX_COUNT_TRIES){
				nrc_usr_print("[%s]: fail for connection max try count%d !! \r\n", __func__);
				return NRC_FAIL;
			}
			_delay_ms(1000);
		}
	}

	return NRC_SUCCESS;
}

void nrc_wifi_event_dispatcher(int vif, tWIFI_EVENT_ID event, int data_len, void *data)
{
	if (vif != 0){
		nrc_usr_print("[%s]: not my network exit\r\n", __func__);
		return;
	}

	if (event == WIFI_EVT_DISCONNECT){
		nrc_usr_print("[%s]: wifi disconnect\r\n", __func__);
		free_resources();
		bd_state = DISCONECTED;
	}
}

nrc_err_t get_ip_address()
{
	//Check if is ready IP
	int count_tries = 0;
	while (1){
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP ...\r\n",__func__);
			break;
		} else {
			nrc_usr_print("[%s] IP Address setting State : %d != NET_ADDR_SET(%d) yet...\r\n",
						  __func__, nrc_addr_get_state(0), NET_ADDR_SET);
			if (++count_tries > MAX_COUNT_TRIES){
				nrc_usr_print("[%s]check IP Address setting State max try count\r\n",__func__);
				return NRC_FAIL;
			}
		}
		_delay_ms(1000);
	}
	tWIFI_STATUS code = nrc_wifi_get_ip_address(0, &ip_address);
	nrc_usr_print("[%s]: IP is %s", __func__, ip_address);
	return code == WIFI_SUCCESS ? NRC_SUCCESS : NRC_FAIL;
}

void start_simple_icmp(WIFI_CONFIG* param)
{

	while(1){
		switch(bd_state){
			case DISCONECTED:
				//Try connect for ssid
				if (!connect_wifi(param)){
					bd_state = CONNETED_FOR_SSID;
				}
				break;
			case CONNETED_FOR_SSID:
				//Try get IP
				if (!get_ip_address()){
					bd_state = INITED_IP;
				} else {
					free_resources();
				}
				break;
			default:
				break;
		}


	}
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	WIFI_CONFIG wifi_config;
	WIFI_CONFIG* param = &wifi_config;
	
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	nrc_usr_print("[%s] \n", __func__);

	nrc_wifi_set_config(param);
  start_simple_icmp(param);
	
}
