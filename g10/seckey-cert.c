/* seckey-cert.c -  secret key certifucate packet handling
 *	Copyright (C) 1998 Free Software Foundation, Inc.
 *
 * This file is part of GNUPG.
 *
 * GNUPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "memory.h"
#include "packet.h"
#include "mpi.h"
#include "keydb.h"
#include "cipher.h"
#include "main.h"
#include "options.h"


static int
do_check( PKT_secret_cert *cert )
{
    byte *buffer;
    u16 csum=0;
    int res;
    unsigned nbytes;

    if( cert->is_protected ) { /* remove the protection */
	DEK *dek = NULL;
	u32 keyid[2];
	CIPHER_HANDLE cipher_hd=NULL;
	PKT_secret_cert *save_cert;
	char save_iv[8];

	switch( cert->protect.algo ) {
	  case CIPHER_ALGO_NONE: BUG(); break;
	  case CIPHER_ALGO_BLOWFISH:
	  case CIPHER_ALGO_CAST:
	    keyid_from_skc( cert, keyid );
	    if( cert->protect.s2k == 1 || cert->protect.s2k == 3 )
		dek = get_passphrase_hash( keyid, NULL,
						 cert->protect.salt );
	    else
		dek = get_passphrase_hash( keyid, NULL, NULL );

	    cipher_hd = cipher_open( cert->protect.algo,
				     CIPHER_MODE_AUTO_CFB, 1);
	    cipher_setkey( cipher_hd, dek->key, dek->keylen );
	    cipher_setiv( cipher_hd, NULL );
	    m_free(dek); /* pw is in secure memory, so m_free() burns it */
	    save_cert = copy_secret_cert( NULL, cert );
	    memcpy(save_iv, cert->protect.iv, 8 );
	    cipher_decrypt( cipher_hd, cert->protect.iv, cert->protect.iv, 8 );
	    switch( cert->pubkey_algo ) {
	      case PUBKEY_ALGO_ELGAMAL:
		buffer = mpi_get_secure_buffer( cert->d.elg.x, &nbytes, NULL );
		cipher_decrypt( cipher_hd, buffer, buffer, nbytes );
		mpi_set_buffer( cert->d.elg.x, buffer, nbytes, 0 );
		csum = checksum_mpi( cert->d.elg.x );
		m_free( buffer );
		break;
	      case PUBKEY_ALGO_DSA:
		buffer = mpi_get_secure_buffer( cert->d.dsa.x, &nbytes, NULL );
		cipher_decrypt( cipher_hd, buffer, buffer, nbytes );
		mpi_set_buffer( cert->d.dsa.x, buffer, nbytes, 0 );
		csum = checksum_mpi( cert->d.dsa.x );
		m_free( buffer );
		break;
	    #ifdef HAVE_RSA_CIPHER
	      case PUBKEY_ALGO_RSA:
		csum = 0;
		#define X(a) do { \
		    buffer = mpi_get_secure_buffer( cert->d.rsa.##a,	 \
						    &nbytes, NULL );	 \
		    csum += checksum_u16( nbytes*8 );			 \
		    cipher_decrypt( cipher_hd, buffer, buffer, nbytes ); \
		    csum += checksum( buffer, nbytes ); 		 \
		    mpi_set_buffer(cert->d.rsa.##a, buffer, nbytes, 0 ); \
		    m_free( buffer );					 \
		   } while(0)
		X(d);
		X(p);
		X(q);
		X(u);
		#undef X
		break;
	    #endif /* HAVE_RSA_CIPHER */

	      default: BUG();
	    }
	    cipher_close( cipher_hd );
	    /* now let's see wether we have used the right passphrase */
	    if( csum != cert->csum ) {
		if( cert->pubkey_algo == PUBKEY_ALGO_ELGAMAL ) {
		    /* very bad kludge to work around an early bug */
		    csum -= checksum_u16( mpi_get_nbits(cert->d.elg.x) );
		    nbytes = mpi_get_nlimbs(cert->d.elg.x) * 4;
		    csum += checksum_u16( nbytes*8 );
		    if( !opt.batch && csum == cert->csum )
			log_info("Probably you have an old key - use "
			     "\"--change-passphrase\" to convert.\n");
		}
		if( csum != cert->csum ) {
		    copy_secret_cert( cert, save_cert );
		    free_secret_cert( save_cert );
		    memcpy( cert->protect.iv, save_iv, 8 );
		    return G10ERR_BAD_PASS;
		}
	    }

	    switch( cert->pubkey_algo ) {
	      case PUBKEY_ALGO_ELGAMAL:
		res = elg_check_secret_key( &cert->d.elg );
		break;
	      case PUBKEY_ALGO_DSA:
		res = dsa_check_secret_key( &cert->d.dsa );
		break;
	    #ifdef HAVE_RSA_CIPHER
	      case PUBKEY_ALGO_RSA:
		res = rsa_check_secret_key( &cert->d.rsa );
		break;
	    #endif
	      default: BUG();
	    }
	    if( !res ) {
		copy_secret_cert( cert, save_cert );
		free_secret_cert( save_cert );
		memcpy( cert->protect.iv, save_iv, 8 );
		return G10ERR_BAD_PASS;
	    }
	    free_secret_cert( save_cert );
	    cert->is_protected = 0;
	    break;

	  default:
	    return G10ERR_CIPHER_ALGO; /* unsupported protection algorithm */
	}
    }
    else { /* not protected */
	switch( cert->pubkey_algo ) {
	  case PUBKEY_ALGO_ELGAMAL:
	    csum = checksum_mpi( cert->d.elg.x );
	    break;
	  case PUBKEY_ALGO_DSA:
	    csum = checksum_mpi( cert->d.dsa.x );
	    break;
	#ifdef HAVE_RSA_CIPHER
	  case PUBKEY_ALGO_RSA:
	    csum =0;
	    buffer = mpi_get_buffer( cert->d.rsa.rsa_d, &nbytes, NULL );
	    csum += checksum_u16( nbytes*8 );
	    csum += checksum( buffer, nbytes );
	    m_free( buffer );
	    buffer = mpi_get_buffer( cert->d.rsa.rsa_p, &nbytes, NULL );
	    csum += checksum_u16( nbytes*8 );
	    csum += checksum( buffer, nbytes );
	    m_free( buffer );
	    buffer = mpi_get_buffer( cert->d.rsa.rsa_q, &nbytes, NULL );
	    csum += checksum_u16( nbytes*8 );
	    csum += checksum( buffer, nbytes );
	    m_free( buffer );
	    buffer = mpi_get_buffer( cert->d.rsa.rsa_u, &nbytes, NULL );
	    csum += checksum_u16( nbytes*8 );
	    csum += checksum( buffer, nbytes );
	    m_free( buffer );
	    break;
	#endif
	  default: BUG();
	}
	if( csum != cert->csum ) {
	    if( cert->pubkey_algo == PUBKEY_ALGO_ELGAMAL ) {
		/* very bad kludge to work around an early bug */
		csum -= checksum_u16( mpi_get_nbits(cert->d.elg.x) );
		nbytes = mpi_get_nlimbs(cert->d.elg.x) * 4;
		csum += checksum_u16( nbytes*8 );
		if( !opt.batch && csum == cert->csum )
		    log_info("Probably you have an old key - use "
			 "\"--change-passphrase\" to convert.\n");
	    }
	    if( csum != cert->csum )
		return G10ERR_CHECKSUM;
	}
    }

    return 0;
}



