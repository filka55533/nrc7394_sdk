/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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
 *
 */

#include "nrc_sdk.h"

#include <esp_http_server.h>
#include <esp_err.h>
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wifi_config.h"
#include "nrc_lwip.h"
#include "dhcpserver.h"
#include "nvs.h"
/* wifi_common */
#include "nvs_config.h"

static nvs_handle_t nvs_handle;
#define NVS_WIFI_CONFIGURED "wifi_configured"

#define AP_SSID "halow_setup"
#define SECURITY_MODE_SIZE 10

static const char input_element[] = R"rawliteral(
<!DOCTYPE html>
<html>
<body>

<h1>Configure Wi-Fi</h1>

<form action="/hello">
  <label for="ssid">Wi-Fi name:</label>
  <input type="text" id="ssid" name="ssid"><br><br>
  <label for="passwd">Wi-Fi password:</label>
  <input type="text" id="passwd" name="passwd"><br><br>
  <label for="security">Security mode:</label>
  <select id="security" name="security">
    <option value="open">OPEN</option>
    <option value="wpa2">WPA2</option>
    <option value="wpa3-sae">WPA3-SAE</option>
    <option value="wpa3-owe">WPA3-OWE</option>
  </select>
  <input type="submit" value="Submit">
</form>

<p>Input desired values and click the "Submit" button.</p>

</body>
</html>

)rawliteral";

static const char complete_element[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
	<title>Configuration complete</title>
</head>
<body>
	<h1>Setup completed!</h1>
	<button onclick="reboot()">Reboot</button>

	<script>
		function reboot() {
			// Perform the necessary actions to reboot the system
			// after the configuration is completed
			window.location.href = "reboot.html";
		}
	</script>
</body>
</html>
)rawliteral";

static const char rebooting_element[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
	<title>Rebooting...</title>
</head>
<body>
	<h1>Rebooting...</h1>
	<p>The device will connect to configured AP.</p>
</body>
</html>
)rawliteral";

static nrc_err_t start_softap(WIFI_CONFIG* param)
{
	int i = 0;
	int count = 0;
	int dhcp_server = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	count = 10;
	dhcp_server = param->dhcp_server;

	/* set initial wifi configuration */
	while(1) {
		if (wifi_init(param) == WIFI_SUCCESS) {
			nrc_usr_print ("\033[31m [%s] wifi_init Success !! \033[39m\n", __func__);
			break;
		} else {
			nrc_usr_print ("\033[31m [%s] wifi_init Failed !! \033[39m\n", __func__);
			_delay_ms(1000);
		}
	}
	nrc_usr_print ("\033[31m [%s] calling wifi_start_softap \033[39m\n", __func__);
	if (wifi_start_softap(param) != WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail to start softap\n", __func__);
		return NRC_FAIL;
	}
	nrc_usr_print ("\033[31m [%s] calling nrc_wifi_softap_set_ip \033[39m\n", __func__);
	if (nrc_wifi_softap_set_ip(0, (char *)&param->static_ip, (char *)&param->netmask, (char *)&param->gateway) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set AP's IP\n", __func__);
		return NRC_FAIL;
	}

	nrc_usr_print ("\033[31m [%s] dhcp_server start? %d \033[39m\n", __func__, dhcp_server);
	if (dhcp_server == 1) {
		nrc_usr_print("\033[31m [%s] Trying to start DHCP Server \033[39m\n",	__func__);
		if(nrc_wifi_softap_start_dhcp_server(0) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to start dhcp server\n", __func__);
			return NRC_FAIL;
		}
	}

	return NRC_SUCCESS;
}

static nrc_err_t connect_to_ap(WIFI_CONFIG* param)
{
	int i = 0;
	int count = 0;
	int dhcp_server = 0;

	nrc_wifi_set_config(param);

	count = 10;
	dhcp_server = param->dhcp_server;

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] wifi initialization failed.\n", __func__);
		return NRC_FAIL;
	}

	nrc_usr_print ("[%s] Trying to connect to AP - %s...\n", __func__, param->ssid);
	do {
		if (wifi_connect(param) == WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Wi-Fi connection successful...\n", __func__);
			break;
		}
		nrc_usr_print ("[%s] Connect to AP(%s) timed out, trying again...\n", __func__, param->ssid);
		i++;
	} while (i < 10);

	if (i >= 10) {
		nrc_usr_print ("[%s] Wi-Fi connection failed. Check AP availability and SSID, and try again...\n", __func__);
		return NRC_FAIL;
	}

	i = 0;
	do {
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP received!\n",__func__);
			break;
		}
		_delay_ms(1000);
		i++;
	} while (i < 10);

	if (nrc_addr_get_state(0) != NET_ADDR_SET) {
		nrc_usr_print("[%s] Fail to connect or get IP !\n",__func__);
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] Device is online connected to %s\n",__func__, param->ssid);
	return NRC_SUCCESS;
}

