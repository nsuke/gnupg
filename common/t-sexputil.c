/* t-sexputil.c - Module test for sexputil.c
 *	Copyright (C) 2007 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

#define pass()  do { ; } while(0)
#define fail(a)  do { fprintf (stderr, "%s:%d: test %d failed\n",\
                               __FILE__,__LINE__, (a));          \
                     exit (1);                                   \
                   } while(0)


static void
test_hash_algo_from_sigval (void)
{
  int algo;
  /* A real world example.  */
  unsigned char example1_rsa_sha1[] =
    ("\x28\x37\x3A\x73\x69\x67\x2D\x76\x61\x6C\x28\x33\x3A\x72\x73\x61"
     "\x28\x31\x3A\x73\x31\x32\x38\x3A\x17\xD2\xE9\x5F\xB4\x24\xD4\x1E"
     "\x8C\xEE\x94\xDA\x41\x42\x1F\x26\x5E\xF4\x6D\xEC\x5B\xBD\x5B\x89"
     "\x7A\x69\x11\x43\xE9\xD2\x23\x21\x25\x64\xA6\xB0\x56\xEF\xB4\xE9"
     "\x06\xB2\x44\xF6\x80\x1E\xFF\x41\x23\xEB\xC9\xFA\xFD\x09\xBF\x9C"
     "\x8E\xCF\x7F\xC3\x7F\x3A\x40\x48\x89\xDC\xBA\xB7\xDB\x9E\xF1\xBA"
     "\x7C\x08\xEA\x74\x1D\x49\xE7\x65\xEF\x67\x79\xBC\x23\xD9\x49\xCD"
     "\x05\x99\xD3\xD8\xB7\x7B\xC7\x0E\xF2\xB3\x01\x48\x0F\xC8\xEB\x05"
     "\x7B\xFB\x61\xCC\x41\x04\x74\x6D\x33\x84\xB1\xE6\x6A\xD8\x0F\xBC"
     "\x27\xAC\x43\x45\xFA\x04\xD1\x22\x29\x29\x28\x34\x3A\x68\x61\x73"
     "\x68\x34\x3A\x73\x68\x61\x31\x29\x29");
  /* The same but without the hash algo. */
  unsigned char example1_rsa[] =
    ("\x28\x37\x3A\x73\x69\x67\x2D\x76\x61\x6C\x28\x33\x3A\x72\x73\x61"
     "\x28\x31\x3A\x73\x31\x32\x38\x3A\x17\xD2\xE9\x5F\xB4\x24\xD4\x1E"
     "\x8C\xEE\x94\xDA\x41\x42\x1F\x26\x5E\xF4\x6D\xEC\x5B\xBD\x5B\x89"
     "\x7A\x69\x11\x43\xE9\xD2\x23\x21\x25\x64\xA6\xB0\x56\xEF\xB4\xE9"
     "\x06\xB2\x44\xF6\x80\x1E\xFF\x41\x23\xEB\xC9\xFA\xFD\x09\xBF\x9C"
     "\x8E\xCF\x7F\xC3\x7F\x3A\x40\x48\x89\xDC\xBA\xB7\xDB\x9E\xF1\xBA"
     "\x7C\x08\xEA\x74\x1D\x49\xE7\x65\xEF\x67\x79\xBC\x23\xD9\x49\xCD"
     "\x05\x99\xD3\xD8\xB7\x7B\xC7\x0E\xF2\xB3\x01\x48\x0F\xC8\xEB\x05"
     "\x7B\xFB\x61\xCC\x41\x04\x74\x6D\x33\x84\xB1\xE6\x6A\xD8\x0F\xBC"
     "\x27\xAC\x43\x45\xFA\x04\xD1\x22\x29\x29\x29");

  algo = hash_algo_from_sigval (example1_rsa_sha1);
  if (algo != GCRY_MD_SHA1)
    fail (0);
  algo = hash_algo_from_sigval (example1_rsa);
  if (algo)
    fail (0);
}


