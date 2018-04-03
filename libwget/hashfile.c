/*
 * Copyright(c) 2012 Tim Ruehsen
 * Copyright(c) 2015-2016 Free Software Foundation, Inc.
 *
 * This file is part of Wget.
 *
 * Wget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wget.  If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * Hash routines
 *
 * Changelog
 * 29.07.2012  Tim Ruehsen  created
 *
 */

#include <config.h>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef HAVE_MMAP
#	include <sys/mman.h>
#endif

#include <wget.h>
#include "private.h"

/**
 * \file
 * \brief Hashing functions
 * \ingroup libwget-hash
 * @{
 *
 */

/**
 * \param[in] hashname Name of the hashing algorithm (see table below)
 * \return A constant to be used by libwget hashing functions
 *
 * Get the hashing algorithms list item that corresponds to the named hashing algorithm.
 *
 * This function returns a constant that uniquely identifies a known supported hashing algorithm
 * within libwget. All the supported algorithms are listed in the ::wget_digest_algorithm_t enum.
 *
 * Algorithm name | Constant
 * -------------- | --------
 * sha1 or sha-1|WGET_DIGTYPE_SHA1
 * sha256 or sha-256|WGET_DIGTYPE_SHA256
 * sha512 or sha-512|WGET_DIGTYPE_SHA512
 * sha224 or sha-224|WGET_DIGTYPE_SHA224
 * sha384 or sha-384|WGET_DIGTYPE_SHA384
 * md5|WGET_DIGTYPE_MD5
 * md2|WGET_DIGTYPE_MD2
 * rmd160|WGET_DIGTYPE_RMD160
 */
wget_digest_algorithm_t wget_hash_get_algorithm(const char *hashname)
{
	if (hashname) {
		if (*hashname == 's' || *hashname == 'S') {
			if (!wget_strcasecmp_ascii(hashname, "sha-1") || !wget_strcasecmp_ascii(hashname, "sha1"))
				return WGET_DIGTYPE_SHA1;
			else if (!wget_strcasecmp_ascii(hashname, "sha-256") || !wget_strcasecmp_ascii(hashname, "sha256"))
				return WGET_DIGTYPE_SHA256;
			else if (!wget_strcasecmp_ascii(hashname, "sha-512") || !wget_strcasecmp_ascii(hashname, "sha512"))
				return WGET_DIGTYPE_SHA512;
			else if (!wget_strcasecmp_ascii(hashname, "sha-224") || !wget_strcasecmp_ascii(hashname, "sha224"))
				return WGET_DIGTYPE_SHA224;
			else if (!wget_strcasecmp_ascii(hashname, "sha-384") || !wget_strcasecmp_ascii(hashname, "sha384"))
				return WGET_DIGTYPE_SHA384;
		}
		else if (!wget_strcasecmp_ascii(hashname, "md5"))
			return WGET_DIGTYPE_MD5;
		else if (!wget_strcasecmp_ascii(hashname, "md2"))
			return WGET_DIGTYPE_MD2;
		else if (!wget_strcasecmp_ascii(hashname, "rmd160"))
			return WGET_DIGTYPE_RMD160;
	}

	error_printf(_("Unknown hash type '%s'\n"), hashname);
	return WGET_DIGTYPE_UNKNOWN;
}

#if defined WITH_GNUTLS && defined HAVE_GNUTLS_CRYPTO_H && !defined WITH_LIBNETTLE
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

struct _wget_hash_hd_st {
	gnutls_hash_hd_t
		dig;
};

static const gnutls_digest_algorithm_t
	_gnutls_algorithm[] = {
		[WGET_DIGTYPE_UNKNOWN] = GNUTLS_DIG_UNKNOWN,
		[WGET_DIGTYPE_MD2] = GNUTLS_DIG_MD2,
		[WGET_DIGTYPE_MD5] = GNUTLS_DIG_MD5,
		[WGET_DIGTYPE_RMD160] = GNUTLS_DIG_RMD160,
		[WGET_DIGTYPE_SHA1] = GNUTLS_DIG_SHA1,
		[WGET_DIGTYPE_SHA224] = GNUTLS_DIG_SHA224,
		[WGET_DIGTYPE_SHA256] = GNUTLS_DIG_SHA256,
		[WGET_DIGTYPE_SHA384] = GNUTLS_DIG_SHA384,
		[WGET_DIGTYPE_SHA512] = GNUTLS_DIG_SHA512
};