/****************
 * Check the secret key certificate
 * Ask up to 3 time for a correct passphrase
 */
int
check_secret_key( PKT_secret_cert *cert )
{
    int rc = G10ERR_BAD_PASS;
    int i;

    for(i=0; i < 3 && rc == G10ERR_BAD_PASS; i++ ) {
	if( i )
	    log_error("Invalid passphrase; please try again ...\n");
	switch( cert->pubkey_algo ) {
	  case PUBKEY_ALGO_ELGAMAL:
	  case PUBKEY_ALGO_DSA:
	    rc = do_check( cert );
	    break;
	  default: rc = G10ERR_PUBKEY_ALGO;
	}
	if( get_passphrase_fd() != -1 )
	    break;
    }
    return rc;
}

/****************
 * check wether the secret key is protected.
 * Returns: 0 not protected, -1 on error or the protection algorithm
 */
int
is_secret_key_protected( PKT_secret_cert *cert )
{
    return cert->is_protected? cert->protect.algo : 0;
}


static int
do_protect( void (*fnc)(CIPHER_HANDLE, byte *, byte *, unsigned),
	    CIPHER_HANDLE fnc_hd, PKT_secret_cert *cert )
{
    byte *buffer;
    unsigned nbytes;

    switch( cert->pubkey_algo ) {
      case PUBKEY_ALGO_ELGAMAL:
	/* recalculate the checksum, so that --change-passphrase
	 * can be used to convert from the faulty to the correct one
	 * wk 06.04.98:
	 * fixme: remove this some time in the future.
	 */
	cert->csum = checksum_mpi( cert->d.elg.x );
	buffer = mpi_get_buffer( cert->d.elg.x, &nbytes, NULL );
	(*fnc)( fnc_hd, buffer, buffer, nbytes );
	mpi_set_buffer( cert->d.elg.x, buffer, nbytes, 0 );
	m_free( buffer );
	break;

      case PUBKEY_ALGO_DSA:
	buffer = mpi_get_buffer( cert->d.dsa.x, &nbytes, NULL );
	(*fnc)( fnc_hd, buffer, buffer, nbytes );
	mpi_set_buffer( cert->d.dsa.x, buffer, nbytes, 0 );
	m_free( buffer );
	break;

      default: return G10ERR_PUBKEY_ALGO;
    }
    return 0;
}


/****************
 * Protect the secret key certificate with the passphrase from DEK
 */
int
protect_secret_key( PKT_secret_cert *cert, DEK *dek )
{
    int rc=0;

    if( !dek )
	return 0;

    if( !cert->is_protected ) { /* okay, apply the protection */
	CIPHER_HANDLE cipher_hd=NULL;

	switch( cert->protect.algo ) {
	  case CIPHER_ALGO_NONE: BUG(); break;
	  case CIPHER_ALGO_BLOWFISH:
	  case CIPHER_ALGO_CAST:
	    cipher_hd = cipher_open( cert->protect.algo,
				     CIPHER_MODE_AUTO_CFB, 1 );
	    cipher_setkey( cipher_hd, dek->key, dek->keylen );
	    cipher_setiv( cipher_hd, NULL );
	    cipher_encrypt( cipher_hd, cert->protect.iv, cert->protect.iv, 8 );
	    if( !do_protect( &cipher_encrypt, cipher_hd, cert ) )
		cert->is_protected = 1;
	    cipher_close( cipher_hd );
	    break;

	  default:
	    rc = G10ERR_CIPHER_ALGO; /* unsupport protection algorithm */
	    break;
	}
    }
    return rc;
}

