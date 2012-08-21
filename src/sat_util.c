#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <iconv.h>
#include <glib.h>

#include "sat_manager.h"
#include "util.h"

#define	tabGsmUniMax2 9
#define	tabGsmUniMax 42

static void _convert_gsm_to_utf8(unsigned char *dest, unsigned short *dest_len,	unsigned char *src, unsigned int src_len);

static gboolean _convert_gsm_to_unicode(unsigned short *dest, int dest_len, unsigned char *src, unsigned int src_len);
static gboolean _convert_unicode_to_gsm(unsigned char* dest, int dest_len, unsigned short* src, int src_len);

static char* _convert_ucs_to_utf8(unsigned char *src, int src_len);
static int _convert_utf8_to_unicode(unsigned short* dest, unsigned char* src, unsigned int src_len);

static unsigned short* _convert_process_unicode(unsigned short *dest, int dest_buf_len,	unsigned char *src, unsigned long src_len);

static int _convert_ucs2_to_gsm(unsigned char* dest, unsigned short* src, unsigned int src_len);
static int _convert_gsm_to_ucs2(unsigned short* dest, unsigned char* src, unsigned int src_len);

static gboolean _find_gsm_code_exception_table(unsigned short src);
static int _get_gsm_code_size(unsigned short* src, int src_len);
static unsigned short* _swap_byte_order(unsigned short *dest, unsigned short *src, int len);

typedef struct {
	char gsm;
	unsigned short unicode;
} GsmUniTable;

const GsmUniTable gsm_unicode2_table[] = {
		{ 0x14, 0x005E }, { 0x28, 0x007B }, { 0x29, 0x007D }, { 0x2F, 0x005C },
		{ 0x3C, 0x005B }, { 0x3D, 0x007E }, { 0x3E, 0x005D }, { 0x40, 0x007C },
		{ 0x65, 0x20AC } };

const GsmUniTable gsm_unicode_table[] = {
		{ 0x00, 0x0040 }, { 0x01, 0x00A3 }, { 0x02, 0x0024 }, { 0x03, 0x00A5 },
		{ 0x04, 0x00E8 }, { 0x05, 0x00E9 }, { 0x06, 0x00F9 }, { 0x07, 0x00EC }, { 0x08, 0x00F2 },
		{ 0x09, 0x00E7 }, { 0x0B, 0x00D8 }, { 0x0C, 0x00F8 }, { 0x0E, 0x00C5 }, { 0x0F, 0x00E5 },
		{ 0x10, 0x0394 }, { 0x11, 0x005F }, { 0x12, 0x03A6 }, { 0x13, 0x0393 }, { 0x14, 0x039B },
		{ 0x15, 0x03A9 }, { 0x16, 0x03A0 }, { 0x17, 0x03A8 }, { 0x18, 0x03A3 }, { 0x19,	0x0398 },
		{ 0x1A, 0x039E }, { 0x1C, 0x00C6 }, { 0x1D, 0x00E6 }, { 0x1E, 0x00DF }, { 0x1F, 0x00C9 },
		{ 0x24, 0x00A4 }, { 0x40, 0x00A1 }, { 0x5B, 0x00C4 }, { 0x5C, 0x00D6 }, { 0x5D, 0x00D1 },
		{ 0x5E, 0x00DC }, { 0x5F, 0x00A7 }, { 0x60, 0x00BF }, { 0x7B, 0x00E4 }, { 0x7C, 0x00F6 },
		{ 0x7D, 0x00F1 }, { 0x7E, 0x00FC }, { 0x7F, 0x00E0 }, };

