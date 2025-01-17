/*
 * Copyright (c) 2022 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

/*
 * Builtin filters.
 *
 * Typical tokenization pipeline:
 *
 *	tokenizer => normalizer -> stopword filter -> stemmer => terms
 */

#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include <libstemmer.h>

#define __NXSLIB_PRIVATE
#include "nxs_impl.h"
#include "filters.h"
#include "strbuf.h"
#include "tokenizer.h"
#include "rhashmap.h"
#include "utf8.h"

/*
 * Basic token normalizer.
 */

static void *
normalizer_create(const char *lang)
{
	return utf8_ctx_create(lang);
}

static void
normalizer_destroy(void *arg)
{
	utf8_ctx_t *ctx = arg;
	utf8_ctx_destroy(ctx);
}

static filter_action_t
normalizer_filter(void *arg, strbuf_t *buf)
{
	utf8_ctx_t *ctx = arg;

	/*
	 * Lowercase and Unicode NFKC normalization.
	 */
	if (utf8_normalize(ctx, buf) == -1) {
		return FILT_ERROR;
	}

	/*
	 * TODO: Substitute diacritics.
	 */

	return FILT_MUTATION;
}

static const filter_ops_t normalizer_ops = {
	.create		= normalizer_create,
	.destroy	= normalizer_destroy,
	.filter		= normalizer_filter,
};

/*
 * Stopwords.
 */

static rhashmap_t *	swdicts = NULL;

static int
stopwords_sysinit(nxs_t *nxs)
{
	char *dbpath, *line = NULL;
	size_t lcap = 0;
	ssize_t len;
	FILE *fp;

	swdicts = rhashmap_create(0, RHM_NOCOPY | RHM_NONCRYPTO);  // XXX
	if (asprintf(&dbpath, "%s/filters/stopwords/%s",
	    nxs->basedir, "en") == -1) {
		return -1;
	}
	fp = fopen(dbpath, "r");
	free(dbpath);

	if (fp == NULL) {
		/* No stop words. */
		return 0;
	}
	while ((len = getline(&line, &lcap, fp)) > 0) {
		if (len == 0) {
			continue;
		}
		line[len - 1] = '\0';
		rhashmap_put(swdicts, line, len, (void *)(uintptr_t)0x1);
	}
	free(line);
	fclose(fp);
	return 0;
}

static void *
stopwords_create(const char *lang)
{
	return rhashmap_get(swdicts, lang, strlen(lang));
}

static filter_action_t
stopwords_filter(void *arg, strbuf_t *buf)
{
	rhashmap_t *swmap = arg;

	if (swmap && rhashmap_get(swmap, buf->value, buf->length)) {
		return FILT_DROP;
	}
	return FILT_MUTATION;  // pass-through
}

static const filter_ops_t stopwords_ops = {
	.create		= stopwords_create,
	.destroy	= NULL,
	.filter		= stopwords_filter,
};

/*
 * Stemmer
 */

static void *
stemmer_create(const char *lang)
{
	return sb_stemmer_new(lang, NULL /* UTF-8 */);
}

static void
stemmer_destroy(void *arg)
{
	struct sb_stemmer *sbs = arg;
	sb_stemmer_delete(sbs);
}

static filter_action_t
stemmer_filter(void *arg, strbuf_t *buf)
{
	struct sb_stemmer *sbs = arg;
	const char *stemmed;
	size_t len;

	stemmed = (const char *)sb_stemmer_stem(sbs,
	    (const unsigned char *)buf->value, buf->length);
	len = sb_stemmer_length(sbs);

	/*
	 * NOTE: The return value of sb_stemmer_stem() is global and
	 * must copied (unfortunately, the API is not re-entrant).
	 */
	if (strbuf_acquire(buf, stemmed, len) == -1) {
		return FILT_ERROR;
	}
	return FILT_MUTATION;
}

static const filter_ops_t stemmer_ops = {
	.create		= stemmer_create,
	.destroy	= stemmer_destroy,
	.filter		= stemmer_filter,
};

/*
 * Register the builtin filters.
 */

int
filters_builtin_sysinit(nxs_t *nxs)
{
	if (stopwords_sysinit(nxs) == -1) {
		return -1;
	}
	nxs_filter_register(nxs, "normalizer", &normalizer_ops);
	nxs_filter_register(nxs, "stopwords", &stopwords_ops);
	nxs_filter_register(nxs, "stemmer", &stemmer_ops);
	return 0;
}
