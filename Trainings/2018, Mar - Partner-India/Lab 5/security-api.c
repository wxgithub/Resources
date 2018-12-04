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
/****************************************************************************
 * examples/security_test/security_test_main.c
 *
 *   Copyright (C) 2016 SAMSUNG ELECTRONICS CO., LTD. All rights reserved.
 *   Author: Jisuu Kim <jisuu.kim@samsung.com>
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tls/base64.h"
#include "tls/x509.h"
#include "tls/x509_crt.h"
#include "tls/timing.h"

#include <stdio.h>
#include <string.h>

#include "tls/see_api.h"
#include "command.h"
#define SEE_CERTNUM 1

static int security_factory_getcert(int argc, char *argv[]);
static int security_read_write_secure_storage(int argc, char *argv[]);

const struct command security_commands[] = {
	{ "factorycert", "Get factory certification", security_factory_getcert },
	{ "securestorage", "Read Write Secure Storage", security_read_write_secure_storage},
	{"", "", NULL}
};

/*
 * Definition for handling pthread
 */
#define SECURITY_TEST_PRIORITY     100
#define SECURITY_TEST_STACK_SIZE   51200
#define SECURITY_TEST_SCHED_POLICY SCHED_RR


struct pthread_arg {
	int argc;
	char **argv;
};

#define see_selfprintf printf

struct fac_info {
	int id;
	char *name;
};

int print_crt(int debug_mode, unsigned char *buf, unsigned int buf_len)
{
	int ret;
	int pos = 0;
	size_t len = 0;
	unsigned char *p;
	int buf_format = MBEDTLS_X509_FORMAT_DER;

	if (debug_mode != 1) {
		return 0;
	}

	mbedtls_x509_crt crt;
	mbedtls_x509_crt *t_crt;

	mbedtls_x509_crt_init(&crt);

	if (strstr((const char *)buf, "-----BEGIN CERTIFICATE-----") != NULL) {
		buf_format = MBEDTLS_X509_FORMAT_PEM;
	}

	if (buf_format == MBEDTLS_X509_FORMAT_DER) {

		p = (unsigned char *)buf;

		while (pos < buf_len) {
			p = (unsigned char *)buf + pos;
			ret = mbedtls_asn1_get_tag(&p, buf + buf_len, &len,
				MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
			if (ret != 0) {
				goto exit;
			}
			if (pos + len < buf_len) {
				ret = mbedtls_x509_crt_parse(&crt, buf + pos, len + 4);
				if (ret != 0) {
					goto exit;
				}
			}
			pos += len + 4;
		}
	} else {
		ret = mbedtls_x509_crt_parse(&crt, buf, buf_len);
		if (ret != 0) {
			goto exit;
		}
	}

	t_crt = &crt;

	while (t_crt != NULL) {
		ret = mbedtls_x509_crt_info((char *)buf, buf_len, "  - ", t_crt);
		if (ret <= 0) {
			goto exit;
		}
		printf("\n%s\n", buf);
		t_crt = t_crt->next;
	}

	return 0;

exit:
	mbedtls_x509_crt_free(&crt);
	return ret;
}

int security_factory_getcert(int argc, char **argv)
{
	int ret = 0, i = 0;
	struct fac_info cert[SEE_CERTNUM] = { {FACTORYKEY_ARTIK_CERT, "ARTIK CERT"} };

	unsigned char buf[4096];
	unsigned int buf_len = 4096;

	while (i < SEE_CERTNUM) {
		printf("[%d] %s ... ", i, cert[i].name);
		buf_len = 4096;

		ret = see_get_certificate(buf, &buf_len, cert[i].id, CERT_PEM);
		if (ret != 0) {
			printf("get failed : %d\n", ret);
			i++;
			continue;
		}
		ret = print_crt(1, buf, buf_len);

		if (ret) {
			printf("parse failed : %x\n", -ret);
			i++;
			continue;
		}
		i++;
		printf("success\n");
	}

	return 0;
}

int security_read_write_secure_storage(int argc, char **argv)
{
	unsigned char input[30]="Samsung ARTIK Partner Training";
	unsigned int len = sizeof(input);
	unsigned char output[30];

	printf(" Write to Secure Storage...\n");
	if (see_write_secure_storage(input, len, 0) != SEE_OK) {
		printf("Fail\n see_write_secure_storage\n");
		return 1;
	}
	printf("Write %s to secure storage data slot 0\n", input);

	printf(" Read to Secure Storage...\n");
	if (see_read_secure_storage(output, &len, 0) != SEE_OK) {
		printf("Fail\n see_read_secure_storage\n");
		return 1;
	}
	printf("Read secure storage data slot 0: %s\n", output);

	return 0;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int security_test_main(int argc, char *argv[])
#endif
{
	return commands_parser(argc, argv, security_commands);
}