static int _convert_utf8_to_unicode(unsigned short* dest, unsigned char* src, unsigned int src_len)
{
	unsigned short* org = NULL;
	unsigned char hi = 0;
	unsigned char mid = 0;
	unsigned char low = 0;

	if ((NULL == dest) || (NULL == src)) {
		dbg( "[SAT] INPUT PARAM NULL");
		return -1;
	}

	org = dest;

	while (src_len > 0 && (*src != '\0')) {
		if (*src < 0x80) {
			*dest = (*src & 0x7F);
			dest++;
			src++;
			src_len--;
		} else if (((0xC0 <= *src) && (*src < 0xE0)) && (*(src + 1) >= 0x80)) {
			hi = *src & 0x1F;
			low = *(src+1) & 0x3F;
			*dest = (hi << 6) | low;
			dest++;
			src += 2;
			src_len -= 2;
		} else if ((*src >= 0xE0) && (*(src + 1) >= 0x80) && (*(src + 2) >= 0x80)) {
			hi = *src & 0x0F;
			mid = *(src+1) & 0x3F;
			low = *(src+2) & 0x3F;
			*dest = (hi << 12) | (mid << 6) | low;
			dest++;
			src += 3;
			src_len -= 3;
		} else {
			*dest = (*src & 0x7F);
			dest++;
			src++;
			src_len--;
			dbg( "[SAT] utf8 incorrect range");
		}
	}
	*dest = 0;
	return (dest - org);
}

static gboolean _find_gsm_code_exception_table(unsigned short src)
{
	if ((src >= 0x0020 && src <= 0x0023)
			|| (src >= 0x0025 && src <= 0x003F)
			|| (src >= 0x0041 && src <= 0x005A)
			|| (src >= 0x0061 && src <= 0x007A)
			|| src == 0x000A || src == 0x000D)
		return TRUE;
	return FALSE;
}

static int _get_gsm_code_size(unsigned short* src, int src_len)
{
	gboolean in_table = FALSE;
	gboolean in_sec_table = FALSE;
	int i, gsm_len = 0;

	if (NULL == src) {
		dbg( "INPUT PARAM was NULL");
		return -1;
	}

	for (; src_len > 0 && src; src_len--) {
		if (_find_gsm_code_exception_table(*src) == TRUE) {
			src++;
			gsm_len++;
			continue;
		}
		in_table = FALSE;
		for (i = 0; i < tabGsmUniMax; i++) {
			if (*src == gsm_unicode_table[i].unicode) {
				src++;
				in_table = TRUE;
				gsm_len++;
				break;
			}
		}
		if (in_table == FALSE) {
			in_sec_table = FALSE;
			for (i = 0; i < tabGsmUniMax2; i++) {/* second table */
				if (*src == gsm_unicode2_table[i].unicode) {
					src++;
					in_table = TRUE;
					in_sec_table = TRUE;
					gsm_len += 2;
					break;
				}
			}
			if (in_sec_table == FALSE) {/* second*/
				if (_find_gsm_code_exception_table(*src) == FALSE) {
					dbg( "GSM Char[%d], gsm_len[%d]", *src, gsm_len);
					return -1;
				}
				src++;
				gsm_len++;
			}
		}
	}
	return gsm_len;
}

static int _convert_ucs2_to_gsm(unsigned char* dest, unsigned short* src, unsigned int src_len)
{
	unsigned char* rear = NULL;
	unsigned short* p;
	unsigned char temp;
	gboolean in_table = FALSE;
	gboolean in_sec_table = FALSE;
	int i, gc_len = 0;

	if ((!dest) || (!src) || (0x00 == src_len)) {
		dbg( "Warning: Wrong Input");
		return -1;
	}

	rear = dest;
	p = src;

	for (; src_len > 0 && p; src_len--) {
		in_table = FALSE;
		for (i = 0; i < tabGsmUniMax; i++) { /* is in table  */
			if (*p == gsm_unicode_table[i].unicode) {
				temp = (unsigned char) (gsm_unicode_table[i].gsm);
				*rear = temp;
				rear++;
				p++;
				in_table = TRUE;
				gc_len++;
				break;
			}
		}
		if (in_table == FALSE) {
			in_sec_table = FALSE;
			for (i = 0; i < tabGsmUniMax2; i++) { /* second table*/
				if (*p == gsm_unicode2_table[i].unicode) {
					*rear = 0x1B;
					rear++;
					temp = (unsigned char) (gsm_unicode2_table[i].gsm);
					*rear = temp;
					rear++;
					p++;
					in_table = TRUE;
					in_sec_table = TRUE;
					gc_len += 2;
					break;
				}
			}
			if (in_sec_table == FALSE) { /* second */
				if (_find_gsm_code_exception_table(*p) == FALSE)
					return -1;
				temp = (unsigned char) (*p); /* isn't in table */
				*rear = temp;
				rear++;
				p++;
				gc_len++;
			}
		}
	}
	src = p;
	return gc_len;
}