static void
test_make_canon_sexp_from_rsa_pk (void)
{
  struct {
    unsigned char *m;
    size_t mlen;
    unsigned char *e;
    size_t elen;
    unsigned char *result;
    size_t resultlen;
    gpg_err_code_t reverr;  /* Expected error from the reverse function.  */
  } tests[] = {
    {
      "\x82\xB4\x12\x48\x08\x48\xC0\x76\xAA\x8E\xF1\xF8\x7F\x5E\x9B\x89"
      "\xA9\x62\x92\xA2\x16\x1B\xF5\x9F\xE1\x41\xF3\xF0\x42\xB5\x5C\x46"
      "\xB8\x83\x9F\x39\x97\x73\xFF\xC5\xB2\xF4\x59\x5F\xBA\xC7\x0E\x03"
      "\x9D\x27\xC0\x86\x37\x31\x46\xE0\xA1\xFE\xA1\x41\xD4\xE3\xE9\xB3"
      "\x9B\xD5\x84\x65\xA5\x37\x35\x34\x07\x58\xB6\xBA\x21\xCA\x21\x72"
      "\x4C\xF3\xFC\x91\x47\xD1\x3C\x1D\xA5\x9C\x38\x4D\x58\x39\x92\x16"
      "\xB1\xE5\x43\xFE\xB5\x46\x4B\x43\xD1\x47\xB0\xE8\x2A\xDB\xF8\x34"
      "\xB0\x5A\x22\x3D\x14\xBB\xEA\x63\x65\xA7\xF1\xF2\xF8\x97\x74\xA7",
      128,
      "\x40\x00\x00\x81",
      4,
      "\x28\x31\x30\x3a\x70\x75\x62\x6c\x69\x63\x2d\x6b\x65\x79\x28\x33"
      "\x3a\x72\x73\x61\x28\x31\x3a\x6e\x31\x32\x39\x3a\x00\x82\xb4\x12"
      "\x48\x08\x48\xc0\x76\xaa\x8e\xf1\xf8\x7f\x5e\x9b\x89\xa9\x62\x92"
      "\xa2\x16\x1b\xf5\x9f\xe1\x41\xf3\xf0\x42\xb5\x5c\x46\xb8\x83\x9f"
      "\x39\x97\x73\xff\xc5\xb2\xf4\x59\x5f\xba\xc7\x0e\x03\x9d\x27\xc0"
      "\x86\x37\x31\x46\xe0\xa1\xfe\xa1\x41\xd4\xe3\xe9\xb3\x9b\xd5\x84"
      "\x65\xa5\x37\x35\x34\x07\x58\xb6\xba\x21\xca\x21\x72\x4c\xf3\xfc"
      "\x91\x47\xd1\x3c\x1d\xa5\x9c\x38\x4d\x58\x39\x92\x16\xb1\xe5\x43"
      "\xfe\xb5\x46\x4b\x43\xd1\x47\xb0\xe8\x2a\xdb\xf8\x34\xb0\x5a\x22"
      "\x3d\x14\xbb\xea\x63\x65\xa7\xf1\xf2\xf8\x97\x74\xa7\x29\x28\x31"
      "\x3a\x65\x34\x3a\x40\x00\x00\x81\x29\x29\x29",
      171
    },
    {
      "\x63\xB4\x12\x48\x08\x48\xC0\x76\xAA\x8E\xF1\xF8\x7F\x5E\x9B\x89",
      16,
      "\x03",
      1,
      "\x28\x31\x30\x3a\x70\x75\x62\x6c\x69\x63\x2d\x6b\x65\x79\x28\x33"
      "\x3a\x72\x73\x61\x28\x31\x3a\x6e\x31\x36\x3a\x63\xb4\x12\x48\x08"
      "\x48\xc0\x76\xaa\x8e\xf1\xf8\x7f\x5e\x9b\x89\x29\x28\x31\x3a\x65"
      "\x31\x3a\x03\x29\x29\x29",
      54,
    },
    {
      "",
      0,
      "",
      0,
      "\x28\x31\x30\x3a\x70\x75\x62\x6c\x69\x63\x2d\x6b\x65\x79\x28\x33"
      "\x3a\x72\x73\x61\x28\x31\x3a\x6e\x31\x3a\x00\x29\x28\x31\x3a\x65"
      "\x31\x3a\x00\x29\x29\x29",
      38,
      GPG_ERR_BAD_PUBKEY
    },
    {
      NULL
    }
  };
  int idx;
  gpg_error_t err;
  unsigned char *sexp;
  size_t length;
  const unsigned char *rsa_n, *rsa_e;
  size_t rsa_n_len, rsa_e_len;

  for (idx=0; tests[idx].m; idx++)
    {
      sexp = make_canon_sexp_from_rsa_pk (tests[idx].m, tests[idx].mlen,
                                          tests[idx].e, tests[idx].elen,
                                          &length);
      if (!sexp)
        {
          fprintf (stderr, "%s:%d: out of core\n", __FILE__, __LINE__);
          exit (1);
        }

      if (length != tests[idx].resultlen)
        fail (idx);
      if (memcmp (sexp, tests[idx].result, tests[idx].resultlen))
        fail (idx);

      /* Test the reverse function.  */
      err = get_rsa_pk_from_canon_sexp (sexp, length,
                                        &rsa_n, &rsa_n_len,
                                        &rsa_e, &rsa_e_len);
      if (gpg_err_code (err) != tests[idx].reverr)
        fail (idx);
      if (!err)
        {
          if (tests[idx].mlen != rsa_n_len)
            fail (idx);
          if (memcmp (tests[idx].m, rsa_n, rsa_n_len))
            fail (idx);
          if (tests[idx].elen != rsa_e_len)
            fail (idx);
          if (memcmp (tests[idx].e, rsa_e, rsa_e_len))
            fail (idx);
        }

      xfree (sexp);
    }
}



