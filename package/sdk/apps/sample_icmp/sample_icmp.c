#include "sample_icmp.h"
#include "nrc_sdk.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#include "lwip/ip_addr.h"
#include "lwip/raw.h"
#include "lwip/ip.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"

#include "led_routine.h"

#ifndef ECHO_HEADER_SIZE
#define ECHO_HEADER_SIZE sizeof(struct icmp_echo_hdr)
#endif

#define LED_DELAY 500

static BOARD_STATE bd_state = DISCONECTED;

char* ip_address_str = NULL;
struct raw_pcb * ping_pcb = NULL;
ip_addr_t ip_ap = { .addr = 0 };
void free_resources()
{
	if (ip_address_str){
		nrc_mem_free(ip_address_str);
		ip_address_str = NULL;
	}

	if (ping_pcb){
		raw_remove(ping_pcb);
		nrc_mem_free(ping_pcb);
		ping_pcb = NULL;
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
	tWIFI_STATUS code = nrc_wifi_get_ip_address(0, &ip_address_str);
	nrc_usr_print("[%s]: IP is %s\r\n", __func__, ip_address_str);
	return code == WIFI_SUCCESS ? NRC_SUCCESS : NRC_FAIL;
}

u8_t on_recieve_ping_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p,
		const ip_addr_t *addr)
{
	nrc_usr_print("[%s]: echo reply from %s", __func__, ipaddr_ntoa(addr));
	nrc_usr_print("[%s]: ip address is %s equal\r\n", __func__, addr->addr == ip_ap.addr ? "" : "no");
	nrc_usr_print("Result of set: %s\r\n", set_pin_on_led() == NRC_SUCCESS ? "OK" : "FAIL");
	_delay_ms(LED_DELAY);
	nrc_usr_print("Result of reset: %s\r\n",reset_pin_on_led() == NRC_SUCCESS ? "OK" : "FAIL");
	return 0;
}

nrc_err_t init_ping_echo()
{
	int point_count = 0;
	for(int i=0;ip_address_str[i] != '\0';i++){
		if (point_count >= 3){
			ip_address_str[i] = '1';
			ip_address_str[i + 1] = '\0';
		}
		
		if (ip_address_str[i] == '.'){
			point_count++;
		}
	}

	if(!ipaddr_aton(ip_address_str, &ip_ap)){
		nrc_usr_print("[%s]: error on parsing ip address\r\n", __func__);
		return NRC_FAIL;
	}

	nrc_usr_print("[%s]:Gateway is %s\r\n", __func__, ip_address_str);

	ping_pcb = raw_new(IP_PROTO_ICMP);
	if (ping_pcb == NULL){
		nrc_usr_print("[%s]: Error for allocate ping PCB\r\n", __func__);
		return NRC_FAIL;
	}

	// Bind callback for receiving
	raw_recv(ping_pcb, on_recieve_ping_callback, NULL);
	return NRC_SUCCESS;
}

nrc_err_t send_echo()
{
	static u16_t seq_num = 0;
	struct pbuf *p;
	p = pbuf_alloc(PBUF_IP, ECHO_HEADER_SIZE + PING_DATA_SIZE, PBUF_RAM);
	if (p == NULL){
		nrc_usr_print("[%s]: error on allocating packet data\r\n", __func__);
		return NRC_FAIL;
	}
	struct icmp_echo_hdr * iecho = (struct icmp_echo_hdr *)p->payload;
	iecho->type = ICMP_ECHO;
	iecho->code = 0;
	iecho->chksum = 0;
	iecho->id = 0;
	iecho->seqno = htons(++seq_num);

	memset(p->payload + ECHO_HEADER_SIZE, 0, PING_DATA_SIZE);

	iecho->chksum = inet_chksum(iecho, ECHO_HEADER_SIZE + PING_DATA_SIZE);

	raw_sendto(ping_pcb, p, &ip_ap);

	pbuf_free(p);
	return NRC_SUCCESS;
}

void timer_routine()
{
	
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
			case INITED_IP:
				if (!init_ping_echo()){
					char* led_message = init_led() == NRC_SUCCESS ? "Success" : "Error";
					nrc_usr_print("[%s]: %s on init led\r\n", __func__, led_message);
					bd_state = SENDING_ECHO;
				} else {
					free_resources();
					bd_state = CONNETED_FOR_SSID;
				}
				break;
			case SENDING_ECHO:
				send_echo();
				_delay_ms(DELAY_TIME);
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
	nrc_wifi_register_event_handler(0, nrc_wifi_event_dispatcher);
	start_simple_icmp(param);
}