static gboolean _convert_unicode_to_gsm(unsigned char* dest, int dest_len, unsigned short* src, int src_len)
{
	char* tmp_str;
	int gc_len = 0;

	if ((NULL == dest) || (NULL == src)) {
		dbg( "INPUT PARAM was NULL");
		return FALSE;
	}

	if (src_len == 0)
		return FALSE;

	gc_len = _get_gsm_code_size(src, src_len);
	if (0 >= gc_len) {
		dbg( "Warning: Error[%d] while finding the GSM Code Size", gc_len);
		return FALSE;
	}

	if (dest_len < gc_len) {
		if (dest_len == sizeof(void*)) {
			dbg( "Out buffer size seems to be small (%s)", dest);
		} else {
			dbg("Buffer size is too small (%s): dest_len(%d), gc_len(%d)", dest, dest_len, gc_len);
		}
		return FALSE;
	}

	tmp_str = calloc(1, (unsigned short) gc_len);
	if (tmp_str == NULL) {
		dbg( "Memory Allocation Failed!");
		return FALSE;
	}

	gc_len = _convert_ucs2_to_gsm((unsigned char*) tmp_str, src, src_len);
	if (gc_len != -1) {
		memcpy((char*) dest, (char*) tmp_str, gc_len);
		free(tmp_str);
		return TRUE;
	}

	free(tmp_str);
	return FALSE;
}

void sat_mgr_convert_utf8_to_gsm(unsigned char *dest, int *dest_len, unsigned char* src, unsigned int src_len)
{
	unsigned short *uc = NULL;
	int gc_len = 0;
	int uc_len = 0;
	gboolean ret = FALSE;

	if (src == NULL || src_len == 0) {
		dbg( "WARNING: Invalid Parameter");
		return;
	}

	uc = (unsigned short*) calloc(src_len + 1, sizeof(unsigned short));
	if (!uc) {
		dbg( "WARNING: calloc Failed");
		return;
	}

	/*Converting from UTF8 => UNICODE*/
	uc_len = _convert_utf8_to_unicode(uc, src, src_len);
	dbg( "uc_len:[%d]", uc_len);
	if(uc_len == -1) {
		dbg( "_convert_utf8_to_unicode returns false!");
		free(uc);
		return;
	}

	/*Finding the GSMCode Size*/
	gc_len = _get_gsm_code_size(uc, uc_len);
	dbg( "gc_len:[%d]", gc_len);
	if ( gc_len == -1) {
		dbg( "SM- DATA is not in GSM7BIT Character Set & Error:[%d]",	gc_len);
		free(uc);
		return;
	}

	*dest_len = gc_len;
	/*Converting from UNICODE ==> GSM CODE */
	ret = _convert_unicode_to_gsm((unsigned char*) dest, *dest_len, uc, uc_len);
	if (ret == FALSE) {
		dbg( "_convert_unicode_to_gsm Failed");
		*dest_len = 0x00;
		free(uc);
		return;
	}

	if(uc)
		free(uc);
}

void sat_mgr_convert_utf8_to_ucs2(unsigned char* dest, int* dest_len,	unsigned char* src, int src_len)
{
	gsize byte_converted = 0;
	gsize byte_read = 0;
	gchar* str_converted = NULL;
	GError *error = NULL;
	int i;
	char tmp_char;

	if (dest == NULL || dest_len == NULL || src == NULL) {
		dbg( "Invalid Input Parameter");
		return;
	}

	/*Converting from UTF8 => UCS-2 using the g_convert*/
	str_converted = (gchar*) g_convert((gchar*) src, (gsize) src_len,
																		(gchar*) "UCS-2", (gchar*) "UTF8",
																		(gsize*) &byte_read, (gsize*) &byte_converted,
																		&error);
	if (str_converted == NULL) {
		dbg( "str_converted is NULL");
		if (error != NULL) {
			dbg( "Problem while conversion UTF8 => UCS2, ErrorCode[%d]", error->code);
		}
		return;
	}

	dbg( "src_len[%u], byte_read[%u], byte_converted[%u]", src_len, byte_read, byte_converted);
	*dest_len = (int) byte_converted;

	if (byte_converted % 2 != 0) {
		dbg( "string length is wrong!");
	} else {
		for (i = 0; i < (int)byte_converted; i++) {
			if (i % 2 == 0) {
				tmp_char = str_converted[i];
				str_converted[i] = str_converted[i + 1];
				str_converted[i + 1] = tmp_char;
			}
		}
	}
	memcpy((unsigned char*) dest, (unsigned char*) str_converted, byte_converted);
	g_free(str_converted);
	return;
}

