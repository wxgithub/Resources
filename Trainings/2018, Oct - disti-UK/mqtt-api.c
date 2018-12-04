/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/**
 * @file http-api.c
 */

#include <stdio.h>
#include <string.h>
#include <shell/tash.h>

#include <artik_module.h>
#include <artik_mqtt.h>

#include <artik_error.h>
#include <artik_gpio.h>

#include "command.h"

static artik_mqtt_handle g_handle;
static artik_mqtt_module *g_mqtt;

static const char root_ca[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n"
	"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
	"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
	"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
	"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
	"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n"
	"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n"
	"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n"
	"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n"
	"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n"
	"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n"
	"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n"
	"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n"
	"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n"
	"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n"
	"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n"
	"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n"
	"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n"
	"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n"
	"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n"
	"-----END CERTIFICATE-----\r\n";

static int mqtt_cmd_connect(int argc, char **argv);
static int mqtt_cmd_disconnect(int argc, char **argv);
static int mqtt_cmd_publish(int argc, char **argv);
static int mqtt_cmd_subscribe(int argc, char **argv);
static int mqtt_cmd_unsubscribe(int argc, char **argv);

const struct command mqtt_commands[] = {
	{ "connect", "connect <host> <port> <user> <password>", mqtt_cmd_connect },
	{ "disconnect", "disconnect", mqtt_cmd_disconnect },
	{ "publish", "publish <topic> <message>", mqtt_cmd_publish },
	{ "subscribe", "subscribe <topic>", mqtt_cmd_subscribe },
	{ "unsubscribe", "unsubscribe <topic>", mqtt_cmd_unsubscribe },
	{ "", "", NULL }
};

static void on_connect(artik_mqtt_config *client_config, void *data_user,
		int err)
{
	artik_error ret = S_OK;

	fprintf(stdout, "MQTT connection result: %s\n", error_msg(err));
	if (err != S_OK) {
		/* If connection failed, disconnect the client */
		mqtt_cmd_disconnect(0, NULL);
	}

	if (!g_handle) {
		fprintf(stderr, "MQTT connection is not active\n");
		return;
	}

}

static void on_disconnect(artik_mqtt_config *client_config, void *data_user,
		int err)
{
	fprintf(stdout, "MQTT disconnection result: %s\n", error_msg(err));
}

static void on_subscribe(artik_mqtt_config *client_config, void *data_user,
		int mid, int qos_count, const int *granted_qos)
{
	fprintf(stdout, "MQTT subscription OK\n");
}

static void on_unsubscribe(artik_mqtt_config *client_config, void *data_user,
		int mid)
{
	fprintf(stdout, "MQTT unsubscription OK\n");
}

static void on_publish(artik_mqtt_config *client_config, void *data_user,
		int mid)
{
	fprintf(stdout, "MQTT message published: %d\n", mid);
}

static void on_message(artik_mqtt_config *client_config, void *data_user,
		artik_mqtt_msg *msg)
{
	fprintf(stdout, "MQTT message received on topic %s: %s\n", msg->topic,
			msg->payload);
}


static int gpio_io(artik_gpio_dir_t dir, artik_gpio_id id, int new_value)
{
	artik_gpio_module *gpio;
	artik_gpio_config config;
	artik_gpio_handle handle;
	char name[16] = "";
	int ret = 0;

	gpio = (artik_gpio_module *)artik_request_api_module("gpio");
	if (!gpio) {
		fprintf(stderr, "GPIO module is not available\n");
		return -1;
	}

	memset(&config, 0, sizeof(config));
	config.dir = dir;
	config.id = id;
	snprintf(name, 16, "gpio%d", config.id);
	config.name = name;

	if (gpio->request(&handle, &config) != S_OK) {
		fprintf(stderr, "Failed to request GPIO %d\n", config.id);
		ret = -1;
		goto exit;
	}

	if (dir == GPIO_OUT) {
		ret = gpio->write(handle, new_value);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to write GPIO %d [err %d]\n", config.id, ret);
		} else {
			fprintf(stdout, "Write %d to GPIO %d\n", new_value, config.id);
		}
	} else {
		ret = gpio->read(handle);
		if (ret < 0) {
			fprintf(stderr, "Failed to read GPIO %d [err %d]\n", config.id, ret);
		} else {
			fprintf(stdout, "The value read in GPIO %d is %d\n", config.id, ret);
		}
	}

	gpio->release(handle);

exit:
	artik_release_api_module(gpio);
	return ret;
}