/**
 * \param[in] algorithm One of the hashing algorithms returned by wget_hash_get_algorithm()
 * \param[in] text Input data to hash
 * \param[in] textlen Length of the input data
 * \param[in] digest Caller-supplied buffer where the output hash will be placed
 * \return Zero on success or a negative value on error
 *
 * Convenience function to hash the given data in a single call.
 *
 * The caller must ensure that the provided output buffer \p digest is large enough
 * to store the hash. A particular hash algorithm is guaranteed to always generate
 * the same amount of data (e.g. 512 bits) but different hash algorithms will output
 * different lengths of data. To get the output length of the chosen algorithm \p algorithm,
 * call wget_hash_get_len().
 *
 * \note This function's behavior depends on the underlying cryptographic engine libwget was compiled with.
 */
int wget_hash_fast(wget_digest_algorithm_t algorithm, const void *text, size_t textlen, void *digest)
{
	if ((unsigned)algorithm < countof(_gnutls_algorithm))
		return gnutls_hash_fast(_gnutls_algorithm[algorithm], text, textlen, digest);
	else
		return -1;
}

/**
 * \param[in] algorithm One of the hashing algorithms returned by wget_hash_get_algorithm()
 * \return The length of the output data generated by the algorithm
 *
 * Determines the output length of the given hashing algorithm.
 *
 * A particular hash algorithm is guaranteed to always generate
 * the same amount of data (e.g. 512 bits) but different hash algorithms will output
 * different lengths of data.
 */
int wget_hash_get_len(wget_digest_algorithm_t algorithm)
{
	if ((unsigned)algorithm < countof(_gnutls_algorithm))
		return gnutls_hash_get_len(_gnutls_algorithm[algorithm]);
	else
		return 0;
}

/**
 * \param[out] handle Caller-provided pointer to a ::wget_hash_hd_t structure where the handle to this
 * hashing primitive will be stored, needed in subsequent calls to wget_hash()
 * \param[in] algorithm One of the hashing algorithms returned by wget_hash_get_algorithm()
 * \return Zero on success or a negative value on error
 *
 * Initialize the cryptographic engine to compute hashes with the given hashing algorithm,
 * as well as the hashing algorithm itself.
 *
 * After this function returns, wget_hash() might be called as many times as desired.
 */
int wget_hash_init(wget_hash_hd_t *handle, wget_digest_algorithm_t algorithm)
{
	if ((unsigned)algorithm < countof(_gnutls_algorithm))
		return gnutls_hash_init(&handle->dig, _gnutls_algorithm[algorithm]) == 0 ? 0 : -1;
	else
		return -1;
}

/**
 * \param[in] handle Handle to the hashing primitive returned by a subsequent call to wget_hash_init()
 * \param[in] text Input data
 * \param[in] textlen Length of the input data
 * \return Zero on success or a negative value on error
 *
 * Update the digest by adding additional input data to it. This method can be called
 * as many times as desired. Once finished, call wget_hash_deinit() to complete
 * the computation and get the resulting hash.
 */
int wget_hash(wget_hash_hd_t *handle, const void *text, size_t textlen)
{
	return gnutls_hash(handle->dig, text, textlen) == 0 ? 0 : -1;
}

/**
 * \param[in] handle Handle to the hashing primitive returned by a subsequent call to wget_hash_init()
 * \param[out] digest Caller-supplied buffer where the output hash will be placed.
 *
 * Complete the hash computation by performing final operations, such as padding,
 * and obtain the final result. The result will be placed in the caller-supplied
 * buffer \p digest. The caller must ensure that the provided output buffer \p digest
 * is large enough to store the hash. To get the output length of the chosen algorithm
 * \p algorithm, call wget_hash_get_len().
 */
void wget_hash_deinit(wget_hash_hd_t *handle, void *digest)
{
	gnutls_hash_deinit(handle->dig, digest);
}

#elif defined WITH_LIBNETTLE
#include <nettle/nettle-meta.h>
#include <nettle/md2.h>
#include <nettle/md5.h>
#include <nettle/ripemd160.h>
#include <nettle/sha2.h>

struct _wget_hash_hd_st {
	const struct nettle_hash
		*hash;
	void
		*context;
};

static const struct nettle_hash *
	_nettle_algorithm[] = {
		[WGET_DIGTYPE_UNKNOWN] = NULL,
		[WGET_DIGTYPE_MD2] = &nettle_md2,
		[WGET_DIGTYPE_MD5] = &nettle_md5,
#ifdef RIPEMD160_DIGEST_SIZE
		[WGET_DIGTYPE_RMD160] = &nettle_ripemd160,
#endif
		[WGET_DIGTYPE_SHA1] = &nettle_sha1,
#ifdef SHA224_DIGEST_SIZE
		[WGET_DIGTYPE_SHA224] = &nettle_sha224,
#endif
#ifdef SHA256_DIGEST_SIZE
		[WGET_DIGTYPE_SHA256] = &nettle_sha256,
#endif
#ifdef SHA384_DIGEST_SIZE
		[WGET_DIGTYPE_SHA384] = &nettle_sha384,
#endif
#ifdef SHA512_DIGEST_SIZE
		[WGET_DIGTYPE_SHA512] = &nettle_sha512,
#endif
};