static int _convert_ucs2_to_utf8(char *out, unsigned short *out_len, char *in, unsigned short in_len)
{
	char *p_o = NULL;
	size_t src_len = in_len;
	size_t dest_len = in_len*3;
	short *dest = NULL;
	short *src = NULL;
	int i = 0;

	iconv_t cd = iconv_open("UTF-8", "UCS2");
	if (cd == (iconv_t) (-1)) {
		perror("iconv_open");
		return 0;
	}
//For byte swap - start
	src = (short*)malloc(in_len);
	memcpy(src, in, in_len);
	dest = (short*)malloc(in_len);
    memset(dest, 0, in_len);
    for(i=0; i<in_len/2; i++){
        dest[i] = ((src[i] << 8) + (src[i] >> 8));
    }
    memcpy(in, dest, in_len);
    free(dest);
    free(src);
 //For byte swap - end

	p_o = out;

	dbg("expected input bytes:%d dest_len:%d\n", src_len, dest_len);

	if (iconv(cd, &in, &src_len, &p_o, &dest_len) == (size_t)(-1)) {
		dbg("failed to iconv errno:%d", errno);
	} else {
		dbg("remained input bytes:%d processed bytes:%d", src_len, in_len*3-dest_len);
		out[in_len*3-dest_len] = '\0';
	}
	*out_len = in_len*3-dest_len;
	dbg("out_len[%d], output[%s]", *out_len, out);
	iconv_close(cd);
	return 0;
}

void sat_mgr_convert_string(unsigned char *dest, unsigned short *dest_len,
		enum alphabet_format dcs, unsigned char *src, unsigned short src_len)
{
	int tmp_str_len = 0;
	unsigned char* tmp_dest_str = dest;
	unsigned short* in_buf = NULL;

	/*get string length*/
	/* 0xFF is the end of string */
	while (src[tmp_str_len] != 0xFF && tmp_str_len < src_len) {
		tmp_str_len++;
	}
	/* last space character must be deleted */
	while (src[tmp_str_len - 1] == 0x20 && tmp_str_len > 0) {
		tmp_str_len--;
	}

	dbg( "[SAT] alphabetFormat : [0x%x]", dcs);
	dbg( "[SAT] tmp_str_len[%d] src_len[%d]", tmp_str_len, src_len);

	switch (dcs) {
		case ALPHABET_FROMAT_SMS_DEFAULT:
			tmp_dest_str = (unsigned char*)tcore_util_unpack_gsm7bit((const unsigned char *)src, src_len);
			_convert_gsm_to_utf8(dest, dest_len, tmp_dest_str, strlen((const char*)tmp_dest_str));
			break;

		case ALPHABET_FROMAT_8BIT_DATA:
			_convert_gsm_to_utf8(dest, dest_len, src, tmp_str_len);
			break;

		case ALPHABET_FROMAT_UCS2:{
			if (src[0] == 0x80) {
				dbg("UCS2:[0x%2x] prefix case", src[0]);
				_convert_ucs2_to_utf8((char*) dest, dest_len, (char*) (src + 1), src_len - 1);
			} else if (src[0] == 0x81 || src[0] == 0x82) {
				dbg("UCS2:[0x%2x] prefix case", src[0]);
				*dest_len = tmp_str_len;
				in_buf = (unsigned short*) malloc(tmp_str_len * sizeof(unsigned short) + 1);
				if (!in_buf) {
					dbg( "[SAT] Error:malloc failed");
					return;
				}
				_convert_process_unicode(in_buf, (tmp_str_len * sizeof(unsigned short) + 1), src,
						tmp_str_len);
				dest = (unsigned char*) _convert_ucs_to_utf8((unsigned char *) in_buf, tmp_str_len);
				if (in_buf) free(in_buf);

				if (!dest) {
					dbg( "[SAT] WARNING: dest str is NULL");
					return;
				}
				memcpy(tmp_dest_str, dest, SAT_TEXT_STRING_LEN_MAX);

				if (tmp_dest_str != dest) {
					dbg( "[SAT] destination string address changed hence freeing");
					free(dest);
					dest = NULL;
				}
				dbg( "[SAT] string[%s]", dest);
				dbg( "[SAT] out put string[%s]", tmp_dest_str);
				dbg( "[SAT] string pointer after parsing dcs[0x%x]", dest);
				dbg( "[SAT] string length[%d]", *dest_len);
			} else {
				dbg("UCS2: no prefix case");
				_convert_ucs2_to_utf8((char*) dest, dest_len, (char*) src, src_len);
			}
		}
			break;

		default:
			dbg("not handled alpha format[%d]", dcs);
			break;
	}

}

