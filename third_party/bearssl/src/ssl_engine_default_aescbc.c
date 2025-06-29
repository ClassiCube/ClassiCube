/*
 * Copyright (c) 2017 Thomas Pornin <pornin@bolet.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "inner.h"

/* see bearssl_ssl.h */
void
br_ssl_engine_set_default_aes_cbc(br_ssl_engine_context *cc)
{
#if BR_AES_X86NI
	const br_block_cbcenc_class *ienc;
	const br_block_cbcdec_class *idec;
#endif

	br_ssl_engine_set_cbc(cc,
		&br_sslrec_in_cbc_vtable,
		&br_sslrec_out_cbc_vtable);
#if BR_AES_X86NI
	ienc = br_aes_x86ni_cbcenc_get_vtable();
	idec = br_aes_x86ni_cbcdec_get_vtable();
	if (ienc != NULL && idec != NULL) {
		br_ssl_engine_set_aes_cbc(cc, ienc, idec);
		return;
	}
#endif
	br_ssl_engine_set_aes_cbc(cc,
		&br_aes_big_cbcenc_vtable,
		&br_aes_big_cbcdec_vtable);
}
