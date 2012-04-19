/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdlib.h>
#include <ipxe/crypto.h>
#include <ipxe/sha256.h>
#include <ipxe/x509.h>
#include <ipxe/settings.h>
#include <ipxe/dhcp.h>
#include <ipxe/init.h>
#include <ipxe/rootcert.h>

/** @file
 *
 * Root certificate store
 *
 */

/** Length of a root certificate fingerprint */
#define FINGERPRINT_LEN SHA256_DIGEST_SIZE

/* Use iPXE root CA if no trusted certificates are explicitly specified */
#ifndef TRUSTED
#define TRUSTED								\
	/* iPXE root CA */						\
	0x9f, 0xaf, 0x71, 0x7b, 0x7f, 0x8c, 0xa2, 0xf9, 0x3c, 0x25,	\
	0x6c, 0x79, 0xf8, 0xac, 0x55, 0x91, 0x89, 0x5d, 0x66, 0xd1,	\
	0xff, 0x3b, 0xee, 0x63, 0x97, 0xa7, 0x0d, 0x29, 0xc6, 0x5e,	\
	0xed, 0x1a,
#endif

/** Root certificate fingerprints */
static const uint8_t fingerprints[] = { TRUSTED };

/** Root certificate fingerprint setting */
struct setting trust_setting __setting ( SETTING_CRYPTO ) = {
	.name = "trust",
	.description = "Trusted root certificate fingerprint",
	.tag = DHCP_EB_TRUST,
	.type = &setting_type_hex,
};

/** Root certificates */
struct x509_root root_certificates = {
	.digest = &sha256_algorithm,
	.count = ( sizeof ( fingerprints ) / FINGERPRINT_LEN ),
	.fingerprints = fingerprints,
};

/**
 * Initialise root certificate
 *
 * We allow the list of trusted root certificate fingerprints to be
 * overridden using the "trust" setting, but only at the point of iPXE
 * initialisation.  This prevents untrusted sources of settings
 * (e.g. DHCP) from subverting the chain of trust, while allowing
 * trustworthy sources (e.g. VMware GuestInfo or non-volatile stored
 * options) to change the trusted root certificate without requiring a
 * rebuild.
 */
static void rootcert_init ( void ) {
	void *external;
	int len;
	int rc;

	/* Fetch copy of "trust" setting, if it exists.  This memory
	 * will never be freed.
	 */
	len = fetch_setting_copy ( NULL, &trust_setting, &external );
	if ( len < 0 ) {
		rc = len;
		DBGC ( &root_certificates, "ROOTCERT cannot fetch trusted "
		       "root certificate fingerprints: %s\n", strerror ( rc ) );
		/* No way to prevent startup; fail safe by trusting no
		 * certificates.
		 */
		root_certificates.count = 0;
		return;
	}

	/* Use certificates from "trust" setting, if present */
	if ( external ) {
		root_certificates.fingerprints = external;
		root_certificates.count = ( len / FINGERPRINT_LEN );
	}

	DBGC ( &root_certificates, "ROOTCERT using %d %s certificate(s):\n",
	       root_certificates.count, ( external ? "external" : "built-in" ));
	DBGC_HDA ( &root_certificates, 0, root_certificates.fingerprints,
		   ( root_certificates.count * FINGERPRINT_LEN ) );
}

/** Root certificate initialiser */
struct init_fn rootcert_init_fn __init_fn ( INIT_LATE ) = {
	.initialise = rootcert_init,
};