/* Replace the encoded Hex substring from URL encoded string */
static void replaceUrlEncodedSubstring(char *str)
{
    char *ptr = str;

    while ((ptr = strstr(ptr, "%")) != NULL) {
        char hex[3] = {ptr[1], ptr[2], '\0'};
        int ascii = strtol(hex, NULL, 16);
        if (isprint(ascii) || ascii == '\n' || ascii == '\t') {
            *ptr = ascii;
            memmove(ptr + 1, ptr + 3, strlen(ptr + 3) + 1);
        } else {
            memmove(ptr, ptr + 3, strlen(ptr + 3) + 1);
        }
    }
}

/* An HTTP GET handler */
static esp_err_t input_get_handler(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;
	WIFI_CONFIG param;

	/* Get header value string length and allocate memory for length + 1,
	 * extra byte for null termination */
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
		buf = nrc_mem_malloc(buf_len);
		/* Copy null terminated value string into buffer */
		if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found header => Host: %s\n", buf);
		}
		nrc_mem_free(buf);
	}

	buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
	if (buf_len > 1) {
		buf = nrc_mem_malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found header => Test-Header-2: %s\n", buf);
		}
		nrc_mem_free(buf);
	}

	buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
	if (buf_len > 1) {
		buf = nrc_mem_malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found header => Test-Header-1: %s\n", buf);
		}
		nrc_mem_free(buf);
	}

	/* Read URL query string length and allocate memory for length + 1,
	 * extra byte for null termination */
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = nrc_mem_malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found URL query => %s\n", buf);
			char security_mode[SECURITY_MODE_SIZE];
			memset(&param, 0x0, WIFI_CONFIG_SIZE);
			nrc_wifi_set_config(&param);
			memset(security_mode, 0x0, SECURITY_MODE_SIZE);

			/* Get value of expected key from query string */
			if (httpd_query_key_value(buf, "ssid", (char *) param.ssid, sizeof(param.ssid)) == ESP_OK) {
				replaceUrlEncodedSubstring((char *)param.ssid);
				nrc_usr_print ("Found URL query parameter => ssid=%s\n", param.ssid);
			}
			if (httpd_query_key_value(buf, "passwd", (char *) param.password, sizeof(param.password)) == ESP_OK) {
				replaceUrlEncodedSubstring((char *)param.password);
				nrc_usr_print ("Found URL query parameter => passwd=%s\n", param.password);
			}
			if (httpd_query_key_value(buf, "security", security_mode, sizeof(security_mode)) == ESP_OK) {
				nrc_usr_print ("Found URL query parameter => security=%s\n", security_mode);
				if (strcmp(security_mode, "open") == 0) {
					param.security_mode = WIFI_SEC_OPEN;
					memset(param.password, 0x0, sizeof(param.password));
				} else if (strcmp(security_mode, "wpa2") == 0) {
					param.security_mode = WIFI_SEC_WPA2;
				} else if (strcmp(security_mode, "wpa3-owe") == 0) {
					param.security_mode = WIFI_SEC_WPA3_OWE;
                } else if (strcmp(security_mode, "wpa3-sae") == 0) {
					param.security_mode = WIFI_SEC_WPA3_SAE;
				}
			}
		}
		nrc_mem_free(buf);
	}

	/* Set some custom headers */
	httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
	httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

	/* Send response with custom headers and body set as the
	 * string passed in user context*/
	const char* resp_str = (const char*) req->user_ctx;
	httpd_resp_send(req, resp_str, strlen(resp_str));

	/* After sending the HTTP response the old HTTP request
	 * headers are lost. Check if HTTP request headers can be read now. */
	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
		nrc_usr_print ("Request headers lost\n");
	}


	if (nvs_set_str(nvs_handle, NVS_SSID, (char *) param.ssid) != NVS_OK) {
		return ESP_FAIL;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_SECURITY, param.security_mode) != NVS_OK) {
		return ESP_FAIL;
	}
	if ((param.security_mode == WIFI_SEC_WPA2) || (param.security_mode == WIFI_SEC_WPA3_SAE)) {
		if (nvs_set_str(nvs_handle, NVS_WIFI_PASSWORD, (char *) param.password) != NVS_OK) {
			return ESP_FAIL;
		}
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_CONFIGURED, 1) != NVS_OK) {
		return ESP_FAIL;
	}
	return ESP_OK;
}