int wget_hash_fast(wget_digest_algorithm_t algorithm, const void *text, size_t textlen, void *digest)
{
	wget_hash_hd_t dig;

	if (wget_hash_init(&dig, algorithm) == 0) {
		if (wget_hash(&dig, text, textlen) == 0) {
			wget_hash_deinit(&dig, digest);
			return 0;
		}
	}

	return -1;
}

int wget_hash_get_len(wget_digest_algorithm_t algorithm)
{
	if (algorithm >= 0 && algorithm < countof(_nettle_algorithm))
		return _nettle_algorithm[algorithm]->digest_size;
	else
		return 0;
}

int wget_hash_init(wget_hash_hd_t *dig, wget_digest_algorithm_t algorithm)
{
	if (algorithm >= 0 && algorithm < countof(_nettle_algorithm)) {
		dig->hash = _nettle_algorithm[algorithm];
		dig->context = xmalloc(dig->hash->context_size);
		dig->hash->init(dig->context);
		return 0;
	} else {
		dig->hash = NULL;
		dig->context = NULL;
		return -1;
	}
}

int wget_hash(wget_hash_hd_t *dig, const void *text, size_t textlen)
{
	dig->hash->update(dig->context, textlen, text);
	return 0;
}

void wget_hash_deinit(wget_hash_hd_t *dig, void *digest)
{
	dig->hash->digest(dig->context, dig->hash->digest_size, digest);
	xfree(dig->context);
}

#elif defined WITH_GCRYPT
#ifdef HAVE_GCRYPT_H
  #include <gcrypt.h>
#endif

struct _wget_hash_hd_st {
	int
		algorithm;
	gcry_md_hd_t
		context;
};

static const int _gcrypt_algorithm[] = {
	[WGET_DIGTYPE_UNKNOWN] = GCRY_MD_NONE,
	[WGET_DIGTYPE_MD2] = GCRY_MD_MD2,
	[WGET_DIGTYPE_MD5] = GCRY_MD_MD5,
	[WGET_DIGTYPE_RMD160] = GCRY_MD_RMD160,
	[WGET_DIGTYPE_SHA1] = GCRY_MD_SHA1,
	[WGET_DIGTYPE_SHA224] = GCRY_MD_SHA224,
	[WGET_DIGTYPE_SHA256] = GCRY_MD_SHA256,
	[WGET_DIGTYPE_SHA384] = GCRY_MD_SHA384,
	[WGET_DIGTYPE_SHA512] = GCRY_MD_SHA512
};

int wget_hash_fast(wget_digest_algorithm_t algorithm, const void *text, size_t textlen, void *digest)
{
	wget_hash_hd_t dig;

	if (wget_hash_init(&dig, algorithm) == 0) {
		if (wget_hash(&dig, text, textlen) == 0) {
			wget_hash_deinit(&dig, digest);
			return 0;
		}
	}

	return -1;
}

int wget_hash_get_len(wget_digest_algorithm_t algorithm)
{
	if (algorithm >= 0 && algorithm < countof(_gcrypt_algorithm))
		return gcry_md_get_algo_dlen(_gcrypt_algorithm[algorithm]);
	else
		return 0;
}

int wget_hash_init(wget_hash_hd_t *dig, wget_digest_algorithm_t algorithm)
{
	if (algorithm >= 0 && algorithm < countof(_gcrypt_algorithm)) {
		dig->algorithm = _gcrypt_algorithm[algorithm];
		gcry_md_open(&dig->context, dig->algorithm, 0);
		return 0;
	} else {
		return -1;
	}
}

int wget_hash(wget_hash_hd_t *dig, const void *text, size_t textlen)
{
	gcry_md_write(dig->context, text, textlen);
	return 0;
}

void wget_hash_deinit(wget_hash_hd_t *dig, void *digest)
{
	gcry_md_final(dig->context);
	void *ret = gcry_md_read(dig->context, dig->algorithm);
	memcpy(digest, ret, gcry_md_get_algo_dlen(dig->algorithm));
	gcry_md_close(dig->context);
}

#else // use the gnulib functions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include "md2.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
#pragma GCC diagnostic pop