static int _convert_gsm_to_ucs2(unsigned short* dest, unsigned char* src, unsigned int src_len)
{
	int index;
	unsigned short* org;

	org = dest;

	for(index=0; index < (int)src_len; index++){
		int table_index=0;
		gboolean b_tabled = FALSE;

		/*
		 * if the first byte is 0x1B, it is the escape character.
		 * The byte value shoulbe be changed to unicode.
		 */
		if(*src == 0x1B){
			src++; index++;//move to next byte
			for(table_index=0; table_index < tabGsmUniMax2; table_index++){
				if(*src == gsm_unicode2_table[table_index].gsm){
					*dest = gsm_unicode2_table[table_index].unicode;
					b_tabled = TRUE;
					break;
				}
			}

			//if matched data is not in table, it should be changed to NULL;
			if(!b_tabled){
				*dest = 0x0020;
			}
		}
		else{
			for(table_index=0; table_index < tabGsmUniMax; table_index++){
				if(*src == gsm_unicode_table[table_index].gsm){
					*dest = gsm_unicode_table[table_index].unicode;
					b_tabled = TRUE;
					break;
				}
			}

			//if matched data is not in table, it is using original value;
			if(!b_tabled){
				*dest = *src;
			}
		}

		//move to next position
		src++; dest++;
	}

	dbg("[SAT] cvt sr(%s), the size of data (%d) ", org, dest - org);
	return (dest - org);
}

static gboolean _convert_gsm_to_unicode(unsigned short *dest, int dest_len, unsigned char *src, unsigned int src_len)
{
	int index, tmp_len;

	if(!dest || !src) {
		dbg( "[SAT] dest(%p) or src(%p) is null",dest, src);
		return FALSE;
	}

	if(!src_len){
		dest[0] = '\0';
		return TRUE;
	}

	dbg("[SAT] source string (%s) len(%d)", src, src_len);

	for(index = 0; index < (int)src_len; index++){
		if(src[index] == 0x1B)
			src_len--;
	}
	dbg("[SAT] strlen excluding escape character (%d)", src_len);

	tmp_len = _convert_gsm_to_ucs2(dest, src, src_len);
	dest[tmp_len] = '\0';

	return TRUE;
}

static char* _convert_ucs_to_utf8(unsigned char* src, int src_len)
{
	char* utf_str = NULL;
	iconv_t cd = NULL;
	size_t ileft = 0;
	size_t oleft = 0;
	int err = 0;

	char* pIn = NULL;
	char* in_buf = NULL;
	char* out_buf = NULL;

	if (!src) {
		dbg("src is null");
		return NULL;
	}

	ileft = src_len * 2;//over allocate as utf-8 may occupy 3 bytes
	oleft = src_len * 3;//over allocate as utf-8 may occupy 3 bytes
	pIn = in_buf = (char*) malloc(ileft + 2);
	utf_str = out_buf = (char *) malloc(oleft + 1);

	memset(in_buf, 0x00, ileft + 2);
	memset(out_buf, 0x00, oleft + 1);
	memcpy(in_buf, src, ileft);

	in_buf[ileft] = '\0';

	cd = iconv_open("UTF-8", "UCS-2");
	err = iconv(cd, (char**) &in_buf, &ileft, &out_buf, &oleft);

	utf_str[src_len * 2 - ileft] = '\0';
	iconv_close(cd);
	free(pIn);
	return utf_str;
}