static httpd_uri_t hello = {
	.uri       = "/hello",
	.method    = HTTP_GET,
	.handler   = input_get_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx  = (char *) complete_element
};

static esp_err_t reboot_page_handler(httpd_req_t *req)
{
	esp_err_t ret = httpd_resp_send(req, rebooting_element, strlen(rebooting_element));

	_delay_ms(3000);
	nrc_sw_reset();

	return ESP_OK;
}

static httpd_uri_t rebooting = {
	.uri       = "/reboot.html",
	.method    = HTTP_GET,
	.handler   = reboot_page_handler,
	.user_ctx  = NULL
};

static esp_err_t input_handler(httpd_req_t *req)
{
	/* Recv, Process and Send */
	return httpd_resp_send(req, input_element, strlen(input_element));
}

static httpd_uri_t input = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = input_handler,
	.user_ctx = NULL
};

/******************************************************************************
 * FunctionName : run_http_server
 * Description  : Start http server
 * Parameters   :
 * Returns	    : return httpd handler
 *******************************************************************************/
static httpd_handle_t run_http_server()
{
	httpd_handle_t handle;
	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&handle, &conf) == ESP_OK) {
		nrc_usr_print("\033[31m[httpd] server on port : %d\033[39m\n", conf.server_port);
		return handle;
	}
	return NULL;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
WIFI_CONFIG param;

void user_init(void)
{
	httpd_handle_t server = NULL;
	uint8_t configured = 0;

	nrc_uart_console_enable(true);

	/* Open nvram */
	/* Note that nvs_init should have already called, and it is done in system start up. */
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed.\n", __func__);
		return;
	}

	memset(&param, 0x0, WIFI_CONFIG_SIZE);

	do {
		if ((nvs_get_u8(nvs_handle, NVS_WIFI_CONFIGURED, &configured) != NVS_OK)
			|| (configured == 0)) {
			nrc_usr_print("\033[31m[httpd] Setting SoftAP config...\033[39m");
			nrc_wifi_set_config(&param);
			strcpy((char *)param.ssid, AP_SSID);
			nrc_usr_print("\033[31m[httpd] Starting SoftAP...\033[39m\n");
			start_softap(&param);

			nrc_usr_print("\033[31m [%s] AP started...\033[39m\n",__func__);
			nrc_usr_print("\033[31m [%s] \tSSID : %s\033[39m\n",__func__, AP_SSID);
			nrc_usr_print("\033[31m [%s] \tSecurity : %d\033[39m\n",__func__, param.security_mode);
			if ((param.security_mode != WIFI_SEC_OPEN) && (param.security_mode != WIFI_SEC_WPA3_OWE)) {
				nrc_usr_print("\033[31m [%s] \tPassword : %s\033[39m\n",__func__, param.password);
			}
			break;
		} else {
			if (connect_to_ap(&param) != NRC_SUCCESS) {
				nvs_erase_key(nvs_handle, NVS_WIFI_CONFIGURED);
				nvs_erase_key(nvs_handle, NVS_SSID);
				nvs_erase_key(nvs_handle, NVS_WIFI_SECURITY);
				nvs_erase_key(nvs_handle, NVS_WIFI_PASSWORD);
				continue;
			} else {
				break;
			}
		}
	} while(1);

	/* start HTTP server */
	nrc_usr_print("\033[31m[httpd] Starting http server...\033[39m\n");
	server = run_http_server();

	if (server) {
		httpd_register_uri_handler(server, &input);
		httpd_register_uri_handler(server, &hello);
		httpd_register_uri_handler(server, &rebooting);
	}

	if (!configured) {
		nrc_usr_print("\033[31m [%s] Connect to %s and browse to access http server\033[39m\n",__func__, AP_SSID);
	}
}