typedef void (*_hash_init_t)(void *);
typedef void (*_hash_process_t)(const void *, size_t, void *);
typedef void (*_hash_finish_t)(void *, void *);
typedef void (*_hash_read_t)(const void *, void *);

static struct _algorithm {
	_hash_init_t init;
	_hash_process_t process;
	_hash_finish_t finish;
	_hash_read_t read;
	size_t ctx_len;
	size_t digest_len;
} _algorithm[] = {
	[WGET_DIGTYPE_MD2] = {
		(_hash_init_t)md2_init_ctx,
		(_hash_process_t)md2_process_bytes,
		(_hash_finish_t)md2_finish_ctx,
		(_hash_read_t)md2_read_ctx,
		sizeof(struct md2_ctx),
		MD2_DIGEST_SIZE
	},
	[WGET_DIGTYPE_MD5] = {
		(_hash_init_t)md5_init_ctx,
		(_hash_process_t)md5_process_bytes,
		(_hash_finish_t)md5_finish_ctx,
		(_hash_read_t)md5_read_ctx,
		sizeof(struct md5_ctx),
		MD5_DIGEST_SIZE
	},
	[WGET_DIGTYPE_SHA1] = {
		(_hash_init_t)sha1_init_ctx,
		(_hash_process_t)sha1_process_bytes,
		(_hash_finish_t)sha1_finish_ctx,
		(_hash_read_t)sha1_read_ctx,
		sizeof(struct sha1_ctx),
		SHA1_DIGEST_SIZE
	},
	[WGET_DIGTYPE_SHA224] = {
		(_hash_init_t)sha224_init_ctx,
		(_hash_process_t)sha256_process_bytes, // sha256 is intentional
		(_hash_finish_t)sha224_finish_ctx,
		(_hash_read_t)sha224_read_ctx,
		sizeof(struct sha256_ctx), // sha256 is intentional
		SHA224_DIGEST_SIZE
	},
	[WGET_DIGTYPE_SHA256] = {
		(_hash_init_t)sha256_init_ctx,
		(_hash_process_t)sha256_process_bytes,
		(_hash_finish_t)sha256_finish_ctx,
		(_hash_read_t)sha256_read_ctx,
		sizeof(struct sha256_ctx),
		SHA256_DIGEST_SIZE
	},
	[WGET_DIGTYPE_SHA384] = {
		(_hash_init_t)sha384_init_ctx,
		(_hash_process_t)sha512_process_bytes, // sha512 is intentional
		(_hash_finish_t)sha384_finish_ctx,
		(_hash_read_t)sha384_read_ctx,
		sizeof(struct sha512_ctx), // sha512 is intentional
		SHA384_DIGEST_SIZE
	},
	[WGET_DIGTYPE_SHA512] = {
		(_hash_init_t)sha512_init_ctx,
		(_hash_process_t)sha512_process_bytes,
		(_hash_finish_t)sha512_finish_ctx,
		(_hash_read_t)sha512_read_ctx,
		sizeof(struct sha512_ctx),
		SHA512_DIGEST_SIZE
	}
};

struct _wget_hash_hd_st {
	const struct _algorithm
		*algorithm;
	void
		*context;
};

int wget_hash_fast(wget_digest_algorithm_t algorithm, const void *text, size_t textlen, void *digest)
{
	wget_hash_hd_t dig;

	if (wget_hash_init(&dig, algorithm) == 0) {
		if (wget_hash(&dig, text, textlen) == 0) {
			wget_hash_deinit(&dig, digest);
			return 0;
		}
	}

	return -1;
}

int wget_hash_get_len(wget_digest_algorithm_t algorithm)
{
	if (algorithm >= 0 && algorithm < countof(_algorithm))
		return _algorithm[algorithm].digest_len;
	else
		return 0;
}

int wget_hash_init(wget_hash_hd_t *dig, wget_digest_algorithm_t algorithm)
{
	if (algorithm >= 0 && algorithm < countof(_algorithm)) {
		if (_algorithm[algorithm].ctx_len) {
			dig->algorithm = &_algorithm[algorithm];
			dig->context = xmalloc(dig->algorithm->ctx_len);
			dig->algorithm->init(dig->context);
			return 0;
		}
	}

	return -1;
}

int wget_hash(wget_hash_hd_t *dig, const void *text, size_t textlen)
{
	dig->algorithm->process(text, textlen, dig->context);
	return 0;
}

void wget_hash_deinit(wget_hash_hd_t *dig, void *digest)
{
	dig->algorithm->finish(dig->context, digest);
	xfree(dig->context);
}
#endif

