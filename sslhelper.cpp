/*
 * sslhelper.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: andrei
 */

#include "sslhelper.h"
#ifdef _MSC_VER
#include <Windows.h>
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

#define CERT_ENTRY_COUNT	4
const char* CERT_ENTRY_NAMES [] {"C", "ST", "O", "OU"};
const char* CERT_ENTRY_VALUES [] {"RU", "Sakha", "ikfia.ysn.ru", "RD"};

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
	EVP_PKEY *pkey;
	X509 *x509_cert;
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

bool mkcert(
		int64_t id,
		X509_NAME *issuerName,
		EVP_PKEY *issuerPKey,
		std::string *retpkey,
		std::string *retcert
		)
{
	X509 *x509;
	EVP_PKEY *pk;
	X509_NAME *name = NULL;

	if ((pk = EVP_PKEY_new()) == NULL)
		return false;

	if ((x509 = X509_new()) == NULL)
	{
	    EVP_PKEY_free(pk);
		return false;
	}

	RSA *rsa = RSA_generate_key(BITS, RSA_F4, NULL, NULL);
	if (!EVP_PKEY_assign_RSA(pk, rsa))
	{
		CERT_ERR
	}

	X509_set_version(x509, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(x509), SERIAL);
	X509_gmtime_adj(X509_get_notBefore(x509), 0);
	X509_gmtime_adj(X509_get_notAfter(x509), (long) 60 * 60 * 24 * DAYS);
	X509_set_pubkey(x509, pk);

	name = X509_get_subject_name(x509);

	for (int i = 0; i < CERT_ENTRY_COUNT; i++)
		X509_NAME_add_entry_by_txt(name, CERT_ENTRY_NAMES[i], MBSTRING_ASC, (const unsigned char *) CERT_ENTRY_VALUES[i], -1, -1, 0);

	unsigned char sid[20];
	sprintf((char *) sid, "%ld", id);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, sid, -1, -1, 0);

	X509_set_issuer_name(x509, issuerName);

    add_ext(x509, NID_netscape_comment, (char *) sid);

	if (!X509_sign(x509, issuerPKey, EVP_sha1()))
		CERT_ERR

    // write key
	BIO *bio = BIO_new(BIO_s_mem());
	if (bio == NULL)
		CERT_ERR
	PEM_write_bio_PrivateKey(bio, pk, NULL, NULL, 0, NULL, NULL);
	char *b;
	long len = BIO_get_mem_data(bio, &b);
	*retpkey = std::string(b, len);
	BIO_free(bio);

	// write cert
	bio = BIO_new(BIO_s_mem());
	if (bio == NULL)
		CERT_ERR
	PEM_write_bio_X509_AUX(bio, x509);
	len = BIO_get_mem_data(bio, &b);
	*retcert = std::string(b, len);

    X509_free(x509);
    EVP_PKEY_free(pk);

	return true;
}

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
