/*
 * sslhelper.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: andrei
 */

#include "sslhelper.h"
#if defined(_WIN32) || defined(_WIN64)
#endif
#include <stdint.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

#define BITS 1024
#define SERIAL 1
#define DAYS	10 * 365

/**
 * Read PrivateKey from the PEM string. After using free key and PEM
 * Usage:
 * pk = getPKey(pemstring, &pem)
 * 	if (pk != NULL)
 *		EVP_PKEY_free(pk);
 *	if (pem != NULL)
 *		BIO_free(pem);
 *
 */
EVP_PKEY *getPKey(
	const std::string &pemkey
)
{
	BIO *pem = BIO_new_mem_buf((void *) pemkey.c_str(), (int) pemkey.length());
	if (pem == NULL)
		return NULL;
	return PEM_read_bio_PrivateKey(pem, NULL, NULL, (void *) "");
}

/**
 * Read PrivateKey from the P12cfile. After using free key and PEM
 * Usage:
 * pk = p12ReadPKey(fn)
 * 	if (pk != NULL)
 *		EVP_PKEY_free(pk);
 *	if (pem != NULL)
 *		BIO_free(pem);
 *
 */
EVP_PKEY *p12ReadPKey(
	const std::string &p12file,
	const char *password
)
{
	FILE *p12_file;
	PKCS12 *p12_cert = NULL;
	EVP_PKEY *pkey = NULL;
	X509 *x509_cert = NULL;
	STACK_OF(X509) *additional_certs = NULL;

	p12_file = fopen(p12file.c_str(), "rb");
	d2i_PKCS12_fp(p12_file, &p12_cert);
	fclose(p12_file);

	PKCS12_parse(p12_cert, password, &pkey, &x509_cert, &additional_certs);
	//PKCS12_free(p12_cert);
	return pkey;
}


/**
 *	Load X509_NAME from PEM certificate at position.
 *	Position 0..N or -1 (last certificate)
 * Usage:
 * x509name = getCertificateName(pemstring, 0, &pem)
 * 	if (x509name != NULL)
 *		X509_NAME_free(x509name);
 *
 */
X509_NAME *getCertificateCN(
	const std::string &pemCertificate,
	int position
)
{
	BIO *pem = BIO_new_mem_buf((void *)pemCertificate.c_str(), (int) pemCertificate.length());
	if (pem == NULL)
		return NULL;
	int p = 0;
	X509 *root = NULL;
	if (position >= 0)
	{
		while (true)
		{
			root = PEM_read_bio_X509_AUX(pem, NULL, NULL, (void *) "");
			if (root == NULL)
			{
				// We're at the end of stream.
				ERR_clear_error();
				break;
			}
			if (p >= position)
				break;
			X509_free(root);
			root = NULL;
			p++;
		}
	}
	else
	{
		// get last
		while (true)
		{
			X509 *r = PEM_read_bio_X509_AUX(pem, NULL, NULL, (void*) "");
			if (r == NULL)
			{
				// We're at the end of stream.
				ERR_clear_error();
				break;
			}
			if (root)	// remove previous
				X509_free(root);
			root = r;
		}
	}

	if (root == NULL)
	{
		// nothing found
		BIO_free(pem);
		return NULL;
	}

	X509_NAME *rootName = X509_get_subject_name(root);
	if (rootName == NULL)
	{
		// Could not get name from root certificate.
		X509_free(root);
		BIO_free(pem);
		return NULL;
	}

	rootName = X509_NAME_dup(rootName);
	X509_free(root);
	BIO_free(pem);
	return rootName;
}

/**
 * Extract certificate common name as long int
 */
uint64_t getCertificateCNAsInt(const std::string &pem)
{
	uint64_t r = 0;
	X509_NAME *x509name = getCertificateCN(pem, -1);
	if (x509name != NULL)
	{
		char *cn0 = X509_NAME_oneline(x509name, 0, 0);

		if (cn0)
		{
			r = strtoul(cn0, NULL, 0);
			OPENSSL_free(cn0);
		}

		X509_NAME_free(x509name);
	}
	return r;
}

/*
 * Add extension using V3 code: we can set the config file as NULL because we
 * wont reference any other sections.
 */

bool add_ext(X509 *cert, int nid, char *value)
{
	X509_EXTENSION *ex;
	X509V3_CTX ctx;
	X509V3_set_ctx_nodb(&ctx);
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
	if (!ex)
		return false;

	X509_add_ext(cert, ex, -1);
	X509_EXTENSION_free(ex);
	return true;
}

#define CERT_ERR    { X509_free(x509); EVP_PKEY_free(pk); return false; }

/**
 * Encodes a binary safe base 64 string in one line
 * @param buffer
 * @param length
 * @return
 */
std::string Base64Encode
(
	const unsigned char* buffer,
	size_t length
)
{
	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Ignore newlines - write everything in one line
	BIO_write(bio, buffer, length);
	BIO_flush(bio);

	BUF_MEM *bufferPtr;
	BIO_get_mem_ptr(bio, &bufferPtr);
	BIO_set_close(bio, BIO_NOCLOSE);

	std::string r((*bufferPtr).data, bufferPtr->length);
	BIO_free_all(bio);

	return r;
}