/**
 * \param[in] hashname Name of the hashing algorithm. See wget_hash_get_algorithm()
 * \param[in] fd File descriptor for the target file
 * \param[out] digest_hex caller-supplied buffer that will contain the resulting hex string
 * \param[in] digest_hex_size Length of \p digest_hex
 * \param[in] offset File offset to start hashing at
 * \param[in] length Number of bytes to hash, starting from \p offset. Zero will hash up to the end of the file
 * \return 0 on success or -1 in case of failure
 *
 * Compute the hash of the contents of the target file and return its hex representation.
 *
 * This function will encode the resulting hash in a string of hex digits, and
 * place that string in the user-supplied buffer \p digest_hex.
 */
int wget_hash_file_fd(const char *hashname, int fd, char *digest_hex, size_t digest_hex_size, off_t offset, off_t length)
{
	wget_digest_algorithm_t algorithm;
	int ret=-1;
	struct stat st;

	if (digest_hex_size)
		*digest_hex=0;

	if (fd == -1 || fstat(fd, &st) != 0)
		return -1;

	if (length == 0)
		length = st.st_size;

	if (offset + length > st.st_size)
		return -1;

	debug_printf("%s hashing pos %llu, length %llu...\n", hashname, (unsigned long long)offset, (unsigned long long)length);

	if ((algorithm = wget_hash_get_algorithm(hashname)) != WGET_DIGTYPE_UNKNOWN) {
		unsigned char digest[wget_hash_get_len(algorithm)];

#ifdef HAVE_MMAP
		char *buf = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, offset);

		if (buf != MAP_FAILED) {
			if (wget_hash_fast(algorithm, buf, length, digest) == 0) {
				wget_memtohex(digest, sizeof(digest), digest_hex, digest_hex_size);
				ret = 0;
			}
			munmap(buf, length);
		} else {
#endif
			// Fallback to read
			ssize_t nbytes = 0;
			wget_hash_hd_t dig;
			char tmp[65536];

			wget_hash_init(&dig, algorithm);
			while (length > 0 && (nbytes = read(fd, tmp, sizeof(tmp))) > 0) {
				wget_hash(&dig, tmp, nbytes);

				if (nbytes <= length)
					length -= nbytes;
				else
					length = 0;
			}
			wget_hash_deinit(&dig, digest);

			if (nbytes < 0) {
				error_printf(_("%s: Failed to read %llu bytes\n"), __func__, (unsigned long long)length);
				close(fd);
				return -1;
			}

			wget_memtohex(digest, sizeof(digest), digest_hex, digest_hex_size);
			ret = 0;
#ifdef HAVE_MMAP
		}
#endif
	}

	return ret;
}

/**
 * \param[in] hashname Name of the hashing algorithm. See wget_hash_get_algorithm()
 * \param[in] fname Target file name
 * \param[out] digest_hex Caller-supplied buffer that will contain the resulting hex string
 * \param[in] digest_hex_size Length of \p digest_hex
 * \param[in] offset File offset to start hashing at
 * \param[in] length Number of bytes to hash, starting from \p offset.  Zero will hash up to the end of the file
 * \return 0 on success or -1 in case of failure
 *
 * Compute the hash of the contents of the target file starting from \p offset and up to \p length bytes
 * and return its hex representation.
 *
 * This function will encode the resulting hash in a string of hex digits, and
 * place that string in the user-supplied buffer \p digest_hex.
 */
int wget_hash_file_offset(const char *hashname, const char *fname, char *digest_hex, size_t digest_hex_size, off_t offset, off_t length)
{
	int fd, ret;

	if ((fd = open(fname, O_RDONLY|O_BINARY)) == -1) {
		if (digest_hex_size)
			*digest_hex=0;
		return 0;
	}

	ret = wget_hash_file_fd(hashname, fd, digest_hex, digest_hex_size, offset, length);
	close(fd);

	return ret;
}

/**
 * \param[in] hashname Name of the hashing algorithm. See wget_hash_get_algorithm()
 * \param[in] fname Target file name
 * \param[out] digest_hex Caller-supplied buffer that will contain the resulting hex string
 * \param[in] digest_hex_size Length of \p digest_hex
 * \return 0 on success or -1 in case of failure
 *
 * Compute the hash of the contents of the target file and return its hex representation.
 *
 * This function will encode the resulting hash in a string of hex digits, and
 * place that string in the user-supplied buffer \p digest_hex.
 */
int wget_hash_file(const char *hashname, const char *fname, char *digest_hex, size_t digest_hex_size)
{
	return wget_hash_file_offset(hashname, fname, digest_hex, digest_hex_size, 0, 0);
}

/**@}*/