static int mqtt_cmd_connect(int argc, char **argv)
{
	artik_mqtt_config config;
	artik_error ret = S_OK;
	artik_ssl_config ssl;
	int port = 0;
	bool use_tls = false;
	char *host = NULL;

	if (g_handle) {
		fprintf(stderr, "The MQTT connection is already active, disconnect first\n");
		return -1;
	}

	g_mqtt = (artik_mqtt_module *)artik_request_api_module("mqtt");
	if (!g_mqtt) {
		fprintf(stderr, "MQTT module is not available\n");
		return -1;
	}

	host = argv[3];

	memset(&config, 0, sizeof(config));
	config.clean_session = true;
	config.user_name = NULL;
	config.pwd = NULL;
	config.keep_alive_time = 60 * 1000;

	if (use_tls) {
		memset(&ssl, 0, sizeof(ssl));
		ssl.verify_cert = ARTIK_SSL_VERIFY_REQUIRED;
		ssl.ca_cert.data = (char *)root_ca;
		ssl.ca_cert.len = sizeof(root_ca);
		config.tls = &ssl;
	}

	StartWifiConnection();

	ret = g_mqtt->create_client(&g_handle, &config);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to create MQTT client\n");
		artik_release_api_module(g_mqtt);
		return -1;
	}

	g_mqtt->set_connect(g_handle, on_connect, NULL);
	g_mqtt->set_disconnect(g_handle, on_disconnect, NULL);
	g_mqtt->set_subscribe(g_handle, on_subscribe, NULL);
	g_mqtt->set_unsubscribe(g_handle, on_unsubscribe, NULL);
	g_mqtt->set_publish(g_handle, on_publish, NULL);
	g_mqtt->set_message(g_handle, on_message, NULL);

	port = 1883;

	ret = g_mqtt->connect(g_handle, host, port);
	if (ret != S_OK) {
		g_mqtt->destroy_client(g_handle);
		g_handle = NULL;
		fprintf(stderr, "Failed to initiate MQTT connection\n");
		artik_release_api_module(g_mqtt);
		return -1;
	}

	return 0;
}

int mqtt_cmd_disconnect(int argc, char **argv)
{
	artik_error ret = S_OK;

	if (!g_handle) {
		fprintf(stderr, "MQTT connection is not active\n");
		return -1;
	}

	ret = g_mqtt->disconnect(g_handle);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to close MQTT connection\n");
		return -1;
	}

	ret = g_mqtt->destroy_client(g_handle);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to clean MQTT client\n");
		return -1;
	}

	artik_release_api_module(g_mqtt);
	g_handle = NULL;

	return 0;
}

 int mqtt_cmd_publish(int argc, char **argv)
{
	artik_error ret = S_OK;
	int pin = 44;

	if (!g_handle) {
		fprintf(stderr, "MQTT connection is not active\n");
		return -1;
	}

	while (true) {
		int value = gpio_io(GPIO_IN, pin, 0);
		char adc_data[4];
		char message[100] = "";
		char topic[10] = "Machine";
		sprintf(adc_data, "%d", value);

		printf("MQTT: publish %d on %s\n", value, topic);

		ret = g_mqtt->publish(g_handle, 0, false, topic, strlen(adc_data), adc_data);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to publish message\n");
			return -1;
		}
		sleep(5);
	}

	return 0;
}

int mqtt_cmd_subscribe(int argc, char **argv)
{
	artik_error ret = S_OK;

	if (argc < 4) {
		fprintf(stderr, "Wrong arguments\n");
		usage(argv[1], mqtt_commands);
		return -1;
	}

	if (!g_handle) {
		fprintf(stderr, "MQTT connection is not active\n");
		return -1;
	}

	fprintf(stdout, "MQTT: subscribe to %s\n", argv[3]);

	ret = g_mqtt->subscribe(g_handle, 0, argv[3]);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to subscribe to topic %s\n", argv[3]);
		return -1;
	}

	return 0;
}

int mqtt_cmd_unsubscribe(int argc, char **argv)
{
	artik_error ret = S_OK;

	if (argc < 4) {
		fprintf(stderr, "Wrong arguments\n");
		usage(argv[1], mqtt_commands);
		return -1;
	}

	if (!g_handle) {
		fprintf(stderr, "MQTT connection is not active\n");
		return -1;
	}

	fprintf(stdout, "MQTT: unsubscribe from %s\n", argv[3]);

	ret = g_mqtt->unsubscribe(g_handle, argv[3]);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to unsubscribe from topic %s\n", argv[3]);
		return -1;
	}

	return 0;
}

int mqtt_main(int argc, char **argv)
{
	return commands_parser(argc, argv, mqtt_commands);
}

#ifdef CONFIG_EXAMPLES_ARTIK_MQTT
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int artik_mqtt_main(int argc, char *argv[])
#endif
{
	StartWifiConnection();
	return 0;
}
#endif