/* Communiacation object for tcmp.  */
struct tcmp_parm_s {
  int curve_seen;
};

/* Helper for test_cmp_canon_sexp.  */
static int
tcmp1 (void *opaque, int depth,
       const unsigned char *aval, size_t alen,
       const unsigned char *bval, size_t blen)
{
  struct tcmp_parm_s *parm = opaque;

  (void)depth;

  if (parm->curve_seen)
    {
      /* Last token was "curve", canonicalize its argument.  */
      parm->curve_seen = 0;

      if (alen == 8 && !memcmp (aval, "nistp256", alen))
        {
          alen = 19;
          aval = "1.2.840.10045.3.1.7";
        }

      if (blen == 8 && !memcmp (bval, "nistp256", blen))
        {
          blen = 19;
          bval = "1.2.840.10045.3.1.7";
        }
    }
  else if (alen == 5 && !memcmp (aval, "curve", 5))
    parm->curve_seen = 1;
  else
    parm->curve_seen = 0;

  if (alen > blen)
    return 1;
  else if (alen < blen)
    return -1;
  else
    return memcmp (aval, bval, alen);
}


static void
test_cmp_canon_sexp (void)
{
  struct {
    unsigned char *a;
    unsigned char *b;
    int expected0;  /* Expected result without compare function.    */
    int expected1;  /* Expected result with compare function tcmp1. */
  }
  tests[] = {
  {
   "(10:public-key(3:ecc(5:curve8:nistp256)(1:q10:qqqqqqqqqq)))",
   "(10:public-key(3:ecc(5:curve8:nistp256)(1:q10:qqqqqqqqqq)))",
   0, 0
  },
  {
   "(10:public-key(3:ecc(5:curve19:1.2.840.10045.3.1.7)(1:q10:qqqqqqqqqq)))",
   "(10:public-key(3:ecc(5:curve19:1.2.840.10045.3.1.7)(1:q10:qqqqqqqqqq)))",
   0, 0
  },
  {
   "(10:public-key(3:ecc(5:curve8:nistp256)(1:q10:qqqqqqqqqq)))",
   "(10:public-key(3:ecc(5:curve19:1.2.840.10045.3.1.7)(1:q10:qqqqqqqqqq)))",
   -1, 0
  },
  {
   "(10:public-key(3:ecc(5:curve19:1.2.840.10045.3.1.7)(1:q10:qqqqqqqqqq)))",
   "(10:public-key(3:ecc(5:curve8:nistp256)(1:q10:qqqqqqqqqq)))",
   1, 0
  },
  {
   NULL
  }
  };
  struct tcmp_parm_s parm = {0};
  int idx;
  int res;

  for (idx=0; tests[idx].a; idx++)
    {
      res = cmp_canon_sexp (tests[idx].a, strlen (tests[idx].a),
                            tests[idx].b, strlen (tests[idx].b),
                            NULL, NULL);
      if (res != tests[idx].expected0)
        fail (idx);
      res = cmp_canon_sexp (tests[idx].a, strlen (tests[idx].a),
                            tests[idx].b, strlen (tests[idx].b),
                            tcmp1, &parm);
      if (res != tests[idx].expected1)
        fail (idx);
    }
}


int
main (int argc, char **argv)
{
  (void)argc;
  (void)argv;

  test_hash_algo_from_sigval ();
  test_make_canon_sexp_from_rsa_pk ();
  test_cmp_canon_sexp ();

  return 0;
}