static void _convert_gsm_to_utf8(unsigned char* dest, unsigned short* dest_len, unsigned char* src, unsigned int src_len)
{
	int tmp_len = 0;
	char *target_tmp = NULL;
	unsigned char *raw_unicode = NULL;
	unsigned short tmp_dest[SAT_TEXT_STRING_LEN_MAX];

	memset(tmp_dest, 0 , SAT_TEXT_STRING_LEN_MAX);

	_convert_gsm_to_unicode(tmp_dest, SAT_TEXT_STRING_LEN_MAX, src, src_len);
	while(tmp_dest[tmp_len] != '\0')
		tmp_len++;
	tmp_len++; // add null character

	tmp_len = tmp_len*2; //for byte align
	raw_unicode = (unsigned char*)malloc(tmp_len);
	memset(raw_unicode, 0, tmp_len);

	memcpy(raw_unicode, (unsigned char*)tmp_dest, tmp_len);

	*dest_len = tmp_len;
	target_tmp = _convert_ucs_to_utf8(raw_unicode, tmp_len);
	if(!target_tmp){
		dbg( "str is NULL");
		return;
	}

	memcpy(dest, target_tmp, strlen((const char*)target_tmp));
	dbg("final utf8 str (%s), length (%d)", dest, tmp_len);
	return;
}

/*
 * UCS2 character decoding for SIM alpha identifier as defined ETSI 102 221.
 */
static unsigned short* _convert_process_unicode(unsigned short* dest, int dest_buf_len,	unsigned char* src, unsigned long src_len)
{
	int i, j, base = 0, str_len = 0, shift =0;
	unsigned short tmp_str[255 + 1];

	switch (src[0]) {
		case 0x80: {
			str_len = (src_len - 1) / 2;
			memcpy(tmp_str, &src[1], src_len - 1);
			tmp_str[str_len] = '\0';
			_swap_byte_order(dest, tmp_str, str_len);
			dest[str_len] = '\0';
			return dest;
		}

		case 0x81: {
			str_len = (int) src[1];
			base = (int) src[2];
			j = 3;
			shift = 7;
		}
			break;

		case 0x82: {
			str_len = (int) src[1];
			base = (int) (src[2] << 8 | src[3]);
			j = 4;
			shift = 0;
		}
			break;

		default: {
			for (i = 0; i < (int) src_len; i++) {
				if (src[i] == 0xFF) {
					src_len = i;
					break;
				}
			}
			str_len = (src_len) / 2;
			memcpy(tmp_str, &src[0], src_len);
			tmp_str[str_len] = '\0';
			_swap_byte_order(dest, tmp_str, str_len);
			dest[str_len] = '\0';
			return dest;
			break;
		}
	}

	dbg( "[SAT] str_len[%d], dest_buf_len[%d]", str_len, dest_buf_len);

	for (i = 0; i < str_len && i < dest_buf_len; i++, j++) {
		if (src[j] <= 0x7F) {/* GSM Code */
			if (src[j] == 0x1B) {/* 2Byte GSM Code */
				_convert_gsm_to_unicode(&dest[i], 2, &src[j], 2);
				j++;
			} else {
				_convert_gsm_to_unicode(&dest[i], 2, &src[j], 1);
			}
		} else {
			dest[i] = (unsigned short) ((base << shift)	+ (src[j] & 0x7F));
			dbg( "[SAT] converted0x%x\n", src[j]);
			dbg( "[SAT] converted0x%x\n", dest[i]);
		}
	}
	dest[i] = '\0';
	return dest;
}

static unsigned short* _swap_byte_order(unsigned short* dest, unsigned short* src, int src_len)
{
	int i;
	for (i = 0; i < src_len; i++) {
		if (src[i] == '\0')
			break;
		dest[i] = ((src[i] << 8) + (src[i] >> 8));
	}
	dest[i] = '\0';
	return dest;
}
