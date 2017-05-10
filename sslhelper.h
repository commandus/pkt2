/*
 * sslhelper.h
 *
 *  Created on: Apr 5, 2016
 *      Author: andrei
 */

#ifndef SSLHELPER_H_
#define SSLHELPER_H_

#include <string>
#include <inttypes.h>
#include <openssl/pem.h>

/**
 * Read PrivateKey from the PEM string. After using free key and PEM
 * Usage:
 * pk = getPKey(pemstring, &pem)
 * 	if (pk != NULL)
 *		EVP_PKEY_free(pk);
 *
 */
EVP_PKEY *getPKey(
	const std::string &pemkey
);

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
);


/**
 *	Load X509_NAME from PEM certificate at position.
 *	Position 0..N or -1 (last certificate)
 * Usage:
 * x509name = getCertificateName(pemstring, 0, &pem)
 * 	if (x509name != NULL)
 *		X509_NAME_free(x509name);
 */
X509_NAME *getCertificateCN(
	const std::string &pemCertificate,
	int position
);

/**
 * Extract certificate common name as long int
 */
uint64_t getCertificateCNAsInt(const std::string &pem);

// BUGBUG Seems like memory leaking
/**
 * https://gist.github.com/barrysteyn/7308212
 * Encodes a binary safe base 64 string in one line
 * @param buffer
 * @param length
 * @return
 */
std::string Base64Encode
(
	const unsigned char* buffer,
	size_t length
);

/**
 * If error occurred, retval contains error descrtiption
 * @param data
 * @param pemkey PEM private key
 * @param retval
 * @return
 */
int jws_sign
(
	const std::string &data,
	const std::string &pemkey,
	std::string &retval
);

#endif /* SSLHELPER_H_ */
